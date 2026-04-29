#pragma once
#include <cstdint>

// ────────────────────────────────────────────────────────────────────
// Palworld 메모리 오프셋 중앙 관리
//
// 새로운 치트가 추가될 때:
//   1) 본인이 분석한 오프셋을 해당 계층(World / GameState / Pawn / ...)에
//      constexpr 한 줄로 추가
//   2) Pal.cpp의 체인 헬퍼를 재사용해 새 getter 작성
//
// ⚠ 게임 패치 시 Module::GWorld가 가장 먼저 깨집니다. AOB 스캐너로
//    대체될 때까지는 패치 직후 이 값을 갱신해야 합니다 (P1-6).
// ────────────────────────────────────────────────────────────────────

namespace SDK::Offsets
{
    // ───── 모듈 베이스 (Palworld-Win64-Shipping.exe) 기준 ─────
    namespace Module {
        constexpr uintptr_t GWorld = 0x090D3030;
    }

    // ───── UE 객체 그래프 (포인터 체인의 공통 구간) ─────
    namespace World {
        constexpr uintptr_t GameState = 0x158;
    }
    namespace GameState {
        constexpr uintptr_t PlayerArray = 0x2A8; // TArray<APlayerState*>
    }
    namespace PlayerState {
        constexpr uintptr_t PawnPrivate = 0x308;
    }
    namespace Pawn {
        constexpr uintptr_t CharacterParameterComponent = 0x628;
    }
    namespace CharParamComp {
        constexpr uintptr_t IndividualParameter = 0x130;
    }

    // ───── 게임 데이터 (실제 치트 타겟) ─────
    namespace IndividualParam {
        constexpr uintptr_t HP = 0x400;
        // TODO(Team B): MaxHP, Stamina, Hunger, SAN, ...
    }
}
