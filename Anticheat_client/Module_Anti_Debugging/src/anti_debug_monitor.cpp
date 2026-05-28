#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <winternl.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

    constexpr ULONG ProcessDebugPort = 7;
    constexpr ULONG ProcessDebugObjectHandle = 30;
    constexpr ULONG ProcessDebugFlags = 31;
    constexpr NTSTATUS STATUS_PORT_NOT_SET_VALUE = static_cast<NTSTATUS>(0xC0000353L);

    constexpr ULONG FLG_HEAP_ENABLE_TAIL_CHECK = 0x00000010;
    constexpr ULONG FLG_HEAP_ENABLE_FREE_CHECK = 0x00000020;
    constexpr ULONG FLG_HEAP_VALIDATE_PARAMETERS = 0x00000040;
    constexpr ULONG DEBUG_HEAP_FLAGS =
        FLG_HEAP_ENABLE_TAIL_CHECK | FLG_HEAP_ENABLE_FREE_CHECK | FLG_HEAP_VALIDATE_PARAMETERS;
    constexpr DWORD DEFAULT_INTERVAL_MS = 3000;
    constexpr SIZE_T MEMORY_SCAN_CHUNK_SIZE = 64 * 1024;
    constexpr BYTE INT3_OPCODE = 0xCC;

#ifndef DBG_CONTROL_C
    constexpr DWORD DBG_CONTROL_C = 0x40010005;
#endif

#ifndef DBG_PRINTEXCEPTION_C
    constexpr DWORD DBG_PRINTEXCEPTION_C = 0x40010006;
