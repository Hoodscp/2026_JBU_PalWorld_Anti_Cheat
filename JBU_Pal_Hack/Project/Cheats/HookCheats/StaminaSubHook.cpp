#include "StaminaSubHook.h"
#include "../../Memory/NopHook.h"
#include "../../SDK/Signatures.h"
#include "../../GUI/Menu.h"

namespace StaminaSubHook
{
    // AOB 15바이트:
    //   F3 0F 5C C2 — subss xmm0,xmm2 (NOP 대상, 4바이트)
    //   F3 48 0F 2C C0 48 89 01 48 8B C1 — tail (11바이트, RIP-rel 없음, 그대로 실행)
    static constexpr size_t kPatchLen    = 15;
    static constexpr size_t kNopInstrLen = 4;

    static MidHook::Handle* g_handle = nullptr;

    bool Install()
    {
        g_handle = NopHook::Install("StaminaSubHook", "Palworld-Win64-Shipping.exe",
                                    SDK::Signatures::StaminaSub, kPatchLen, kNopInstrLen);
        return true; // placeholder/실패도 다른 hook 영향 X
    }

    void Tick()
    {
        MidHook::SetEnabled(g_handle, Menu::Config.bHookInfStamina);
    }

    void Shutdown()
    {
        MidHook::Shutdown(g_handle);
        g_handle = nullptr;
    }
}
