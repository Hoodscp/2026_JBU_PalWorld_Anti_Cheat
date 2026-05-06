#include "PlayerCheats.h"
#include "../../GUI/Menu.h"
#include "../../SDK/Pal.h"

namespace PlayerCheats
{
    // FFixedPoint64 는 정수형 고정소수점이므로 큰 값을 그대로 채워도 안전.
    static constexpr int64_t kForcedHP        = 1'000'000'000;
    static constexpr int64_t kForcedStamina   = 1'000'000'000;
    static constexpr int64_t kForcedShieldHP  = 1'000'000'000;
    static constexpr float   kForcedStomach   = 100000.0f;
    static constexpr uint16_t kFreeStatusPool = 999;

    void Tick()
    {
        const auto& cfg = Menu::Config;

        if (cfg.bInfiniteStamina) {
            SDK::SetLocalPlayerStamina(kForcedStamina);
        }
        if (cfg.bNoHunger) {
            // FullStomach 가 0 으로 떨어지면 HP 가 깎이므로 매 프레임 채움.
            SDK::SetLocalPlayerFullStomach(kForcedStomach);
        }
        if (cfg.bShieldBoost) {
            SDK::SetLocalPlayerShieldHP(kForcedShieldHP);
        }
        if (cfg.bFreeStatusPoint) {
            // 한 번 분배해도 다음 프레임에 다시 999 로 복구되어 무한 분배 가능.
            SDK::SetLocalPlayerUnusedStatusPoint(kFreeStatusPool);
        }
    }
}