#endif

    using NtQueryInformationProcessFn = NTSTATUS(NTAPI*)(
        HANDLE ProcessHandle,
        PROCESSINFOCLASS ProcessInformationClass,
        PVOID ProcessInformation,
        ULONG ProcessInformationLength,
        PULONG ReturnLength);

    struct CheckResult {
        std::string name;
        bool detected = false;
        bool available = true;
        std::string detail;
        std::string koreanDetail;
    };

    struct ProbeReport {
        bool processAlive = true;
        bool debuggerDetected = false;
        std::vector<CheckResult> checks;
    };

    struct MonitorState {
        bool hasPreviousTiming = false;
        ULONGLONG previousProcessTimeMs = 0;
        ULONGLONG previousWallTickMs = 0;
        int zeroCpuTimeStreak = 0;
    };

    volatile bool g_running = true;
    thread_local bool g_exceptionProbeHandled = false;

    BOOL WINAPI ConsoleHandler(DWORD controlType) {
        if (controlType == CTRL_C_EVENT || controlType == CTRL_BREAK_EVENT ||
            controlType == CTRL_CLOSE_EVENT) {
            g_running = false;
            return TRUE;
        }
        return FALSE;
    }

    std::string GetLastErrorText(DWORD errorCode) {
        if (errorCode == ERROR_SUCCESS) {
            return "success";
        }

        LPSTR buffer = nullptr;
        const DWORD size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            errorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPSTR>(&buffer),
            0,
            nullptr);

        std::string message = size > 0 && buffer ? buffer : "unknown error";
        if (buffer) {
            LocalFree(buffer);
        }

        while (!message.empty() && (message.back() == '\r' || message.back() == '\n' || message.back() == ' ')) {
            message.pop_back();
        }
        return message;
    }

    std::string NtStatusHex(NTSTATUS status) {
        std::ostringstream stream;
        stream << "NTSTATUS 0x" << std::hex << std::uppercase << static_cast<unsigned long>(status);
        return stream.str();
    }

    bool NtSuccess(NTSTATUS status) {
        return status >= 0;
    }

    NtQueryInformationProcessFn ResolveNtQueryInformationProcess() {
        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        if (!ntdll) {
            return nullptr;
        }
        return reinterpret_cast<NtQueryInformationProcessFn>(
            GetProcAddress(ntdll, "NtQueryInformationProcess"));
    }

    template <typename T>
    std::optional<T> ReadRemoteValue(HANDLE process, uintptr_t address, std::string& error) {
        T value{};
        SIZE_T bytesRead = 0;
        if (!ReadProcessMemory(
            process,
            reinterpret_cast<LPCVOID>(address),
            &value,
            sizeof(T),
            &bytesRead) ||
            bytesRead != sizeof(T)) {
            error = GetLastErrorText(GetLastError());
            return std::nullopt;
        }
        return value;
    }

    ULONGLONG FileTimeToMs(const FILETIME& fileTime) {
        ULARGE_INTEGER value{};
        value.LowPart = fileTime.dwLowDateTime;
        value.HighPart = fileTime.dwHighDateTime;
        return value.QuadPart / 10000;
    }

    bool IsExecutableProtect(DWORD protect) {
        const DWORD baseProtect = protect & 0xFF;
        return baseProtect == PAGE_EXECUTE ||
            baseProtect == PAGE_EXECUTE_READ ||
            baseProtect == PAGE_EXECUTE_READWRITE ||
            baseProtect == PAGE_EXECUTE_WRITECOPY;
    }

    bool IsReadableProtect(DWORD protect) {
        const DWORD baseProtect = protect & 0xFF;
        return baseProtect == PAGE_READONLY ||
            baseProtect == PAGE_READWRITE ||
            baseProtect == PAGE_WRITECOPY ||
            baseProtect == PAGE_EXECUTE_READ ||
            baseProtect == PAGE_EXECUTE_READWRITE ||
            baseProtect == PAGE_EXECUTE_WRITECOPY;
    }

    CheckResult CheckRemoteDebugger(HANDLE process) {
        BOOL present = FALSE;
        if (!CheckRemoteDebuggerPresent(process, &present)) {
            return {
                "CheckRemoteDebuggerPresent",
                false,
                false,
                GetLastErrorText(GetLastError()),
                "Windows API로 대상 프로세스에 원격 디버거가 붙어 있는지 확인했지만 실패했습니다." };
        }
        return {
            "CheckRemoteDebuggerPresent",
            present == TRUE,
            true,
            present ? "debugger present" : "not present",
            present
                ? "대상 프로세스에 디버거가 연결된 것으로 확인되었습니다."
                : "Windows API 기준으로는 원격 디버거 연결 신호가 없습니다." };
    }

    CheckResult CheckDebugPort(HANDLE process, NtQueryInformationProcessFn ntQuery) {
        if (!ntQuery) {
            return {
                "ProcessDebugPort",
                false,
                false,
                "NtQueryInformationProcess unavailable",
                "ntdll의 NtQueryInformationProcess를 사용할 수 없어 디버그 포트 검사를 건너뜁니다." };
        }

        ULONG_PTR debugPort = 0;
        NTSTATUS status = ntQuery(
            process,
            static_cast<PROCESSINFOCLASS>(ProcessDebugPort),
            &debugPort,
            sizeof(debugPort),
            nullptr);

        if (!NtSuccess(status)) {
            return {
                "ProcessDebugPort",
                false,
                false,
                NtStatusHex(status),
                "ProcessDebugPort 조회가 실패했습니다. 권한 또는 OS 상태 문제일 수 있습니다." };
        }

        const bool detected = debugPort != 0;
        std::ostringstream detail;
        detail << "debug port=" << debugPort;
        return {
            "ProcessDebugPort",
            detected,
            true,
            detail.str(),
            detected
                ? "디버그 포트 값이 0이 아니므로 디버거 연결 가능성이 높습니다."
                : "디버그 포트 값이 0이므로 이 항목에서는 디버거 신호가 없습니다." };
    }

    CheckResult CheckDebugFlags(HANDLE process, NtQueryInformationProcessFn ntQuery) {
        if (!ntQuery) {
            return {
                "ProcessDebugFlags",
                false,
                false,
                "NtQueryInformationProcess unavailable",
                "ntdll의 NtQueryInformationProcess를 사용할 수 없어 디버그 플래그 검사를 건너뜁니다." };
        }

        ULONG debugFlags = 0;
        NTSTATUS status = ntQuery(
            process,
            static_cast<PROCESSINFOCLASS>(ProcessDebugFlags),
            &debugFlags,
            sizeof(debugFlags),
            nullptr);

        if (!NtSuccess(status)) {
            return {
                "ProcessDebugFlags",
                false,
                false,
                NtStatusHex(status),
                "ProcessDebugFlags 조회가 실패했습니다. 권한 또는 OS 상태 문제일 수 있습니다." };
        }

        const bool detected = debugFlags == 0;
        std::ostringstream detail;
        detail << "debug flags=" << debugFlags << " (0 usually means debugged)";
        return {
            "ProcessDebugFlags",
            detected,
            true,
            detail.str(),
            detected
                ? "ProcessDebugFlags 값이 0입니다. Windows에서는 보통 디버깅 중임을 의미합니다."
                : "ProcessDebugFlags 값 기준으로는 디버깅 신호가 없습니다." };
    }

    CheckResult CheckDebugObject(HANDLE process, NtQueryInformationProcessFn ntQuery) {
        if (!ntQuery) {
            return {
                "ProcessDebugObjectHandle",
                false,
                false,
                "NtQueryInformationProcess unavailable",
                "ntdll의 NtQueryInformationProcess를 사용할 수 없어 디버그 오브젝트 검사를 건너뜁니다." };
        }

        HANDLE debugObject = nullptr;
        NTSTATUS status = ntQuery(
            process,
            static_cast<PROCESSINFOCLASS>(ProcessDebugObjectHandle),
            &debugObject,
            sizeof(debugObject),
            nullptr);

        if (!NtSuccess(status)) {
            if (status == STATUS_PORT_NOT_SET_VALUE) {
                return {
                    "ProcessDebugObjectHandle",
                    false,
                    true,
                    "no debug object",
                    "대상 프로세스에 연결된 디버그 오브젝트가 없습니다." };
            }
            return {
                "ProcessDebugObjectHandle",
                false,
                false,
                NtStatusHex(status),
                "ProcessDebugObjectHandle 조회가 실패했습니다. 권한 또는 OS 상태 문제일 수 있습니다." };
        }

        if (debugObject) {
            CloseHandle(debugObject);
        }

        std::ostringstream detail;
        detail << "debug object handle=" << debugObject;
        return {
            "ProcessDebugObjectHandle",
            debugObject != nullptr,
            true,
            detail.str(),
            debugObject
                ? "디버그 오브젝트 핸들이 존재하므로 디버거 연결 가능성이 높습니다."
                : "디버그 오브젝트 핸들이 없어 이 항목에서는 디버거 신호가 없습니다." };
    }

    CheckResult CheckPebBeingDebugged(HANDLE process, NtQueryInformationProcessFn ntQuery) {
        if (!ntQuery) {
            return {
                "PEB.BeingDebugged",
                false,
                false,
                "NtQueryInformationProcess unavailable",
                "PEB 주소를 얻을 수 없어 BeingDebugged 플래그 검사를 건너뜁니다." };
        }

        PROCESS_BASIC_INFORMATION basicInfo{};
        NTSTATUS status = ntQuery(
            process,
            ProcessBasicInformation,
            &basicInfo,
            sizeof(basicInfo),
            nullptr);

        if (!NtSuccess(status) || !basicInfo.PebBaseAddress) {
            return {
                "PEB.BeingDebugged",
                false,
                false,
                NtStatusHex(status),
                "대상 프로세스의 PEB 기본 정보를 조회하지 못했습니다." };
        }

        std::string error;
        const uintptr_t pebAddress = reinterpret_cast<uintptr_t>(basicInfo.PebBaseAddress);
        auto beingDebugged = ReadRemoteValue<BYTE>(process, pebAddress + 0x02, error);
        if (!beingDebugged) {
            return {
                "PEB.BeingDebugged",
                false,
                false,
                error,
                "대상 프로세스 메모리에서 PEB.BeingDebugged 값을 읽지 못했습니다." };
        }

        std::ostringstream detail;
        detail << "PEB=" << basicInfo.PebBaseAddress << ", BeingDebugged=" << static_cast<int>(*beingDebugged);
        return {
            "PEB.BeingDebugged",
            *beingDebugged != 0,
            true,
            detail.str(),
            *beingDebugged
                ? "PEB의 BeingDebugged 플래그가 켜져 있어 디버깅 중일 가능성이 높습니다."
                : "PEB의 BeingDebugged 플래그가 꺼져 있습니다." };
    }

    CheckResult CheckPebNtGlobalFlag(HANDLE process, NtQueryInformationProcessFn ntQuery) {
        if (!ntQuery) {
            return {
                "PEB.NtGlobalFlag",
                false,
                false,
                "NtQueryInformationProcess unavailable",
                "PEB 주소를 얻을 수 없어 NtGlobalFlag 검사를 건너뜁니다." };
        }

        PROCESS_BASIC_INFORMATION basicInfo{};
        NTSTATUS status = ntQuery(
            process,
            ProcessBasicInformation,
            &basicInfo,
            sizeof(basicInfo),
            nullptr);

        if (!NtSuccess(status) || !basicInfo.PebBaseAddress) {
            return {
                "PEB.NtGlobalFlag",
                false,
                false,
                NtStatusHex(status),
                "대상 프로세스의 PEB 기본 정보를 조회하지 못했습니다." };
        }

        BOOL selfWow64 = FALSE;
        BOOL targetWow64 = FALSE;
        IsWow64Process(GetCurrentProcess(), &selfWow64);
        IsWow64Process(process, &targetWow64);

        const uintptr_t pebAddress = reinterpret_cast<uintptr_t>(basicInfo.PebBaseAddress);
        const uintptr_t offset = targetWow64 ? 0x68 : 0xBC;

        if (selfWow64 && !targetWow64) {
            return {
                "PEB.NtGlobalFlag",
                false,
                false,
                "32-bit monitor cannot read 64-bit PEB reliably",
                "32비트 감시 프로그램에서는 64비트 대상 PEB를 안정적으로 읽기 어렵습니다." };
        }

        std::string error;
        auto ntGlobalFlag = ReadRemoteValue<ULONG>(process, pebAddress + offset, error);
        if (!ntGlobalFlag) {
            return {
                "PEB.NtGlobalFlag",
                false,
                false,
                error,
                "대상 프로세스 메모리에서 PEB.NtGlobalFlag 값을 읽지 못했습니다." };
        }

        const bool detected = (*ntGlobalFlag & DEBUG_HEAP_FLAGS) != 0;
        std::ostringstream detail;
        detail << "NtGlobalFlag=0x" << std::hex << std::uppercase << *ntGlobalFlag;
        return {
            "PEB.NtGlobalFlag",
            detected,
            true,
            detail.str(),
            detected
                ? "디버그 힙 관련 NtGlobalFlag 비트가 켜져 있습니다. 디버거 실행 흔적일 수 있습니다."
                : "NtGlobalFlag의 디버그 힙 관련 비트가 꺼져 있습니다." };
    }

    CheckResult CheckTimeBasedExecutionGap(HANDLE process, MonitorState& state) {
        FILETIME creationTime{};
        FILETIME exitTime{};
        FILETIME kernelTime{};
        FILETIME userTime{};
        if (!GetProcessTimes(process, &creationTime, &exitTime, &kernelTime, &userTime)) {
            return {
                "TimeBased.CpuGap",
                false,
                false,
                GetLastErrorText(GetLastError()),
                "대상 프로세스의 CPU 사용 시간을 조회하지 못해 시간 기반 검사를 수행할 수 없습니다." };
        }

        const ULONGLONG processTimeMs = FileTimeToMs(kernelTime) + FileTimeToMs(userTime);
        const ULONGLONG wallTickMs = GetTickCount64();

        if (!state.hasPreviousTiming) {
            state.hasPreviousTiming = true;
            state.previousProcessTimeMs = processTimeMs;
            state.previousWallTickMs = wallTickMs;
            return {
                "TimeBased.CpuGap",
                false,
                true,
                "baseline captured",
                "첫 검사에서는 기준 시간을 저장합니다. 다음 주기부터 시간 정체 여부를 비교합니다." };
        }

        const ULONGLONG cpuDelta = processTimeMs - state.previousProcessTimeMs;
        const ULONGLONG wallDelta = wallTickMs - state.previousWallTickMs;
        state.previousProcessTimeMs = processTimeMs;
        state.previousWallTickMs = wallTickMs;

        if (wallDelta >= DEFAULT_INTERVAL_MS / 2 && cpuDelta == 0) {
            ++state.zeroCpuTimeStreak;
        }
        else {
            state.zeroCpuTimeStreak = 0;
        }

        const bool detected = state.zeroCpuTimeStreak >= 2;
        std::ostringstream detail;
        detail << "wall_delta=" << wallDelta << "ms, cpu_delta=" << cpuDelta
            << "ms, zero_cpu_streak=" << state.zeroCpuTimeStreak
            << " (idle processes can produce false positives)";
        return {
            "TimeBased.CpuGap",
            detected,
            true,
            detail.str(),
            detected
                ? "대상 프로세스의 CPU 시간이 연속으로 증가하지 않았습니다. 일시정지/브레이크 상태일 가능성이 있습니다."
                : "대상 프로세스의 시간 흐름에서 뚜렷한 정지 신호는 없습니다. 단, idle 프로세스는 오탐 가능성이 있습니다." };
    }

    CheckResult CheckSoftwareBreakpoints(HANDLE process) {
        uintptr_t address = 0;
        MEMORY_BASIC_INFORMATION memoryInfo{};
        SIZE_T regionsScanned = 0;
        SIZE_T bytesScanned = 0;
        SIZE_T int3Count = 0;
        std::vector<uintptr_t> samples;

        while (VirtualQueryEx(
            process,
            reinterpret_cast<LPCVOID>(address),
            &memoryInfo,
            sizeof(memoryInfo)) == sizeof(memoryInfo)) {
            const uintptr_t base = reinterpret_cast<uintptr_t>(memoryInfo.BaseAddress);
            const uintptr_t next = base + memoryInfo.RegionSize;

            if (memoryInfo.State == MEM_COMMIT &&
                !(memoryInfo.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
                IsExecutableProtect(memoryInfo.Protect) &&
                IsReadableProtect(memoryInfo.Protect)) {
                ++regionsScanned;
                std::vector<BYTE> buffer(MEMORY_SCAN_CHUNK_SIZE);
                for (uintptr_t cursor = base; cursor < next;) {
                    const SIZE_T bytesToRead = static_cast<SIZE_T>(
                        std::min<ULONGLONG>(MEMORY_SCAN_CHUNK_SIZE, next - cursor));
                    SIZE_T bytesRead = 0;
                    if (ReadProcessMemory(
                        process,
                        reinterpret_cast<LPCVOID>(cursor),
                        buffer.data(),
                        bytesToRead,
                        &bytesRead) &&
                        bytesRead > 0) {
                        bytesScanned += bytesRead;
                        for (SIZE_T i = 0; i < bytesRead; ++i) {
                            if (buffer[i] == INT3_OPCODE) {
                                ++int3Count;
                                if (samples.size() < 4) {
                                    samples.push_back(cursor + i);
                                }
                            }
                        }
                    }

                    if (bytesToRead == 0 || cursor + bytesToRead <= cursor) {
                        break;
                    }
                    cursor += bytesToRead;
                }
            }

            if (next <= address) {
                break;
            }
            address = next;
        }

        std::ostringstream detail;
        detail << "regions=" << regionsScanned << ", bytes=" << bytesScanned
            << ", int3_count=" << int3Count;
        if (!samples.empty()) {
            detail << ", samples=";
            for (size_t i = 0; i < samples.size(); ++i) {
                if (i > 0) {
                    detail << ",";
                }
                detail << "0x" << std::hex << std::uppercase << samples[i] << std::dec;
            }
            detail << " (0xCC can also be compiler padding)";
        }

        return {
            "Breakpoint.SoftwareINT3",
            int3Count > 0,
            true,
            detail.str(),
            int3Count > 0
                ? "실행 가능한 메모리에서 INT3(0xCC) 바이트를 발견했습니다. 소프트웨어 브레이크포인트일 수 있습니다."
                : "실행 가능한 메모리에서 INT3(0xCC) 바이트를 발견하지 못했습니다." };
    }

    CheckResult CheckHardwareBreakpoints(DWORD pid) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return {
                "Breakpoint.HardwareDRx",
                false,
                false,
                GetLastErrorText(GetLastError()),
                "스레드 스냅샷을 만들지 못해 하드웨어 브레이크포인트 검사를 수행할 수 없습니다." };
        }

        THREADENTRY32 entry{};
        entry.dwSize = sizeof(entry);
        DWORD threadsSeen = 0;
        DWORD threadsChecked = 0;
        DWORD threadsUnavailable = 0;
        DWORD hardwareHits = 0;

        if (!Thread32First(snapshot, &entry)) {
            const DWORD error = GetLastError();
            CloseHandle(snapshot);
            return {
                "Breakpoint.HardwareDRx",
                false,
                false,
                GetLastErrorText(error),
                "스레드 목록을 읽지 못해 하드웨어 브레이크포인트 검사를 수행할 수 없습니다." };
        }

        do {
            if (entry.th32OwnerProcessID != pid) {
                continue;
            }

            ++threadsSeen;
            HANDLE thread = OpenThread(
                THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION,
                FALSE,
                entry.th32ThreadID);
            if (!thread) {
                ++threadsUnavailable;
                continue;
            }

            const DWORD suspendCount = SuspendThread(thread);
            if (suspendCount == static_cast<DWORD>(-1)) {
                ++threadsUnavailable;
                CloseHandle(thread);
                continue;
            }

            CONTEXT context{};
            context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
            if (GetThreadContext(thread, &context)) {
                ++threadsChecked;
                const bool drEnabled = (context.Dr7 & 0xFF) != 0;
                const bool drAddressSet =
                    context.Dr0 != 0 || context.Dr1 != 0 || context.Dr2 != 0 || context.Dr3 != 0;
                if (drEnabled && drAddressSet) {
                    ++hardwareHits;
                }
            }
            else {
                ++threadsUnavailable;
            }

            ResumeThread(thread);
            CloseHandle(thread);
        } while (Thread32Next(snapshot, &entry));

        CloseHandle(snapshot);

        std::ostringstream detail;
        detail << "threads_seen=" << threadsSeen << ", checked=" << threadsChecked
            << ", unavailable=" << threadsUnavailable << ", dr_hits=" << hardwareHits;
        return {
            "Breakpoint.HardwareDRx",
            hardwareHits > 0,
            threadsChecked > 0,
            detail.str(),
            hardwareHits > 0
                ? "스레드 디버그 레지스터에서 활성 하드웨어 브레이크포인트 흔적을 발견했습니다."
                : "확인 가능한 스레드에서는 하드웨어 브레이크포인트 흔적이 없습니다." };
    }

    LONG WINAPI ExceptionProbeHandler(PEXCEPTION_POINTERS exceptionInfo) {
        if (!exceptionInfo || !exceptionInfo->ExceptionRecord) {
            return EXCEPTION_CONTINUE_SEARCH;
        }

        const DWORD code = exceptionInfo->ExceptionRecord->ExceptionCode;
        if (code == DBG_CONTROL_C || code == DBG_PRINTEXCEPTION_C) {
            g_exceptionProbeHandled = true;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }

    bool ExceptionReachedHandler(DWORD exceptionCode) {
        PVOID handler = AddVectoredExceptionHandler(1, ExceptionProbeHandler);
        if (!handler) {
            return false;
        }

        g_exceptionProbeHandled = false;
        RaiseException(exceptionCode, 0, 0, nullptr);
        const bool handled = g_exceptionProbeHandled;
        RemoveVectoredExceptionHandler(handler);
        return handled;
    }

    CheckResult CheckExceptionProbeSelf() {
        const bool controlCHandled = ExceptionReachedHandler(DBG_CONTROL_C);
        const bool printHandled = ExceptionReachedHandler(DBG_PRINTEXCEPTION_C);
        const bool detected = !controlCHandled || !printHandled;

        std::ostringstream detail;
        detail << "monitor_self_probe: DBG_CONTROL_C="
            << (controlCHandled ? "handled" : "consumed")
            << ", DBG_PRINTEXCEPTION_C="
            << (printHandled ? "handled" : "consumed")
            << " (external PID exception probing needs an in-process module)";
        return {
            "Exception.SelfProbe",
            detected,
            true,
            detail.str(),
            detected
                ? "예외가 감시 프로그램의 핸들러에 도달하지 않았습니다. 감시 프로그램 자체에 디버거가 붙었을 가능성이 있습니다."
                : "예외가 감시 프로그램의 핸들러까지 정상 도달했습니다. self-probe 기준 디버거 신호는 없습니다." };
    }

    ProbeReport ProbeProcess(DWORD pid, HANDLE process, NtQueryInformationProcessFn ntQuery, MonitorState& state) {
        ProbeReport report{};

        DWORD exitCode = 0;
        if (!GetExitCodeProcess(process, &exitCode) || exitCode != STILL_ACTIVE) {
            report.processAlive = false;
            report.checks.push_back({
                "ProcessAlive",
                false,
                true,
                "target process exited",
                "대상 프로세스가 종료되어 감시를 중단합니다." });
            return report;
        }

        report.checks.push_back(CheckRemoteDebugger(process));
        report.checks.push_back(CheckDebugPort(process, ntQuery));
        report.checks.push_back(CheckDebugFlags(process, ntQuery));
        report.checks.push_back(CheckDebugObject(process, ntQuery));
        report.checks.push_back(CheckPebBeingDebugged(process, ntQuery));
        report.checks.push_back(CheckPebNtGlobalFlag(process, ntQuery));
        report.checks.push_back(CheckTimeBasedExecutionGap(process, state));
        report.checks.push_back(CheckSoftwareBreakpoints(process));
        report.checks.push_back(CheckHardwareBreakpoints(pid));
        report.checks.push_back(CheckExceptionProbeSelf());

        for (const auto& check : report.checks) {
            if (check.available && check.detected) {
                report.debuggerDetected = true;
            }
        }

        return report;
    }

    std::string CurrentTimeText() {
        SYSTEMTIME time{};
        GetLocalTime(&time);
        std::ostringstream stream;
        stream << std::setfill('0')
            << std::setw(2) << time.wHour << ":"
            << std::setw(2) << time.wMinute << ":"
            << std::setw(2) << time.wSecond;
        return stream.str();
    }

    void PrintReport(DWORD pid, const ProbeReport& report) {
        std::cout << "[" << CurrentTimeText() << "] PID " << pid << " => "
            << (report.debuggerDetected ? "DEBUGGER DETECTED" : "clean/unknown") << '\n';

        for (const auto& check : report.checks) {
            std::cout << "  - " << std::left << std::setw(28) << check.name << " : ";
            if (!check.available) {
                std::cout << "unavailable";
            }
            else {
                std::cout << (check.detected ? "hit" : "clear");
            }

            if (!check.detail.empty()) {
                std::cout << " (" << check.detail << ")";
            }
            std::cout << '\n';
            if (!check.koreanDetail.empty()) {
                std::cout << "    KR: " << check.koreanDetail << '\n';
            }
        }
        std::cout << std::flush;
    }

    std::optional<DWORD> ParsePid(const char* text) {
        char* end = nullptr;
        const unsigned long value = std::strtoul(text, &end, 10);
        if (!text[0] || *end != '\0' || value == 0 || value > 0xFFFFFFFFul) {
            return std::nullopt;
        }
        return static_cast<DWORD>(value);
    }

    void PrintUsage(const char* executable) {
        std::cerr << "Usage: " << executable << " <PID>\n"
            << "Example: " << executable << " 1234\n";
    }

}  // namespace

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    if (argc != 2) {
        PrintUsage(argv[0]);
        return 2;
    }

    const auto pid = ParsePid(argv[1]);
    if (!pid) {
        PrintUsage(argv[0]);
        return 2;
    }

    const DWORD access =
        PROCESS_QUERY_INFORMATION |
        PROCESS_VM_READ |
        SYNCHRONIZE;

    HANDLE process = OpenProcess(access, FALSE, *pid);
    if (!process) {
        std::cerr << "OpenProcess failed for PID " << *pid << ": "
            << GetLastErrorText(GetLastError()) << '\n';
        return 1;
    }

    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    const auto ntQuery = ResolveNtQueryInformationProcess();
    MonitorState state{};

    std::cout << "Monitoring PID " << *pid << " every " << DEFAULT_INTERVAL_MS
        << " ms. Press Ctrl+C to stop.\n\n";

    while (g_running) {
        const ProbeReport report = ProbeProcess(*pid, process, ntQuery, state);
        PrintReport(*pid, report);
        if (!report.processAlive) {
            break;
        }
        std::cout << '\n';
        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_INTERVAL_MS));
    }

    CloseHandle(process);
    std::cout << "Stopped.\n";
    return 0;
}
