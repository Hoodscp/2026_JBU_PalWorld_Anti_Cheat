#include "TechCheats.h"
#include "../../GUI/Menu.h"
#include "../../SDK/Pal.h"

namespace TechCheats
{
    static constexpr int kForcedTechPoint     = 9999;
    static constexpr int kForcedBossTechPoint = 9999;

    void Tick()
    {
        const auto& cfg = Menu::Config;

        if (cfg.bFreeTechPoint) {
            SDK::SetLocalTechnologyPoint(kForcedTechPoint);
        }
        if (cfg.bFreeBossTechPoint) {
            SDK::SetLocalBossTechnologyPoint(kForcedBossTechPoint);
        }
    }
}
