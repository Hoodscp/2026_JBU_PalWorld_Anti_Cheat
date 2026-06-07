#pragma once

// [Track 2 / P2] Perfect Pal Stats on Creation — 새 펄 생성 시 모든 IV 100 강제.
// 토글 ON 시 `mov r14d, 0x65` 직전에 `mov edi, 0x64` 를 삽입해, 직후 명령어
// (sub r14d, edi 등 IV 산정 분기)가 항상 0x64=100 을 기준값으로 평가하도록 한다.
// 결과: 새로 생성/포획되는 펄의 HP/근접/원거리/방어 IV 가 전부 100.
// Menu::Config.bHookPerfectPalStats 토글.
//
// ⚠ 적용 범위: 생성/포획 시점에 호출되는 분기 한정. 이미 잡힌 펄에는 영향 없음
//   (그쪽은 Track 1 의 SetPalTalents 가 담당).
namespace PalStatsCreationHook
{
    bool Install();
    void Tick();
    void Shutdown();
}
