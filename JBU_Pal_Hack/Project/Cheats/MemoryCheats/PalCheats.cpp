#include "PalCheats.h"
#include "../../GUI/Menu.h"
#include "../../SDK/Pal.h"

namespace PalCheats
{
    static constexpr int64_t kForcedHP     = 1'000'000'000;
    static constexpr int64_t kForcedMP     = 1'000'000'000;
    static constexpr float   kForcedSanity = 100.0f;
    static constexpr uint8_t kMaxTalent    = 100;

    void Tick()
    {
        const auto& cfg = Menu::Config;
        const int slot = cfg.SelectedPalSlot;
        if (slot < 0) return;

        // 빈 슬롯이면 어떤 함수도 동작 안 함 → 매 프레임 nullptr 체크 비용만 발생.
        if (SDK::IsPalSlotEmpty(slot)) return;

        if (cfg.bPalGodMode) {
            SDK::SetPalHP(slot, kForcedHP);
        }
        if (cfg.bPalMaxSanity) {
            SDK::SetPalSanity(slot, kForcedSanity);
        }
        if (cfg.bPalInfiniteMP) {
            SDK::SetPalMP(slot, kForcedMP);
        }
        if (cfg.bPalMaxTalents) {
            SDK::SetPalTalents(slot, kMaxTalent, kMaxTalent, kMaxTalent, kMaxTalent);
        }
        if (cfg.bPalForceLevel) {
            int v = cfg.PalTargetLevel;
            if (v < 1)   v = 1;
            if (v > 255) v = 255;
            SDK::SetPalLevel(slot, static_cast<uint8_t>(v));
        }
        if (cfg.bPalForceExp) {
            int64_t v = cfg.PalTargetExp;
            if (v < 0) v = 0;
            SDK::SetPalExp(slot, v);
        }
    }
}
