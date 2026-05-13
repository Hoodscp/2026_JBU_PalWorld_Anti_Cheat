#include "PalCheats.h"
#include "../../GUI/Menu.h"
#include "../../SDK/Pal.h"

namespace PalCheats
{
    static constexpr int64_t kForcedHP     = 1'000'000'000;
    static constexpr int64_t kForcedMP     = 1'000'000'000;
    static constexpr float   kForcedSanity = 100.0f;
    static constexpr uint8_t kMaxTalent    = 100;

    // Source 에 따라 컨테이너 베이스를 골라 모든 *In() 헬퍼에 전달.
    static uintptr_t ResolveContainer(int source)
    {
        // 0 = Pal Box, 1 = Otomo(파티) Override
        return (source == 1) ? SDK::GetOtomoContainerOverride()
                             : SDK::GetLocalPalContainer();
    }

    void Tick()
    {
        const auto& cfg = Menu::Config;

        // UI 에서 입력한 Otomo 컨테이너 주소를 SDK 에 반영 (0 = 비활성).
        // 입력값이 바뀌었을 때만 다시 등록하면 더 깔끔하지만 매 프레임
        // 같은 값으로 write 해도 성능 영향 없음 → 단순화.
        SDK::SetOtomoContainerOverride(cfg.OtomoContainerAddress);

        const int slot   = cfg.SelectedPalSlot;
        const int source = cfg.PalSource;
        if (slot < 0) return;

        const uintptr_t container = ResolveContainer(source);
        if (!container) return;                              // Box 미동기화 / Party 미등록
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
