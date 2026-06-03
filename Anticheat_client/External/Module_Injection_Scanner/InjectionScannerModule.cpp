// InjectionScannerModule.cpp  (External)
#include "InjectionScannerModule.h"

#include <softpub.h>
#include <wintrust.h>

#include <string>

#pragma comment(lib, "wintrust.lib")

namespace {

// Authenticode 서명 유효성. true=유효 서명, false=미서명/무효.
// (네트워크 없이 로컬 체인 검증. EXPIRED 등 일부는 미서명으로 보지 않도록 처리.)
bool HasValidSignature(const std::wstring& path) {
    if (path.empty()) return false;

    WINTRUST_FILE_INFO fileInfo{};
    fileInfo.cbStruct       = sizeof(fileInfo);
    fileInfo.pcwszFilePath  = path.c_str();

    GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    WINTRUST_DATA wd{};
    wd.cbStruct       = sizeof(wd);
    wd.dwUIChoice     = WTD_UI_NONE;
    wd.fdwRevocationChecks = WTD_REVOKE_NONE;
    wd.dwUnionChoice  = WTD_CHOICE_FILE;
    wd.pFile          = &fileInfo;
    wd.dwStateAction  = WTD_STATEACTION_VERIFY;
    wd.dwProvFlags    = WTD_SAFER_FLAG | WTD_CACHE_ONLY_URL_RETRIEVAL;

    LONG status = ::WinVerifyTrust(static_cast<HWND>(INVALID_HANDLE_VALUE), &action, &wd);

    wd.dwStateAction = WTD_STATEACTION_CLOSE;
    ::WinVerifyTrust(static_cast<HWND>(INVALID_HANDLE_VALUE), &action, &wd);

    return status == ERROR_SUCCESS;
}

std::wstring DirOfLower(const std::wstring& path) {
    size_t slash = path.find_last_of(L"\\/");
    std::wstring dir = (slash == std::wstring::npos) ? std::wstring() : path.substr(0, slash);
    for (wchar_t& c : dir) if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c - L'A' + L'a');
    return dir;
}

std::wstring ToLowerW(std::wstring s) {
    for (wchar_t& c : s) if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c - L'A' + L'a');
    return s;
}

}  // namespace

