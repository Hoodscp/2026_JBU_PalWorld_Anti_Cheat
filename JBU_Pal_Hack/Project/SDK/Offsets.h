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
//
// 출처: Reference/SDK (Dumper-7 기반).
//   - FPalIndividualCharacterSaveParameter 가 UPalIndividualCharacterParameter
//     의 +0x388 위치에 임베드됨 → 절대 오프셋 = 0x388 + 멤버 오프셋.
//     예) HP(+0x78) = 0x388+0x78 = 0x400  ← 기존 값과 일치 확인
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
        constexpr uintptr_t PawnPrivate    = 0x308;
        constexpr uintptr_t TechnologyData = 0x5E8; // UPalTechnologyData*
    }
    namespace Pawn {
        constexpr uintptr_t CharacterParameterComponent = 0x628;
    }
    namespace CharParamComp {
        constexpr uintptr_t IndividualParameter = 0x130;
        constexpr uintptr_t SP                  = 0x328; // FFixedPoint64 (Stamina)
        constexpr uintptr_t ItemContainer       = 0x460; // UPalItemContainer*
    }

    // ───── 게임 데이터 (실제 치트 타겟) ─────
    // SaveParameter 임베드 베이스 = 0x388
    namespace IndividualParam {
        // 모두 SaveParameter(0x388) + 멤버 오프셋 으로 계산된 절대 오프셋.
        constexpr uintptr_t HP                = 0x400; // FFixedPoint64
        constexpr uintptr_t MaxHP             = 0x468; // FFixedPoint64
        constexpr uintptr_t FullStomach       = 0x40C; // float
        constexpr uintptr_t MaxFullStomach    = 0x4F4; // float
        constexpr uintptr_t ShieldHP          = 0x488; // FFixedPoint64
        constexpr uintptr_t ShieldMaxHP       = 0x490; // FFixedPoint64
        constexpr uintptr_t UnusedStatusPoint = 0x4FC; // uint16
    }

    namespace TechnologyData {
        constexpr uintptr_t TechnologyPoint     = 0x150; // int32
        constexpr uintptr_t BossTechnologyPoint = 0x154; // int32
    }

    namespace ItemContainer {
        // TArray<UPalItemSlot*> ItemSlotArray;
        // 헤더 레이아웃: { UPalItemSlot** Data; int32 Num; int32 Max; }
        constexpr uintptr_t ItemSlotArray_Data = 0x70;
        constexpr uintptr_t ItemSlotArray_Num  = 0x78;
    }

    namespace ItemSlot {
        constexpr uintptr_t StackCount = 0x154; // int32
    }

    // ───── 미해결 (런타임 검증 필요) ─────
    // CurrentTemperature 는 UPalBodyTemperatureComponent 의 +0x130 (FPalTemperatureInfo
    // 첫 번째 int32) 에 위치하지만 SDK 덤프에 컴포넌트 포인터가 명시돼 있지 않음.
    // 게임 내에서 Cheat Engine 등으로 컴포넌트 시작주소(BodyTempComp on APalPlayerCharacter)
    // 를 찾은 뒤 PlayerCharacter 베이스 ↔ 컴포넌트 차이를 여기에 등록.
    namespace BodyTemperatureComp {
        // UPalBodyTemperatureComponent 내부 — TemperatureInfo 의 첫 int32 가 현재값.
        constexpr uintptr_t CurrentTemperature = 0x130;
    }
    namespace PlayerCharacter {
        // TODO: 런타임에 BodyTemperatureComponent 포인터를 잡으면 갱신할 것.
        // 0 일 경우 Pal.cpp 의 getter 가 -1 을 반환해 안전하게 NOP 처리됨.
        constexpr uintptr_t BodyTemperatureComponent = 0x0;
    }
}
