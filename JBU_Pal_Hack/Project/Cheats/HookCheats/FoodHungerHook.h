#pragma once

// [Track 2 / P0] Never Get Hungry — subss xmm2,xmm7 (배고픔 차감) 무력화.
// Menu::Config.bHookNoHunger 토글.
namespace FoodHungerHook
{
    bool Install();
    void Tick();
    void Shutdown();
}
