// AntiDebugModule.cpp  (External)
#include "AntiDebugModule.h"

#include <winternl.h>

namespace {

using namespace ac;

// PROCESSINFOCLASS 확장 값(winternl.h 미정의분).
constexpr ULONG kProcessDebugPort          = 7;
constexpr ULONG kProcessDebugObjectHandle  = 30;
constexpr ULONG kProcessDebugFlags         = 31;
constexpr NTSTATUS kStatusPortNotSet       = static_cast<NTSTATUS>(0xC0000353L);

// 디버그 힙 관련 NtGlobalFlag 비트.
constexpr ULONG kDebugHeapFlags = 0x10 | 0x20 | 0x40;  // tail/free check + validate params

using NtQIP = NTSTATUS(NTAPI*)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

NtQIP ResolveNtQIP() {
    HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return nullptr;
    return reinterpret_cast<NtQIP>(::GetProcAddress(ntdll, "NtQueryInformationProcess"));
}

bool NtOk(NTSTATUS s) { return s >= 0; }

}  // namespace

namespace ac {

bool AntiDebugModule::Initialize(DetectionEventQueue* queue) {
    queue_ = queue;
    return queue_ != nullptr;
}

void AntiDebugModule::Shutdown() {
    queue_ = nullptr;
    reported_.clear();
    currentPid_ = 0;
    hasPrevTiming_ = false;
    zeroCpuStreak_ = 0;
}

void AntiDebugModule::ResetSession(DWORD pid) {
    currentPid_     = pid;
    reported_.clear();
    hasPrevTiming_  = false;
    prevProcMs_     = 0;
    prevWallMs_     = 0;
    zeroCpuStreak_  = 0;
}

void AntiDebugModule::ReportOnce(const std::string& key, const std::string& type,
                                 Severity sev, const std::string& desc,
                                 const std::string& evidence) {
    if (!queue_) return;
    if (!reported_.insert(key).second) return;  // 세션당 1회
    DetectionEvent ev;
    ev.moduleName    = Name();
    ev.detectionType = type;
    ev.severity      = sev;
    ev.description   = desc;
    ev.evidence      = evidence;
    ev.timestampMs   = NowMsSystem();
    queue_->Push(std::move(ev));
}

void AntiDebugModule::CheckNtQueries(HANDLE proc) {
    NtQIP q = ResolveNtQIP();
    if (!q) return;

    // ProcessDebugPort — 0 이 아니면 디버거 연결.
    {
        ULONG_PTR port = 0;
        NTSTATUS s = q(proc, static_cast<PROCESSINFOCLASS>(kProcessDebugPort),
                       &port, sizeof(port), nullptr);
        if (NtOk(s) && port != 0) {
            ReportOnce("dbgport", "AntiDebug.DebugPort", Severity::Critical,
                       "대상 프로세스에 디버그 포트가 설정됨(디버거 연결)",
                       "debug_port=" + std::to_string(static_cast<uint64_t>(port)));
        }
    }
    // ProcessDebugObjectHandle — 핸들 존재 시 디버거.
    {
        HANDLE dbgObj = nullptr;
        NTSTATUS s = q(proc, static_cast<PROCESSINFOCLASS>(kProcessDebugObjectHandle),
                       &dbgObj, sizeof(dbgObj), nullptr);
        if (NtOk(s) && dbgObj != nullptr) {
            ::CloseHandle(dbgObj);
            ReportOnce("dbgobj", "AntiDebug.DebugObject", Severity::Critical,
                       "대상 프로세스에 디버그 오브젝트 핸들 존재(디버거 연결)",
                       "debug_object=present");
        } else if (s != kStatusPortNotSet && !NtOk(s)) {
            // 조회 실패는 신호 아님(권한/OS) — 무시.
        }
    }
    // ProcessDebugFlags — 0 이면 보통 디버깅 중.
    {
        ULONG flags = 1;
        NTSTATUS s = q(proc, static_cast<PROCESSINFOCLASS>(kProcessDebugFlags),
                       &flags, sizeof(flags), nullptr);
        if (NtOk(s) && flags == 0) {
            ReportOnce("dbgflags", "AntiDebug.DebugFlags", Severity::High,
                       "ProcessDebugFlags=0 (디버깅 중일 가능성)",
                       "debug_flags=0");
        }
    }
}

void AntiDebugModule::CheckPeb(HANDLE proc) {
    NtQIP q = ResolveNtQIP();
    if (!q) return;

    PROCESS_BASIC_INFORMATION pbi{};
    NTSTATUS s = q(proc, ProcessBasicInformation, &pbi, sizeof(pbi), nullptr);
    if (!NtOk(s) || !pbi.PebBaseAddress) return;

    uintptr_t peb = reinterpret_cast<uintptr_t>(pbi.PebBaseAddress);

    // PEB.BeingDebugged @ +0x02
    if (auto b = ReadRemoteT<uint8_t>(proc, peb + 0x02)) {
        if (*b != 0) {
            ReportOnce("peb.dbg", "AntiDebug.PebBeingDebugged", Severity::Critical,
                       "PEB.BeingDebugged 플래그가 설정됨",
                       "BeingDebugged=" + std::to_string(static_cast<int>(*b)));
        }
    }
    // PEB.NtGlobalFlag @ +0xBC (x64). 디버그 힙 비트 set 이면 디버거 흔적.
    if (auto f = ReadRemoteT<uint32_t>(proc, peb + 0xBC)) {
        if ((*f & kDebugHeapFlags) != 0) {
            ReportOnce("peb.ntgf", "AntiDebug.NtGlobalFlag", Severity::High,
                       "PEB.NtGlobalFlag 의 디버그 힙 비트가 설정됨",
                       "NtGlobalFlag=" + PtrHex(*f));
        }
    }
}

void AntiDebugModule::CheckHardwareBreakpoints(DWORD pid) {
    HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) return;

