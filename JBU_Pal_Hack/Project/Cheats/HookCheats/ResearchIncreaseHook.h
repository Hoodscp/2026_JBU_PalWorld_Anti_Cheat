#pragma once

// [Track 2 / P2] Quick Research — 연구 진행도 누적량 제곱 증가.
// 토글 ON 시 `addss xmm6, [rdi+0x14]` 직후 `mulss xmm6, xmm6` 를 삽입해
// 매 tick 진행도가 제곱 누적되도록 한다. 호출 몇 회면 한 연구 완료.
// Menu::Config.bHookQuickResearch 토글.
namespace ResearchIncreaseHook
{
    bool Install();
    void Tick();
    void Shutdown();
}
