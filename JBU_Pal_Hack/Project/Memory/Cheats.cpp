#include "Cheats.h"
#include "../GUI/Menu.h"
#include "../SDK/Pal.h"

namespace Cheats
{
    static constexpr int64_t kGodModeHP = 1'000'000'000;

    void Tick()
    {
        if (Menu::Config.bGodMode) {
            SDK::SetLocalPlayerHealth(kGodModeHP);
        }

        // TODO(Team C): bInfiniteStamina, bInfiniteAmmo 적용 로직
    }
}
