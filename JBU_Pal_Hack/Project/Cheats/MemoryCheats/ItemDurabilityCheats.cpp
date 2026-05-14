#include "ItemDurabilityCheats.h"
#include "../../GUI/Menu.h"
#include "../../SDK/Pal.h"

namespace ItemDurabilityCheats
{
    void Tick()
    {
        const auto& cfg = Menu::Config;
        const int   ci  = cfg.SelectedContainerIdx;
        const int   si  = cfg.SelectedSlotIndex;

        if (cfg.bForceMaxDurability) {
            // MaxDurability 를 읽어 Durability 에 동기화. 슬롯이 비었거나
            // GUObjectArray 미해소면 -1 / 0 → setter 가 무사 실패.
            float mx = SDK::GetItemMaxDurability(ci, si);
            if (mx > 0.0f) {
                SDK::SetItemDurability(ci, si, mx);
            }
        }
        if (cfg.bForceInfiniteBullets) {
            int v = cfg.TargetBullets;
            if (v < 0) v = 0;
            SDK::SetItemRemainingBullets(ci, si, v);
        }
    }
}
