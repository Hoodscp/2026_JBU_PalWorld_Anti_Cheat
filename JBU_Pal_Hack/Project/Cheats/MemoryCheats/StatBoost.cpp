#include "StatBoost.h"
#include "../../GUI/Menu.h"
#include "../../SDK/Pal.h"

namespace StatBoost
{
    static constexpr float    kForcedSanity = 100.0f;          // SanityValue 캡
    static constexpr int64_t  kForcedMP     = 1'000'000'000;   // FFixedPoint64
    static constexpr uint8_t  kMaxTalent    = 100;             // Talent_HP/Melee/Shot/Defense 캡

    void Tick()
    {
        const auto& cfg = Menu::Config;

        if (cfg.bMaxSanity) {
            SDK::SetLocalPlayerSanity(kForcedSanity);
        }
        if (cfg.bInfiniteMP) {
            SDK::SetLocalPlayerMP(kForcedMP);
        }
        if (cfg.bMaxTalents) {
            // Pal IV(잠재력) 4종 모두 100으로 고정. 본인 캐릭터 슬롯 SaveParameter 에 적용됨.
            SDK::SetLocalPlayerTalents(kMaxTalent, kMaxTalent, kMaxTalent, kMaxTalent);
        }
        if (cfg.bForceLevel) {
            // uint8 wrap 방지. Level 은 1~255 범위에서 의미 있음.
            int v = cfg.TargetLevel;
            if (v < 1)   v = 1;
            if (v > 255) v = 255;
            SDK::SetLocalPlayerLevel(static_cast<uint8_t>(v));
        }
        if (cfg.bForceExp) {
            int64_t v = cfg.TargetExp;
            if (v < 0) v = 0;
            SDK::SetLocalPlayerExp(v);
        }
    }
}
