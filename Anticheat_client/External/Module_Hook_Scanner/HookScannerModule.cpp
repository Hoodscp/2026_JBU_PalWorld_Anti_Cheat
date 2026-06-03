// HookScannerModule.cpp  (External)
#include "HookScannerModule.h"

#include <cstring>
#include <utility>

namespace {

using namespace ac;

// 프롤로그 비교 길이. x64 인라인 후크 스텁은 5~13바이트라 16이면 충분.
constexpr uint32_t kPrologueLen = 16;
// IAT 한 모듈당 임포트 thunk 안전 상한(무한 루프/손상 방지).
constexpr uint32_t kMaxThunksPerDll = 8192;

// rva 가 디스크 이미지의 "실행 가능한" 섹션 안에 있는지. 데이터 익스포트를
// 후크로 오탐하지 않도록 거른다.
bool RvaInExecSection(const DiskImage& img, uint32_t rva) {
    for (const auto& s : img.Sections()) {
        uint32_t va = s.VirtualAddress;
        uint32_t sz = s.Misc.VirtualSize ? s.Misc.VirtualSize : s.SizeOfRawData;
        if (rva >= va && rva < va + sz) {
            return (s.Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
        }
    }
    return false;
}

// 라이브 점프 스텁의 목적지를 가능한 범위에서 해석해 사람이 읽는 설명을 만든다.
// 잡히면 어느 모듈/사적메모리로 점프하는지까지 덧붙인다.
std::string DescribeJumpTarget(HANDLE proc, const std::vector<RemoteModule>& mods,
                               uintptr_t funcVa, const uint8_t* b, size_t n) {
    uintptr_t dst = 0;
    if (n >= 5 && b[0] == 0xE9) {
        int32_t rel = 0;
        std::memcpy(&rel, b + 1, 4);
        dst = funcVa + 5 + static_cast<int64_t>(rel);
    } else if (n >= 2 && b[0] == 0xEB) {
        dst = funcVa + 2 + static_cast<int64_t>(static_cast<int8_t>(b[1]));
    } else if (n >= 6 && b[0] == 0xFF && b[1] == 0x25) {
        int32_t disp = 0;
        std::memcpy(&disp, b + 2, 4);
        uintptr_t slot = funcVa + 6 + static_cast<int64_t>(disp);  // [rip+disp] 위치
        auto ptr = ReadRemoteT<uint64_t>(proc, slot);              // 그 안의 실제 목적지
        if (ptr) dst = static_cast<uintptr_t>(*ptr);
    } else if (n >= 12 && b[0] == 0x48 && b[1] == 0xB8) {  // mov rax, imm64; jmp rax
        uint64_t imm = 0;
        std::memcpy(&imm, b + 2, 8);
        dst = static_cast<uintptr_t>(imm);
    } else if (n >= 13 && b[0] == 0x49 && b[1] == 0xBB) {  // mov r11, imm64; jmp r11
        uint64_t imm = 0;
        std::memcpy(&imm, b + 2, 8);
        dst = static_cast<uintptr_t>(imm);
    }

    if (!dst) return {};
    const RemoteModule* m = ModuleOf(mods, dst);
    if (m) return " dest=" + PtrHex(dst) + " (module=" + m->nameLower + ")";
    return " dest=" + PtrHex(dst) + " (UNBACKED/private memory)";  // 사적메모리 = 트램펄린 강한 증거
}

}  // namespace

namespace ac {

void HookScannerModule::AddWatchedModule(const std::string& moduleLower,
                                         std::vector<std::string> hotFuncs) {
    watch_.push_back({ moduleLower, std::move(hotFuncs) });
}

void HookScannerModule::InstallDefaultWatchlist() {
    // 자주 후킹되는 핫 함수 위주. (전체 익스포트 검사는 EnableScanAllExports 로)
    AddWatchedModule("ntdll.dll", {
        "NtProtectVirtualMemory", "NtWriteVirtualMemory", "NtReadVirtualMemory",
        "NtAllocateVirtualMemory", "NtCreateThreadEx", "NtOpenProcess",
        "NtQueryInformationProcess", "NtSetInformationThread", "NtMapViewOfSection",
        "NtUnmapViewOfSection", "NtResumeThread", "NtClose", "NtContinue",
        "NtGetContextThread", "NtSetContextThread", "LdrLoadDll", "LdrGetProcedureAddress",
    });
    AddWatchedModule("kernelbase.dll", {
        "VirtualProtect", "VirtualProtectEx", "VirtualAlloc", "VirtualAllocEx",
        "WriteProcessMemory", "ReadProcessMemory", "GetProcAddress",
        "LoadLibraryExW", "OpenProcess", "IsDebuggerPresent", "CheckRemoteDebuggerPresent",
    });
    AddWatchedModule("kernel32.dll", {
        "LoadLibraryA", "LoadLibraryW", "LoadLibraryExA", "LoadLibraryExW",
        "GetProcAddress", "VirtualProtect", "VirtualProtectEx", "VirtualAlloc",
        "VirtualAllocEx", "WriteProcessMemory", "ReadProcessMemory",
        "CreateRemoteThread", "OpenProcess",
    });
    AddWatchedModule("user32.dll", {
        "SetWindowsHookExW", "SetWindowsHookExA", "GetAsyncKeyState", "GetKeyState",
        "CallNextHookEx", "PeekMessageW", "GetRawInputData",
    });
}

bool HookScannerModule::Initialize(DetectionEventQueue* queue) {
    queue_ = queue;
    if (watch_.empty()) InstallDefaultWatchlist();
    return queue_ != nullptr;
}

void HookScannerModule::Shutdown() {
    queue_ = nullptr;
    reported_.clear();
    currentPid_ = 0;
}

void HookScannerModule::ResetSession(DWORD pid) {
    currentPid_ = pid;
    reported_.clear();
}

void HookScannerModule::ScanModuleExports(HANDLE proc, const std::vector<RemoteModule>& mods,
                                          const RemoteModule& mod, const WatchModule& wm) {
    DiskImage img;
    if (!img.Open(mod.path) || !img.Valid()) {
        // 디스크 원본을 못 열면 비교 불가(약한 신호 1회만).
        std::string key = "noimg:" + mod.nameLower;
        if (reported_.insert(key).second) {
            Report("Hook.DiskImageUnavailable", Severity::Low,
                   "디스크 원본을 열 수 없어 익스포트 후크 검증을 건너뜀: " + mod.nameLower,
                   "module=" + mod.nameLower + " path=" + WideToUtf8(mod.path));
        }
        return;
    }

    std::vector<std::pair<std::string, uint32_t>> exports;
    img.BuildExportMap(exports);

    std::vector<RelocEntry> relocs;
    img.BuildRelocs(relocs);

    // 검사 대상 결정: hotFuncs 우선. scanAllExports_ 면 전체.
    auto shouldCheck = [&](const std::string& name) -> bool {
        if (scanAllExports_) return true;
        for (const auto& f : wm.hotFuncs) if (f == name) return true;
        return false;
    };

    for (const auto& e : exports) {
        const std::string& name = e.first;
        uint32_t rva = e.second;
        if (!shouldCheck(name)) continue;
        if (!RvaInExecSection(img, rva)) continue;  // 데이터 익스포트 제외

        const uint8_t* disk = img.RvaPtr(rva, kPrologueLen);
        if (!disk) continue;

        uint8_t live[kPrologueLen] = {};
        uintptr_t funcVa = mod.base + rva;
        if (!ReadRemote(proc, funcVa, live, kPrologueLen)) continue;

        // 바이트 비교 — ASLR 재배치가 적용되는 위치는 정상적으로 다를 수 있어 건너뜀.
        bool firstDiffers = false;
        bool anyRealDiff  = false;
        for (uint32_t i = 0; i < kPrologueLen; ++i) {
            if (live[i] == disk[i]) continue;
            if (RangeIsRelocated(relocs, rva + i, 1)) continue;  // 재배치 → 합법적 차이
            anyRealDiff = true;
            if (i == 0) firstDiffers = true;
        }
        if (!anyRealDiff) continue;

        const char* stub = ClassifyJumpStub(live, kPrologueLen);
        std::string key = "exp:" + mod.nameLower + "!" + name;
        if (!reported_.insert(key).second) continue;

        std::string evidence =
            "module=" + mod.nameLower + " func=" + name + " va=" + PtrHex(funcVa) +
            " disk=[" + BytesToHex(disk, 8, true) + "]" +
            " live=[" + BytesToHex(live, 8, true) + "]";

        if (firstDiffers && stub) {
            // 함수 첫 명령이 점프 스텁으로 바뀜 = 인라인 후크(트램펄린).
            Report("Hook.InlineExport", Severity::Critical,
                   "시스템 API 프롤로그가 점프 스텁으로 교체됨(인라인 후크): " +
                       mod.nameLower + "!" + name,
                   evidence + " stub=" + stub +
                       DescribeJumpTarget(proc, mods, funcVa, live, kPrologueLen));
        } else {
            // 첫머리가 점프는 아니지만 (재배치 아닌) 바이트가 변조됨.
            Report("Hook.ProloguePatch", Severity::High,
                   "시스템 API 프롤로그 바이트가 디스크 원본과 다름(패치 의심): " +
                       mod.nameLower + "!" + name,
                   evidence);
        }
    }
}

void HookScannerModule::ScanExportHooks(HANDLE proc, const std::vector<RemoteModule>& mods) {
    for (const auto& wm : watch_) {
        if (!scanAllExports_ && wm.hotFuncs.empty()) continue;
        for (const auto& mod : mods) {
            if (mod.nameLower == wm.nameLower) {
                ScanModuleExports(proc, mods, mod, wm);
                break;
            }
        }
    }
}

void HookScannerModule::ScanIatHooks(HANDLE proc, const std::vector<RemoteModule>& mods) {
    // 메인 모듈(게임 exe) 찾기.
    const RemoteModule* mainMod = nullptr;
    for (const auto& m : mods) {
        if (m.nameLower == target_.imageNameLower) { mainMod = &m; break; }
    }
    if (!mainMod) return;

    RemotePeInfo pe = ReadRemotePeInfo(proc, mainMod->base);
    if (!pe.valid || pe.importRva == 0) return;

    // 임포트 디스크립터 배열 순회.
    for (uint32_t di = 0;; ++di) {
        uintptr_t descAddr =
            mainMod->base + pe.importRva + di * sizeof(IMAGE_IMPORT_DESCRIPTOR);
        auto desc = ReadRemoteT<IMAGE_IMPORT_DESCRIPTOR>(proc, descAddr);
        if (!desc) break;
        // NULL 디스크립터 = 끝.
        if (desc->Name == 0 && desc->FirstThunk == 0 && desc->OriginalFirstThunk == 0) break;
        if (di > 4096) break;  // 안전 상한

        std::string dllName =
            ReadRemoteCStr(proc, mainMod->base + desc->Name, 256);
        std::string dllLower = ToLowerAscii(dllName);

        uint32_t iatRva = desc->FirstThunk;
        if (iatRva == 0) continue;

        for (uint32_t j = 0; j < kMaxThunksPerDll; ++j) {
            uintptr_t slot = mainMod->base + iatRva + j * sizeof(uint64_t);
            auto val = ReadRemoteT<uint64_t>(proc, slot);
            if (!val) break;
            uintptr_t fnPtr = static_cast<uintptr_t>(*val);
            if (fnPtr == 0) break;            // thunk 배열 끝
            if (fnPtr < 0x10000) continue;    // 미해석/잡값 방어

            // 정상: IAT 엔트리는 로드된 어떤 모듈의 주소 공간을 가리킨다.
            // forward/ApiSet 으로 다른 시스템 DLL 을 가리키는 건 정상이므로,
            // "어느 모듈에도 안 속하는" 포인터만 후크로 본다(오탐 최소화).
            if (ModuleOf(mods, fnPtr) != nullptr) continue;

            std::string key = "iat:" + dllLower + ":" + std::to_string(j);
            if (!reported_.insert(key).second) continue;

            Report("Hook.IAT", Severity::Critical,
                   "메인 모듈 IAT 엔트리가 로드된 모듈 밖 메모리를 가리킴(IAT 후크 의심)",
                   "import_dll=" + dllLower + " iat_index=" + std::to_string(j) +
                       " slot=" + PtrHex(slot) + " points_to=" + PtrHex(fnPtr) +
                       " (UNBACKED/private memory)");
        }
    }
}

void HookScannerModule::Report(const std::string& type, Severity sev,
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

void HookScannerModule::Tick() {
    if (!queue_) return;

    DWORD pid = target_.Resolve();
    if (pid == 0) { ResetSession(0); return; }  // 대상 미실행 → 다음을 새 세션으로

    HANDLE proc = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!proc) return;  // 권한 부족 — 다음 틱 재시도

    std::vector<RemoteModule> mods;
    EnumRemoteModules(pid, mods);

    if (pid != currentPid_) {
        // 첫 관측/재시작 → 보고 캐시 초기화 후 곧바로 한 번 스캔.
        ResetSession(pid);
    }

    if (exportScan_) ScanExportHooks(proc, mods);
    if (iatScan_)    ScanIatHooks(proc, mods);

    ::CloseHandle(proc);
}

}  // namespace ac
