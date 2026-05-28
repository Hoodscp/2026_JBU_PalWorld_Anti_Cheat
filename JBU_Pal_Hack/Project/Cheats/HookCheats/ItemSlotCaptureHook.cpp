#include "ItemSlotCaptureHook.h"
#include "../../Memory/SaveSpaceHook.h"
#include "../../SDK/Signatures.h"

namespace ItemSlotCaptureHook
{
    // AOB 9바이트 전체를 patch. 한 명령어로 추정.
    // rcx 가 인벤토리 컨테이너 this 포인터.
    static constexpr size_t kPatchLen = 9;

    static SaveSpaceHook::Handle* g_handle = nullptr;

    bool Install()
    {
        g_handle = SaveSpaceHook::Install("ItemSlotCaptureHook",
                                          "Palworld-Win64-Shipping.exe",
                                          SDK::Signatures::ItemSlotCapture,
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
