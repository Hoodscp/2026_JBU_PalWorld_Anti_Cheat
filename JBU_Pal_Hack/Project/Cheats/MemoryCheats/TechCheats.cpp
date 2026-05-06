#include "TechCheats.h"
#include "../../GUI/Menu.h"
#include "../../SDK/Pal.h"

namespace TechCheats
{
    static constexpr int kForcedTechPoint = 9999;

    void Tick()
    {
        if (Menu::Config.bFreeTechPoint) {
            SDK::SetLocalTechnologyPoint(kForcedTechPoint);
        }
    }
}
