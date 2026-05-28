#pragma once

// [Track 2 / P0] Freeze Food Counter — movss [rbx+0x158], xmm0 (조리/숙성 타이머
// write) 무력화. 음식이 시간 경과로 변하지 않게 됨.
// Menu::Config.bHookFreezeFoodTimer 토글.
namespace FoodTimerHook
{
    bool Install();
    void Tick();
    void Shutdown();
}
