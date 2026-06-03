// IntegrityScannerModule.cpp  (External)
#include "IntegrityScannerModule.h"

#include <cstring>

namespace {

using namespace ac;

// 한 번에 원격에서 읽어 비교할 청크 크기.
constexpr uint32_t kChunkSize = 64 * 1024;
// 인접 차이 구간 병합 허용 간격(이 이하로 떨어진 차이는 한 구간으로 묶음).
constexpr uint32_t kCoalesceGap = 8;
// 섹션당 수집할 차이 바이트 상한(전체가 다른 손상/패킹 이미지 폭주 방지).
constexpr uint32_t kMaxDiffBytes = 200000;

// 변조 구간의 라이브 바이트를 보고 종류를 분류.
const char* ClassifyPatch(const uint8_t* live, size_t n) {
    if (n == 0) return "code modification";
    bool allNop = true;
    for (size_t i = 0; i < n; ++i) {
        if (live[i] != 0x90) { allNop = false; break; }
    }
    if (allNop) return "NOP patch (instruction disabled)";
    if (const char* stub = ClassifyJumpStub(live, n)) {
        (void)stub;
        return "inline hook / trampoline";
    }
    return "code modification";
}

}  // namespace

namespace ac {

bool IntegrityScannerModule::Initialize(DetectionEventQueue* queue) {
    queue_ = queue;
    return queue_ != nullptr;
}

void IntegrityScannerModule::Shutdown() {
    queue_ = nullptr;
    reported_.clear();
    currentPid_ = 0;
}

void IntegrityScannerModule::ResetSession(DWORD pid) {
    currentPid_ = pid;
    reported_.clear();
}

bool IntegrityScannerModule::WantModule(const std::string& nameLower) const {
    if (nameLower == target_.imageNameLower) return true;  // 기본: 메인 exe
    for (const auto& m : extraModules_) if (m == nameLower) return true;
    return false;
}

void IntegrityScannerModule::ScanModule(HANDLE proc, const RemoteModule& mod) {
    DiskImage img;
    if (!img.Open(mod.path) || !img.Valid()) {
        std::string key = "noimg:" + mod.nameLower;
        if (reported_.insert(key).second) {
            Report("Integrity.DiskImageUnavailable", Severity::Low,
                   "디스크 원본을 열 수 없어 무결성 검사를 건너뜀: " + mod.nameLower,
                   "module=" + mod.nameLower + " path=" + WideToUtf8(mod.path));
        }
        return;
    }

    const int64_t delta = static_cast<int64_t>(mod.base) - static_cast<int64_t>(img.PreferredBase());
    std::vector<RelocEntry> relocs;
    if (delta != 0) img.BuildRelocs(relocs);  // delta==0 이면 재배치 보정 불필요

    uint32_t regionsReported = 0;
    bool truncatedNote = false;

    for (const auto& s : img.Sections()) {
        if (!(s.Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;  // 코드 섹션만

        uint32_t secRva  = s.VirtualAddress;
        uint32_t vsize   = s.Misc.VirtualSize;
        uint32_t rawsize = s.SizeOfRawData;
        uint32_t cmpLen  = (vsize && vsize < rawsize) ? vsize : rawsize;  // min(>0)
        if (cmpLen == 0) continue;

        const uint8_t* disk = img.RvaPtr(secRva, cmpLen);
        if (!disk) continue;

        // (1) 청크 단위로 라이브 vs 디스크 비교 → (재배치 제외) 차이 오프셋 수집.
        std::vector<uint32_t> diffs;  // 섹션 시작 기준 오프셋
        std::vector<uint8_t> buf(kChunkSize);
        bool overflow = false;

        for (uint32_t off = 0; off < cmpLen && !overflow; off += kChunkSize) {
            uint32_t want = (cmpLen - off < kChunkSize) ? (cmpLen - off) : kChunkSize;
            if (!ReadRemote(proc, mod.base + secRva + off, buf.data(), want)) {
                // 일부 구간 읽기 실패(보호/언커밋) — 해당 청크 건너뜀.
                continue;
            }
            for (uint32_t i = 0; i < want; ++i) {
                if (buf[i] == disk[off + i]) continue;
                uint32_t rva = secRva + off + i;
                if (delta != 0 && RangeIsRelocated(relocs, rva, 1)) continue;  // ASLR 정상차이
                diffs.push_back(off + i);
                if (diffs.size() >= kMaxDiffBytes) { overflow = true; break; }
            }
        }

        if (diffs.empty()) continue;

        if (overflow) {
            std::string key = "massdiff:" + mod.nameLower + ":" + std::to_string(secRva);
            if (reported_.insert(key).second) {
                Report("Integrity.MassMismatch", Severity::Medium,
                       "코드 섹션 다수 바이트가 디스크 원본과 불일치(패킹/언팩 또는 손상 가능): " +
                           mod.nameLower,
                       "module=" + mod.nameLower + " section_rva=" + PtrHex(secRva) +
                           " diff_bytes>=" + std::to_string(kMaxDiffBytes));
            }
            continue;  // 폭주 구간은 개별 신고 생략
        }

        // (2) 인접 차이 오프셋을 구간으로 병합.
        for (size_t k = 0; k < diffs.size();) {
            uint32_t start = diffs[k];
            uint32_t end   = diffs[k] + 1;
            size_t   m     = k + 1;
            while (m < diffs.size() && diffs[m] <= end + kCoalesceGap) {
                end = diffs[m] + 1;
                ++m;
            }
            k = m;

            uint32_t rvaAbs = secRva + start;
            uint32_t len    = end - start;

            if (regionsReported >= maxRegions_) {
                if (!truncatedNote) {
                    truncatedNote = true;
                    Report("Integrity.RegionsTruncated", Severity::Info,
                           "변조 구간이 많아 일부만 신고함: " + mod.nameLower,
                           "module=" + mod.nameLower +
                               " reported=" + std::to_string(maxRegions_) + " (and more)");
                }
                continue;
            }

            std::string key = "patch:" + mod.nameLower + ":" + PtrHex(rvaAbs);
            if (!reported_.insert(key).second) continue;

            // 신고용 라이브/디스크 바이트(최대 16) 확보.
            uint32_t showLen = len < 16 ? len : 16;
            uint8_t liveBytes[16] = {};
            ReadRemote(proc, mod.base + rvaAbs, liveBytes, showLen);
            const uint8_t* diskBytes = disk + start;  // 섹션 내 오프셋

            const char* kind = ClassifyPatch(liveBytes, showLen);
            Severity sev = (std::strstr(kind, "hook") != nullptr) ? Severity::Critical
                                                                  : Severity::High;

            Report("Integrity.CodePatch", sev,
                   std::string("코드 섹션 변조 발견(") + kind + "): " + mod.nameLower,
                   "module=" + mod.nameLower + " va=" + PtrHex(mod.base + rvaAbs) +
                       " rva=" + PtrHex(rvaAbs) + " len=" + std::to_string(len) +
                       " disk=[" + BytesToHex(diskBytes, showLen, true) + "]" +
                       " live=[" + BytesToHex(liveBytes, showLen, true) + "]" +
                       " kind=" + kind);
            ++regionsReported;
        }
    }
}

void IntegrityScannerModule::Report(const std::string& type, Severity sev,
                                    const std::string& desc, const std::string& evidence) {
    if (!queue_) return;
    DetectionEvent ev;
    ev.moduleName    = Name();
    ev.detectionType = type;
    ev.severity      = sev;
    ev.description   = desc;
    ev.evidence      = evidence;
    ev.timestampMs   = NowMsSystem();
    queue_->Push(std::move(ev));
}

void IntegrityScannerModule::Tick() {
    if (!queue_) return;

    DWORD pid = target_.Resolve();
    if (pid == 0) { ResetSession(0); return; }

    HANDLE proc = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!proc) return;

    std::vector<RemoteModule> mods;
    EnumRemoteModules(pid, mods);

    if (pid != currentPid_) ResetSession(pid);  // 첫 관측/재시작 → 캐시 초기화

    for (const auto& mod : mods) {
        if (WantModule(mod.nameLower)) ScanModule(proc, mod);
    }

    ::CloseHandle(proc);
}

}  // namespace ac
