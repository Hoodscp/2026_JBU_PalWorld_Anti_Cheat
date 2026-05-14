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
    // 있으면 그냥 0 을 반환해 Tick 을 skip — 자동 탐색은 UI(Menu) 측에서만
    // 트리거한다. 과거 여기서 매 Tick 풀스캔을 돌렸더니 두 스레드(MainLoop +
    // Render) 가 동시에 GUObjectArray 200K~ 순회를 반복하며 게임이 크래시.
    static uintptr_t ResolveContainer(int source)
    {
        if (source != 1) return SDK::GetLocalPalContainer();

        uintptr_t cached = SDK::GetOtomoContainerOverride();
        if (cached && SDK::GetSlotCountIn(cached) > 0) return cached;
        return 0;
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
