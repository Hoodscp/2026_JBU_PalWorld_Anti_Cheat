#include "GodMode.h"
#include "../../GUI/Menu.h"
#include "../../SDK/Pal.h"

namespace GodMode
{
    static constexpr int64_t kForcedHP = 1'000'000'000;

    void Tick()
    {
        if (Menu::Config.bGodMode) {
            SDK::SetLocalPlayerHealth(kForcedHP);
        }
    }
}
