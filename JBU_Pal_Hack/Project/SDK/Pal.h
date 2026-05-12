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
    uintptr_t GetLocalInventoryData();
    uintptr_t GetLocalInventoryMultiHelper();
    uintptr_t GetInventoryContainerAt(int containerIndex);
    uintptr_t GetItemSlotAt(int containerIndex, int slotIndex);

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

    // ───── Level / Exp ─────
    int     GetLocalPlayerLevel();
    bool    SetLocalPlayerLevel(uint8_t value);
    int64_t GetLocalPlayerExp();
    bool    SetLocalPlayerExp(int64_t value);

    // ───── Talents (Pal IV — 본인 캐릭터 슬롯에만 적용, 0~100) ─────
    int  GetLocalPlayerTalentHP();
    int  GetLocalPlayerTalentMelee();
    int  GetLocalPlayerTalentShot();
    int  GetLocalPlayerTalentDefense();
    bool SetLocalPlayerTalents(uint8_t hp, uint8_t melee, uint8_t shot, uint8_t def);

    // ───── MP (FFixedPoint64) ─────
    int64_t GetLocalPlayerMP();
    bool    SetLocalPlayerMP(int64_t value);
    int64_t GetLocalPlayerMaxMP();

    // ───── Sanity (float, 0~100) ─────
    float GetLocalPlayerSanity();
    bool  SetLocalPlayerSanity(float value);

    // ───── Friendship / Arena RP (int32) ─────
    int  GetLocalPlayerFriendshipPoint();
    bool SetLocalPlayerFriendshipPoint(int value);
    int  GetLocalPlayerArenaRankPoint();
    bool SetLocalPlayerArenaRankPoint(int value);

    // ───── TechnologyPoint (int32, on UPalTechnologyData) ─────
    int  GetLocalTechnologyPoint();
    bool SetLocalTechnologyPoint(int value);

    // ───── BossTechnologyPoint (int32, on UPalTechnologyData) ─────
    int  GetLocalBossTechnologyPoint();
    bool SetLocalBossTechnologyPoint(int value);

    // ───── Inventory ItemSlot[containerIndex][slotIndex] ─────
    // 체인: PlayerState → InventoryData → MultiHelper → Containers[containerIndex]
    //       → ItemSlotArray[slotIndex] → StackCount
    // 실패 시 -1 / false.
    int  GetItemSlotStackCount(int containerIndex, int slotIndex);
    bool SetItemSlotStackCount(int containerIndex, int slotIndex, int value);

    // ───── 보유 팰 (Pal Box / Storage) ─────
    // 체인: PlayerState → PalStorage → TargetContainer → SlotArray[index]
    //       → ReplicateIndividualParameter → (+0x388 = SaveParameter)
    uintptr_t GetLocalPalStorage();
    uintptr_t GetLocalPalContainer();
    int       GetPalSlotCount();                          // 컨테이너 총 슬롯 수 (빈 슬롯 포함)
    uintptr_t GetPalSlotAt(int slotIndex);                // UPalIndividualCharacterSlot*
    uintptr_t GetPalIndividualParameterAt(int slotIndex); // UPalIndividualCharacterParameter*
    bool      IsPalSlotEmpty(int slotIndex);

    // 보유 팰 슬롯의 SaveParameter 멤버 R/W.
    // 모든 함수는 ReplicateIndividualParameter 가 nullptr 이면 실패(빈 슬롯).
    int     GetPalLevel(int slotIndex);
    bool    SetPalLevel(int slotIndex, uint8_t value);
    int64_t GetPalExp(int slotIndex);
    bool    SetPalExp(int slotIndex, int64_t value);
    int64_t GetPalHP(int slotIndex);
    bool    SetPalHP(int slotIndex, int64_t value);
    int64_t GetPalMaxHP(int slotIndex);
    int     GetPalTalentHP(int slotIndex);
    int     GetPalTalentMelee(int slotIndex);
    int     GetPalTalentShot(int slotIndex);
    int     GetPalTalentDefense(int slotIndex);
    bool    SetPalTalents(int slotIndex, uint8_t hp, uint8_t melee, uint8_t shot, uint8_t def);
    float   GetPalSanity(int slotIndex);
    bool    SetPalSanity(int slotIndex, float value);
    int64_t GetPalMP(int slotIndex);
    bool    SetPalMP(int slotIndex, int64_t value);
}
