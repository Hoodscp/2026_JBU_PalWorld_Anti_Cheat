#include "InventoryCheats.h"
#include "../../GUI/Menu.h"
#include "../../SDK/Pal.h"

namespace InventoryCheats
{
    void Tick()
    {
        const auto& cfg = Menu::Config;
        if (!cfg.bForceItemStack) return;

        // 매 프레임 덮어쓰면 게임이 stack count 를 복구해도 즉시 다시 채움.
        SDK::SetItemSlotStackCount(cfg.SelectedContainerIdx, cfg.SelectedSlotIndex, cfg.TargetStackCount);
    }
}
