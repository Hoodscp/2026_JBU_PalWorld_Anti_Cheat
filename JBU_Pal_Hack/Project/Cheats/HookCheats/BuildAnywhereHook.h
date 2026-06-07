#pragma once

// [Track 2 / P2] Can Build Anything — buildableTest 조건 분기 강제.
// 토글 ON 시 `cmp byte ptr [rcx+0xE4], sil` 직전에 같은 위치에 sil 자체를
// write 해 cmp 결과를 항상 ZF=1 로 만든다 → 건축 가용성 검사 전부 통과.
// Menu::Config.bHookBuildAnywhere 토글.
namespace BuildAnywhereHook
{
    bool Install();
    void Tick();
    void Shutdown();
}
