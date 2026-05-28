#include "FoodTimerHook.h"
#include "../../Memory/NopHook.h"
#include "../../SDK/Signatures.h"
#include "../../GUI/Menu.h"

namespace FoodTimerHook
{
    // movss [rbx+0x158], xmm0 — 한 명령어 8바이트. 통째로 NOP.
    static constexpr size_t kPatchLen    = 8;
    static constexpr size_t kNopInstrLen = 8;

    static MidHook::Handle* g_handle = nullptr;

    bool Install()
    {
        g_handle = NopHook::Install("FoodTimerHook", "Palworld-Win64-Shipping.exe",
                                    SDK::Signatures::FoodTimerWrite, kPatchLen, kNopInstrLen);
        return true;
    }

    void Tick()
    {
        MidHook::SetEnabled(g_handle, Menu::Config.bHookFreezeFoodTimer);
    }

    void Shutdown()
    {
        MidHook::Shutdown(g_handle);
        g_handle = nullptr;
    }
}
