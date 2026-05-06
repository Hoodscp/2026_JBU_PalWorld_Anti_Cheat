#include "TempControl.h"
#include "../../GUI/Menu.h"
#include "../../SDK/Pal.h"

namespace TempControl
{
    void Tick()
    {
        const auto& cfg = Menu::Config;
        if (!cfg.bForceTemperature) return;

        SDK::SetLocalPlayerCurrentTemperature(cfg.TargetTemperature);
    }
}
