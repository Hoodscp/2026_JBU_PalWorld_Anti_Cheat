#include "PlayerPtrCaptureHook.h"
#include "../../Memory/SaveSpaceHook.h"
#include "../../SDK/Signatures.h"

namespace PlayerPtrCaptureHook
{
    // mov rax, [rcx+0x128] — 7바이트. rcx 가 함수 진입 시 캡처 대상.
    static constexpr size_t kPatchLen = 7;

    static SaveSpaceHook::Handle* g_handle = nullptr;

    bool Install()
    {
        g_handle = SaveSpaceHook::Install("PlayerPtrCaptureHook",
                                          "Palworld-Win64-Shipping.exe",
                                          SDK::Signatures::PlayerPtrCapture,
                                          kPatchLen,
                                          SaveSpaceHook::Reg::Rcx);
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
