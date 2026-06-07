#include "CrashHandler.h"
#include "Logger.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <cstdint>
#include <cstdio>

namespace {

    PVOID g_vehHandle = nullptr;
    // 재진입 가드. 핸들러 안에서 예외가 또 나도 무한 반복 방지.
    std::atomic<int> g_inHandler{ 0 };
    // 한 번 치명 로그가 찍힌 뒤엔 중복 신고 방지(다음 같은 예외 무시).
    std::atomic<bool> g_alreadyLogged{ false };

    bool IsFatalCode(DWORD code) {
        switch (code) {
            case EXCEPTION_ACCESS_VIOLATION:     // 0xC0000005
            case EXCEPTION_IN_PAGE_ERROR:        // 0xC0000006
            case EXCEPTION_ILLEGAL_INSTRUCTION:  // 0xC000001D
            case EXCEPTION_PRIV_INSTRUCTION:     // 0xC0000096
            case EXCEPTION_STACK_OVERFLOW:       // 0xC00000FD
            case EXCEPTION_INT_DIVIDE_BY_ZERO:   // 0xC0000094
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:// 0xC000008C
            case 0xC0000409:  // STACK_BUFFER_OVERRUN / __fastfail
            case 0xC0000374:  // HEAP_CORRUPTION
            case 0xC0000028:  // BAD_STACK
                return true;
            default:
                return false;
        }
    }

    // addr 가 속한 모듈명(베이스이름) + RVA. 실패 시 path 비우고 rva = addr.
    void DescribeAddr(uintptr_t addr, char* nameOut, size_t nameCap, uintptr_t& rvaOut) {
        nameOut[0] = '\0';
        rvaOut = addr;
        HMODULE mod = nullptr;
        if (::GetModuleHandleExA(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCSTR>(addr), &mod) && mod) {
            char full[MAX_PATH] = {};
            if (::GetModuleFileNameA(mod, full, MAX_PATH)) {
                // 베이스이름만(경로 제거)
                const char* slash = std::strrchr(full, '\\');
                const char* base  = slash ? slash + 1 : full;
                std::snprintf(nameOut, nameCap, "%s", base);
            }
            rvaOut = addr - reinterpret_cast<uintptr_t>(mod);
        }
    }

    void LogContextRegisters(const CONTEXT* ctx) {
        if (!ctx) return;
        JBU_LOG("  RIP=0x%016llX  RSP=0x%016llX  RBP=0x%016llX",
                static_cast<unsigned long long>(ctx->Rip),
                static_cast<unsigned long long>(ctx->Rsp),
                static_cast<unsigned long long>(ctx->Rbp));
        JBU_LOG("  RAX=0x%016llX  RCX=0x%016llX  RDX=0x%016llX  RBX=0x%016llX",
                static_cast<unsigned long long>(ctx->Rax),
                static_cast<unsigned long long>(ctx->Rcx),
                static_cast<unsigned long long>(ctx->Rdx),
                static_cast<unsigned long long>(ctx->Rbx));
        JBU_LOG("  RSI=0x%016llX  RDI=0x%016llX  R8 =0x%016llX  R9 =0x%016llX",
                static_cast<unsigned long long>(ctx->Rsi),
                static_cast<unsigned long long>(ctx->Rdi),
                static_cast<unsigned long long>(ctx->R8),
                static_cast<unsigned long long>(ctx->R9));
        JBU_LOG("  R10=0x%016llX  R11=0x%016llX  R12=0x%016llX  R13=0x%016llX",
                static_cast<unsigned long long>(ctx->R10),
                static_cast<unsigned long long>(ctx->R11),
                static_cast<unsigned long long>(ctx->R12),
                static_cast<unsigned long long>(ctx->R13));
        JBU_LOG("  R14=0x%016llX  R15=0x%016llX",
                static_cast<unsigned long long>(ctx->R14),
                static_cast<unsigned long long>(ctx->R15));
    }

