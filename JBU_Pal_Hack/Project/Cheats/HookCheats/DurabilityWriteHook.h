#pragma once

// [Track 2 / P0] Infinite Durability — movss [rcx+0x10], xmm0 (장비 내구도 write)
// 무력화. 장비/무기 내구도가 깎이지 않음.
// Menu::Config.bHookInfDurability 토글.
namespace DurabilityWriteHook
{
    bool Install();
    void Tick();
    void Shutdown();
}
