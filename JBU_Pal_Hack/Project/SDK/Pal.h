#pragma once
#include <cstdint>

namespace SDK
{
    // ───── 포인터 체인 단계별 헬퍼 ─────
    // 각 함수는 한 단계만 수행하므로 새 치트는 본인의 분기점부터
    // 재사용할 수 있습니다 (예: 시간/날씨 치트 → GetGameState까지만,
    // 이동속도 치트 → GetLocalPawn까지만 사용). 모두 실패 시 0 반환.
    uintptr_t GetGWorld();
    uintptr_t GetGameState();
    uintptr_t GetLocalPlayerState();
    uintptr_t GetLocalPawn();
    uintptr_t GetCharacterParameterComponent();
    uintptr_t GetIndividualParameter();

    // ───── 고수준 게임 데이터 접근 ─────
    // 로컬 플레이어의 현재 HP 값을 읽어옵니다. (실패 시 -1)
    int64_t GetLocalPlayerHealth();

    // 로컬 플레이어의 HP를 일정 값으로 덮어씁니다.
    bool SetLocalPlayerHealth(int64_t hpVal);
}
