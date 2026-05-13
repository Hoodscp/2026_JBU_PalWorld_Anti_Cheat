#include "ItemDurabilityCheats.h"
#include "../../GUI/Menu.h"
#include "../../SDK/Pal.h"

namespace ItemDurabilityCheats
{
    void Tick()
    {
        const auto& cfg = Menu::Config;
        const uintptr_t addr = cfg.DynamicItemDataAddress;
        if (!addr) return; // 미등록 → 비활성

        if (cfg.bForceMaxDurability) {
            // MaxDurability 를 읽어 그 값으로 Durability 를 채움.
            // (단순히 큰 상수를 쓰면 일부 UI 표시가 깨질 수 있어 Max 와 동기화.)
            float mx = SDK::GetDynamicMaxDurability(addr);
            if (mx > 0.0f) {
                SDK::SetDynamicDurability(addr, mx);
            }
        }
        if (cfg.bForceInfiniteBullets) {
            int v = cfg.TargetBullets;
            if (v < 0) v = 0;
            SDK::SetDynamicRemainingBullets(addr, v);
        }
    }
}
