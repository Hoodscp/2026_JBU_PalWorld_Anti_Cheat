#pragma once

// [Track 2 / P0] Infinite Ammo — lea/mov 쌍 (탄약 -1 후 write) 9바이트 통째로
// NOP. 발사 시 탄약 카운트가 차감되지 않음.
// Menu::Config.bHookInfAmmo 토글.
namespace AmmoDecreaseHook
{
    bool Install();
    void Tick();
    void Shutdown();
}