namespace ac {

bool InjectionScannerModule::Initialize(DetectionEventQueue* queue) {
    queue_ = queue;
    return queue_ != nullptr;
}

void InjectionScannerModule::Shutdown() {
    queue_ = nullptr;
    baselineBases_.clear();
    reported_.clear();
    currentPid_ = 0;
}

bool InjectionScannerModule::IsTrustedPath(const std::wstring& pathLower) const {
    // 1) 게임 디렉터리(메인 exe 가 있는 폴더) 이하
    if (!gameDirLower_.empty() && pathLower.rfind(gameDirLower_, 0) == 0) return true;
    // 2) 윈도우 시스템 디렉터리
    wchar_t winDir[MAX_PATH] = {};
    if (::GetWindowsDirectoryW(winDir, MAX_PATH)) {
        std::wstring w = ToLowerW(winDir);
        if (pathLower.rfind(w, 0) == 0) return true;
    }
    // 3) 사용자 지정 신뢰 접두사
    for (const auto& d : trustedDirs_) {
        if (!d.empty() && pathLower.rfind(d, 0) == 0) return true;
    }
    return false;
}

void InjectionScannerModule::OnTargetChanged(DWORD pid, HANDLE /*proc*/,
                                             const std::vector<RemoteModule>& mods) {
    currentPid_ = pid;
    baselineBases_.clear();
    reported_.clear();
    gameDirLower_.clear();

    // 메인 exe = 이름이 타깃 이미지명과 같은 모듈. 그 폴더를 게임 디렉터리로 신뢰.
    for (const auto& m : mods) {
        baselineBases_.insert(m.base);
        if (m.nameLower == target_.imageNameLower) {
            gameDirLower_ = DirOfLower(m.path);
        }
    }
}

void InjectionScannerModule::ScanModules(HANDLE /*proc*/, const std::vector<RemoteModule>& mods) {
    for (const auto& m : mods) {
        const std::wstring pathLower = ToLowerW(m.path);
        const bool trusted    = IsTrustedPath(pathLower);
        const bool isBaseline = baselineBases_.count(m.base) != 0;

        // (A) 의심 위치 로드 — 게임/시스템 밖. LoadLibrary 인젝션의 전형.
        if (!trusted) {
            std::string key = "loc:" + WideToUtf8(pathLower);
            if (reported_.insert(key).second) {
                Severity sev = isBaseline ? Severity::High : Severity::Critical;
                Report("Injection.UntrustedPath", sev,
                       "비신뢰 경로에서 로드된 모듈: " + m.nameLower,
                       "module=" + m.nameLower + " path=" + WideToUtf8(m.path) +
                       " base=" + PtrHex(m.base) +
                       (isBaseline ? " (baseline)" : " (post-baseline/new)"));
            }
        }

        // (B) 기준선 이후 새로 나타난 모듈(신뢰 위치라도 알림 — 약한 신호).
        if (!isBaseline) {
            std::string key = "new:" + PtrHex(m.base) + ":" + m.nameLower;
            if (reported_.insert(key).second) {
                Report("Injection.NewModule", trusted ? Severity::Low : Severity::High,
                       "기준선 이후 새 모듈 로드: " + m.nameLower,
                       "module=" + m.nameLower + " path=" + WideToUtf8(m.path) +
                       " base=" + PtrHex(m.base) + (trusted ? " (trusted dir)" : " (untrusted dir)"));
            }
        }

        // (C) 서명 검증 — 시스템 모듈은 대부분 서명됨. 미서명 비시스템 DLL 은 의심.
        if (signatureCheck_ && !trusted) {
            std::string key = "sig:" + WideToUtf8(pathLower);
            if (reported_.count(key) == 0) {
                if (!HasValidSignature(m.path)) {
                    reported_.insert(key);
                    Report("Injection.Unsigned", Severity::High,
                           "유효한 디지털 서명이 없는 모듈: " + m.nameLower,
                           "module=" + m.nameLower + " path=" + WideToUtf8(m.path));
                }
            }
        }
    }
}

void InjectionScannerModule::ScanManualMap(HANDLE proc, const std::vector<RemoteModule>& mods) {
    auto regions = EnumRegions(proc);
    for (const auto& r : regions) {
        if (r.State != MEM_COMMIT) continue;
        if (!IsExecProtect(r.Protect)) continue;
        if (r.Protect & (PAGE_GUARD | PAGE_NOACCESS)) continue;
        // MEM_IMAGE = 정상 로드된 모듈 섹션. MEM_PRIVATE/MEM_MAPPED 실행영역이 수상.
        if (r.Type == MEM_IMAGE) continue;

        uintptr_t base = reinterpret_cast<uintptr_t>(r.BaseAddress);
        // 혹시 로더가 추적 못한 모듈 범위면 제외(이중 보고 방지).
        if (ModuleOf(mods, base)) continue;

        // PE 헤더('MZ') 존재 여부 — 수동 매핑된 DLL 의 강한 증거.
        uint8_t hdr[2] = {};
        bool hasMz = ReadRemote(proc, base, hdr, 2) && hdr[0] == 'M' && hdr[1] == 'Z';

        std::string key = "mm:" + PtrHex(base);
        if (!reported_.insert(key).second) continue;

        if (hasMz) {
            Report("Injection.ManualMapPE", Severity::Critical,
                   "로더 목록에 없는 실행 영역에서 PE('MZ') 헤더 발견 — 수동 매핑 인젝션 의심",
                   "base=" + PtrHex(base) + " size=" + std::to_string(r.RegionSize) +
                   " protect=" + PtrHex(r.Protect) + " type=" +
                   (r.Type == MEM_PRIVATE ? "PRIVATE" : "MAPPED"));
        } else {
            Report("Injection.PrivateExec", Severity::High,
                   "로더 목록에 없는 실행 가능한 사적 메모리 영역 — 셸코드/매핑 코드 의심",
                   "base=" + PtrHex(base) + " size=" + std::to_string(r.RegionSize) +
                   " protect=" + PtrHex(r.Protect) +
                   " (JIT/스크립트 런타임 오탐 가능)");
        }
    }
}

void InjectionScannerModule::Report(const std::string& type, Severity sev,
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

void InjectionScannerModule::Tick() {
    if (!queue_) return;

    DWORD pid = target_.Resolve();
    if (pid == 0) {
        // 대상 미실행. 다음 실행을 새 세션으로 보도록 상태 초기화.
        currentPid_ = 0;
        baselineBases_.clear();
        reported_.clear();
        return;
    }

    HANDLE proc = ::OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!proc) return;  // 권한 부족 — 다음 틱 재시도

    std::vector<RemoteModule> mods;
    EnumRemoteModules(pid, mods);

    if (pid != currentPid_) {
        OnTargetChanged(pid, proc, mods);  // 첫 관측/재시작 → 기준선 수립
    } else {
        ScanModules(proc, mods);
        if (manualMapScan_) ScanManualMap(proc, mods);
    }

    ::CloseHandle(proc);
}

}  // namespace ac
