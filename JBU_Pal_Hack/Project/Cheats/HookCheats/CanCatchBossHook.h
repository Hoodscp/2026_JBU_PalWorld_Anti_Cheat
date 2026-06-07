#pragma once

// [Track 2 / P2] Can Catch Boss Pal — 보스/머천트 차단 플래그 강제 0.
// 토글 ON 시 보스 캐릭터 데이터의 `[rax+0x598]` 차단 플래그를 mov 직후·cmp 직전에
// 0 으로 강제 write → 보스/머천트도 일반 펄과 동일하게 페일 스피어로 포획 가능.
// Menu::Config.bHookCanCatchBoss 토글.
namespace CanCatchBossHook
{
    bool Install();
    void Tick();
    void Shutdown();
}
