#include "AmmoDecreaseHook.h"
#include "../../Memory/NopHook.h"
#include "../../SDK/Signatures.h"
#include "../../GUI/Menu.h"

namespace AmmoDecreaseHook
{
    // lea eax,[rdx-1]            (3바이트)
    // mov [rcx+0x84], eax        (6바이트)
    // → 총 9바이트 NOP. 한 쌍이 함께 실행되어야 의미가 있으므로 둘 다 skip.
    static constexpr size_t kPatchLen    = 9;
    static constexpr size_t kNopInstrLen = 9;

    static MidHook::Handle* g_handle = nullptr;

    bool Install()
    {
        g_handle = NopHook::Install("AmmoDecreaseHook", "Palworld-Win64-Shipping.exe",
                                    SDK::Signatures::AmmoDecrease, kPatchLen, kNopInstrLen);
        return true;
    }

    void Tick()
    {
        MidHook::SetEnabled(g_handle, Menu::Config.bHookInfAmmo);
    }

    void Shutdown()
    {
        MidHook::Shutdown(g_handle);
        g_handle = nullptr;
    }
}
