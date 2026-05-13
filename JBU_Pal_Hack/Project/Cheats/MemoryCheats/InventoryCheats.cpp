#include "InventoryCheats.h"
#include "../../GUI/Menu.h"
#include "../../SDK/Pal.h"

namespace InventoryCheats
{
    void Tick()
    {
        const auto& cfg = Menu::Config;

        // ── StackCount 강제 ──
        if (cfg.bForceItemStack) {
            SDK::SetItemSlotStackCount(cfg.SelectedContainerIdx, cfg.SelectedSlotIndex, cfg.TargetStackCount);
        }

        // ── 음식 신선도 (CorruptionProgressValue = 0) ──
        // 단일 슬롯 모드: 위와 동일한 Container/Slot 좌표에 매 프레임 0 강제.
        if (cfg.bFreezeFoodFreshness) {
            SDK::SetItemSlotCorruption(cfg.SelectedContainerIdx, cfg.SelectedSlotIndex, 0.0f);
        }

        // ── 인벤토리 전체 음식 신선도 동결 ──
        // 컨테이너 0(메인 인벤토리)의 모든 슬롯에 매 프레임 0 강제.
        // 컨테이너 0 의 슬롯 수가 보통 32~40 이라 비용은 미미.
        if (cfg.bFreezeAllFoodFreshness) {
            // 슬롯 0~63 까지만 시도 — 빈 슬롯/out-of-range 는 자동으로 false 반환.
            for (int s = 0; s < 64; ++s) {
                SDK::SetItemSlotCorruption(cfg.SelectedContainerIdx, s, 0.0f);
            }
        }
    }
}
