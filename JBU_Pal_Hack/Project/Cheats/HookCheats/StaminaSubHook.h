#pragma once

// [Track 2 / P0] Infinite Stamina — subss xmm0,xmm2 (스태미나 차감 명령) 무력화.
// Menu::Config.bHookInfStamina 토글에 따라 매 프레임 활성/비활성.
namespace StaminaSubHook
{
    bool Install();
    void Tick();
    void Shutdown();
}
