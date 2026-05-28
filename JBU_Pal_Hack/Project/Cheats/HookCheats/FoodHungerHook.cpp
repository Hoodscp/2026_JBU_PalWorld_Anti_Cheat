#include "FoodHungerHook.h"
#include "../../Memory/NopHook.h"
#include "../../SDK/Signatures.h"
#include "../../GUI/Menu.h"

namespace FoodHungerHook
{
    // AOB 14바이트 등록되어 있지만 마지막 `72 07` 은 jb rel8 (relative jump) 라
    // 트램폴린에 복사하면 안 됨. patchLen 을 12 로 잘라 안전 영역만 패치.
    //   F3 0F 5C D7         subss xmm2,xmm7        (4바이트, NOP 대상)
    //   0F 28 7C 24 40      movaps xmm7,[rsp+0x40] (5바이트, tail)
    //   0F 2F D1            comiss xmm2,xmm1       (3바이트, tail)
    static constexpr size_t kPatchLen    = 12;
    static constexpr size_t kNopInstrLen = 4;

    static MidHook::Handle* g_handle = nullptr;

    bool Install()
    {
        g_handle = NopHook::Install("FoodHungerHook", "Palworld-Win64-Shipping.exe",
                                    SDK::Signatures::DoesFoodHungerSub, kPatchLen, kNopInstrLen);
        return true;
    }

    void Tick()
    {
        MidHook::SetEnabled(g_handle, Menu::Config.bHookNoHunger);
    }

    void Shutdown()
    {
        MidHook::Shutdown(g_handle);
        g_handle = nullptr;
    }
}
