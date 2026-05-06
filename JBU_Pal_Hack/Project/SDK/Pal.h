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
    uintptr_t GetLocalTechnologyData();
    uintptr_t GetLocalItemContainer();
    uintptr_t GetItemSlotAt(int index);

    // ───── HP ─────
    int64_t GetLocalPlayerHealth();
    bool    SetLocalPlayerHealth(int64_t hpVal);
    int64_t GetLocalPlayerMaxHP();

    // ───── Stamina (SP, FFixedPoint64) ─────
    int64_t GetLocalPlayerStamina();
    bool    SetLocalPlayerStamina(int64_t spVal);

    // ───── FullStomach (포만감, float) ─────
    float GetLocalPlayerFullStomach();
    bool  SetLocalPlayerFullStomach(float value);
    float GetLocalPlayerMaxFullStomach();

    // ───── ShieldHP (FFixedPoint64) ─────
    int64_t GetLocalPlayerShieldHP();
    bool    SetLocalPlayerShieldHP(int64_t value);
    int64_t GetLocalPlayerShieldMaxHP();

    // ───── UnusedStatusPoint (uint16) ─────
    int     GetLocalPlayerUnusedStatusPoint();
    bool    SetLocalPlayerUnusedStatusPoint(uint16_t value);

    // ───── TechnologyPoint (int32, on UPalTechnologyData) ─────
    int  GetLocalTechnologyPoint();
    bool SetLocalTechnologyPoint(int value);

    // ───── Inventory ItemSlot[index] ─────
    // 첫 슬롯의 StackCount 를 읽거나 덮어씁니다. 실패 시 -1 / false.
    int  GetItemSlotStackCount(int index);
    bool SetItemSlotStackCount(int index, int value);

    // ───── CurrentTemperature (int32, on UPalBodyTemperatureComponent) ─────
    // BodyTemperatureComponent 오프셋이 0(미해결)이면 -1 반환.
    int  GetLocalPlayerCurrentTemperature();
    bool SetLocalPlayerCurrentTemperature(int value);
}