    void LogBacktrace() {
        void* frames[16] = {};
        USHORT n = ::RtlCaptureStackBackTrace(0, 16, frames, nullptr);
        JBU_LOG("  Backtrace (%u frames):", n);
        for (USHORT i = 0; i < n; ++i) {
            char mod[64];
            uintptr_t rva = 0;
            DescribeAddr(reinterpret_cast<uintptr_t>(frames[i]), mod, sizeof(mod), rva);
            if (mod[0]) {
                JBU_LOG("    #%-2u  %-32s +0x%llX",
                        i, mod, static_cast<unsigned long long>(rva));
            } else {
                JBU_LOG("    #%-2u  0x%016llX  (no module)",
                        i, static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(frames[i])));
            }
        }
    }

    LONG WINAPI VehTopHandler(EXCEPTION_POINTERS* ep) {
        if (!ep || !ep->ExceptionRecord) return EXCEPTION_CONTINUE_SEARCH;
        const DWORD code = ep->ExceptionRecord->ExceptionCode;
        if (!IsFatalCode(code)) return EXCEPTION_CONTINUE_SEARCH;

        // 재진입 / 중복 가드
        int prev = g_inHandler.fetch_add(1, std::memory_order_acquire);
        if (prev != 0) { g_inHandler.fetch_sub(1, std::memory_order_release); return EXCEPTION_CONTINUE_SEARCH; }
        if (g_alreadyLogged.exchange(true)) {
            g_inHandler.fetch_sub(1, std::memory_order_release);
            return EXCEPTION_CONTINUE_SEARCH;
        }

        __try {
            const uintptr_t faultVa = reinterpret_cast<uintptr_t>(ep->ExceptionRecord->ExceptionAddress);
            char mod[64]; uintptr_t rva = 0;
            DescribeAddr(faultVa, mod, sizeof(mod), rva);

            JBU_LOG("================================================================");
            JBU_LOG("FATAL EXCEPTION — code=0x%08lX  va=0x%016llX",
                    static_cast<unsigned long>(code),
                    static_cast<unsigned long long>(faultVa));
            if (mod[0]) {
                JBU_LOG("  In module : %s+0x%llX",
                        mod, static_cast<unsigned long long>(rva));
            } else {
                JBU_LOG("  In module : (unbacked/private memory — manual map or trampoline)");
            }
            JBU_LOG("  Phase     : %s", Logger::CurrentPhase());

            // ACCESS_VIOLATION 추가 정보(읽기/쓰기/실행, 접근 VA)
            if (code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_IN_PAGE_ERROR) {
                const ULONG_PTR* info = ep->ExceptionRecord->ExceptionInformation;
                const ULONG_PTR  kind = info[0];  // 0=read, 1=write, 8=execute
                const ULONG_PTR  addr = info[1];
                const char* k = (kind == 0) ? "READ" : (kind == 1) ? "WRITE" : (kind == 8) ? "DEP" : "?";
                JBU_LOG("  AV: %s access at 0x%016llX",
                        k, static_cast<unsigned long long>(addr));
            }

            LogContextRegisters(ep->ContextRecord);
            LogBacktrace();
            JBU_LOG("================================================================");
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            // 로그 코드 자체가 실패해도 게임 흐름은 영향 받지 않게 흡수.
        }

        g_inHandler.fetch_sub(1, std::memory_order_release);
        // 우리는 관찰만. 원본 SEH/디버거 처리에 양보.
        return EXCEPTION_CONTINUE_SEARCH;
    }

}  // anonymous namespace

namespace CrashHandler {

bool Install() {
    if (g_vehHandle) return true;
    // 우선순위 1 = 다른 VEH 보다 먼저 호출. 우리는 어차피 CONTINUE_SEARCH 하므로 안전.
    g_vehHandle = ::AddVectoredExceptionHandler(1, VehTopHandler);
    JBU_LOG("CrashHandler: VEH %s (handle=0x%p)",
            g_vehHandle ? "installed" : "FAILED to install",
            g_vehHandle);
    return g_vehHandle != nullptr;
}

void Shutdown() {
    if (!g_vehHandle) return;
    ::RemoveVectoredExceptionHandler(g_vehHandle);
    g_vehHandle = nullptr;
    JBU_LOG("CrashHandler: VEH removed.");
}

}  // namespace CrashHandler
