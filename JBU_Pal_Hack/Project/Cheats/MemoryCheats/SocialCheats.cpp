#include "SocialCheats.h"
#include "../../GUI/Menu.h"
#include "../../SDK/Pal.h"

namespace SocialCheats
{
    void Tick()
    {
        const auto& cfg = Menu::Config;

        if (cfg.bForceFriendship) {
            int v = cfg.TargetFriendship;
            if (v < 0) v = 0;
            SDK::SetLocalPlayerFriendshipPoint(v);
        }
        if (cfg.bForceArenaRP) {
            int v = cfg.TargetArenaRP;
            if (v < 0) v = 0;
            SDK::SetLocalPlayerArenaRankPoint(v);
        }
    }
}
