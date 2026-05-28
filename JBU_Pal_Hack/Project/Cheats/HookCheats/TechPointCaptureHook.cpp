#include "TechPointCaptureHook.h"
#include "../../Memory/SaveSpaceHook.h"
#include "../../SDK/Signatures.h"

namespace TechPointCaptureHook
{
    // 2B C1                  sub eax, ecx       (2바이트)
    // 89 87 50 01 00 00      mov [rdi+0x150], eax (6바이트)
    // → patchLen 8. rdi 캡처.
    static constexpr size_t kPatchLen = 8;

    static SaveSpaceHook::Handle* g_handle = nullptr;

    bool Install()
    {
        g_handle = SaveSpaceHook::Install("TechPointCaptureHook",
                                          "Palworld-Win64-Shipping.exe",
                                          SDK::Signatures::TechPointCapture,
                                          kPatchLen,
                                          SaveSpaceHook::Reg::Rdi);
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
