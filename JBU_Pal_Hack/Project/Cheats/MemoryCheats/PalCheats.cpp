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

        const uintptr_t container = SDK::GetLocalPalContainer();
        if (!container) return;                              // PalStorage 미동기화
        if (!SDK::GetIndividualParameterInContainer(container, slot)) return;  // 빈 슬롯

        if (cfg.bPalGodMode)     SDK::SetPalHPIn     (container, slot, kForcedHP);
        if (cfg.bPalMaxSanity)   SDK::SetPalSanityIn (container, slot, kForcedSanity);
        if (cfg.bPalInfiniteMP)  SDK::SetPalMPIn     (container, slot, kForcedMP);
        if (cfg.bPalMaxTalents)  SDK::SetPalTalentsIn(container, slot, kMaxTalent, kMaxTalent, kMaxTalent, kMaxTalent);

        if (cfg.bPalForceLevel) {
            int v = cfg.PalTargetLevel;
            if (v < 1)   v = 1;
            if (v > 255) v = 255;
            SDK::SetPalLevelIn(container, slot, static_cast<uint8_t>(v));
        }
        if (cfg.bPalForceExp) {
            int64_t v = cfg.PalTargetExp;
            if (v < 0) v = 0;
            SDK::SetPalExpIn(container, slot, v);
        }
    }
}
