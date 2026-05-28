#include "ExpPtrCaptureHook.h"
#include "../../Memory/SaveSpaceHook.h"
#include "../../SDK/Signatures.h"

namespace ExpPtrCaptureHook
{
    // AOB: CC 48 8B 81 38 03 00 00 C3
    //   CC                      int3              (함수 정렬 패딩)
    //   48 8B 81 38 03 00 00    mov rax, [rcx+0x338]  ← 함수 첫 명령어 (7바이트)
    //   C3                      ret
    //
    // sig 매치 위치 = CC. 실제 함수 진입(mov) 은 +1.
    // → sigOffset = 1, patchLen = 7 (mov 만 패치, ret 침범 안 함).
    static constexpr size_t kSigOffset = 1;
    static constexpr size_t kPatchLen  = 7;

    static SaveSpaceHook::Handle* g_handle = nullptr;

    bool Install()
    {
        g_handle = SaveSpaceHook::Install("ExpPtrCaptureHook",
                                          "Palworld-Win64-Shipping.exe",
                                          SDK::Signatures::ExpPtrCapture,
                                          kPatchLen,
                                          SaveSpaceHook::Reg::Rcx,
                                          kSigOffset);
        return true;
    }

    void Shutdown()
    {
        SaveSpaceHook::Shutdown(g_handle);
        g_handle = nullptr;
    }

    void* GetCapturedThis()
    {
        return SaveSpaceHook::Current(g_handle);
    }
}
