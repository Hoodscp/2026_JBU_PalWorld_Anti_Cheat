#pragma once
#include <cstdint>

namespace SDK
{
    // 로컬 플레이어의 현재 HP 값을 읽어옵니다. (주소를 찾지 못하면 -1 반환)
    int64_t GetLocalPlayerHealth();

    // 로컬 플레이어의 HP 일정 값으로 덮어씁니다. (일단 단순 포인터 연산 방식)
    bool SetLocalPlayerHealth(int64_t hpVal);
}