    THREADENTRY32 te{};
    te.dwSize = sizeof(te);
    if (!::Thread32First(snap, &te)) { ::CloseHandle(snap); return; }

    do {
        if (te.th32OwnerProcessID != pid) continue;
        HANDLE th = ::OpenThread(
            THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION,
            FALSE, te.th32ThreadID);
        if (!th) continue;

        if (::SuspendThread(th) == static_cast<DWORD>(-1)) { ::CloseHandle(th); continue; }

        CONTEXT ctx{};
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (::GetThreadContext(th, &ctx)) {
            bool drEnabled = (ctx.Dr7 & 0xFF) != 0;
            bool drSet     = ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3;
            if (drEnabled && drSet) {
                ReportOnce("hwbp:" + std::to_string(te.th32ThreadID),
                           "AntiDebug.HardwareBreakpoint", Severity::High,
                           "스레드 디버그 레지스터에 활성 하드웨어 브레이크포인트",
                           "tid=" + std::to_string(te.th32ThreadID) +
                               " dr7=" + PtrHex(ctx.Dr7));
            }
        }
        ::ResumeThread(th);
        ::CloseHandle(th);
    } while (::Thread32Next(snap, &te));

    ::CloseHandle(snap);
}

void AntiDebugModule::CheckTiming(HANDLE proc) {
    FILETIME c{}, e{}, k{}, u{};
    if (!::GetProcessTimes(proc, &c, &e, &k, &u)) return;

    auto toMs = [](const FILETIME& ft) -> uint64_t {
        ULARGE_INTEGER v{};
        v.LowPart = ft.dwLowDateTime; v.HighPart = ft.dwHighDateTime;
        return v.QuadPart / 10000;
    };
    uint64_t procMs = toMs(k) + toMs(u);
    uint64_t wallMs = ::GetTickCount64();

    if (!hasPrevTiming_) {
        hasPrevTiming_ = true;
        prevProcMs_ = procMs;
        prevWallMs_ = wallMs;
        return;
    }

    uint64_t cpuDelta  = procMs - prevProcMs_;
    uint64_t wallDelta = wallMs - prevWallMs_;
    prevProcMs_ = procMs;
    prevWallMs_ = wallMs;

    // 벽시계는 충분히 흘렀는데 CPU 시간이 전혀 안 늘면 정체 의심.
    if (wallDelta >= intervalMs_ / 2 && cpuDelta == 0) ++zeroCpuStreak_;
    else                                                zeroCpuStreak_ = 0;

    if (zeroCpuStreak_ >= 2) {
        ReportOnce("timing", "AntiDebug.ExecutionStalled", Severity::Medium,
                   "대상 CPU 시간이 연속으로 정지(브레이크/일시정지 의심)",
                   "zero_cpu_streak=" + std::to_string(zeroCpuStreak_) +
                       " wall_delta=" + std::to_string(wallDelta) + "ms (idle 오탐 가능)");
    }
}

void AntiDebugModule::Tick() {
    if (!queue_) return;

    DWORD pid = target_.Resolve();
    if (pid == 0) { ResetSession(0); return; }

    HANDLE proc = ::OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE, FALSE, pid);
    if (!proc) return;  // 권한 부족 — 다음 틱 재시도

    if (pid != currentPid_) ResetSession(pid);  // 첫 관측/재시작

    // CheckRemoteDebuggerPresent — 즉시 신호.
    BOOL present = FALSE;
    if (::CheckRemoteDebuggerPresent(proc, &present) && present) {
        ReportOnce("remote", "AntiDebug.RemoteDebugger", Severity::Critical,
                   "CheckRemoteDebuggerPresent: 대상에 디버거 연결됨", "present=1");
    }

    CheckNtQueries(proc);
    CheckPeb(proc);
    if (hwbpScan_)    CheckHardwareBreakpoints(pid);
    if (timingCheck_) CheckTiming(proc);

    ::CloseHandle(proc);
}

}  // namespace ac
