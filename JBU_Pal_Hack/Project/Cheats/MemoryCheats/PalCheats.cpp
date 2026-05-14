#include "PalCheats.h"
#include "../../GUI/Menu.h"
#include "../../SDK/Pal.h"

namespace PalCheats
{
    static constexpr int64_t kForcedHP     = 1'000'000'000;
    static constexpr int64_t kForcedMP     = 1'000'000'000;
    static constexpr float   kForcedSanity = 100.0f;
    static constexpr uint8_t kMaxTalent    = 100;

    // Source 에 따라 컨테이너 베이스를 결정. Party 모드에서 캐시가 비어
    // 있거나 무효(SlotArray.Num<=0)면 자동 탐색을 1 회 시도한다.
    static uintptr_t ResolveContainer(int source)
    {
        if (source != 1) return SDK::GetLocalPalContainer();

        uintptr_t cached = SDK::GetOtomoContainerOverride();
        if (cached && SDK::GetSlotCountIn(cached) > 0) return cached;

        // 자동 탐색 시도. 성공하면 SetOtomoContainerOverride 까지 내부에서 처리됨.
        return SDK::AutoFindOtomoContainer();
    }

    void Tick()
    {
        const auto& cfg = Menu::Config;

        const int slot   = cfg.SelectedPalSlot;
        const int source = cfg.PalSource;
        if (slot < 0) return;

        const uintptr_t container = ResolveContainer(source);
        if (!container) return;                              // Box 미동기화 / Party 자동 탐색 실패
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
