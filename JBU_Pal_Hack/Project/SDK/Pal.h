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

    // 음식 부패 진행도(0.0=신선, 1.0=부패 완료). 자동 체인 사용 가능.
    float GetItemSlotCorruption(int containerIndex, int slotIndex);
    bool  SetItemSlotCorruption(int containerIndex, int slotIndex, float value);

    // ───── UE5 TWeakObjectPtr 해소 ─────
    // GUObjectArray(=FUObjectArray 전역)를 통해 FWeakObjectPtr → UObject* 변환.
    // GUObjectArray 가 0 (Offsets::Module::GUObjectArray 미갱신) 이면 0 반환.
    //
    //   ResolveWeakObjectPtr(weakPtrAddr) : 8-byte FWeakObjectPtr 시작 주소
    //                                       (예: slot + 0x190) 를 받아 객체 주소 반환.
    //   ResolveWeakObjectIndex(index, serial=0): index/serial 분리 입력. serial==0
    //                                       이면 직렬번호 검증을 생략.
    uintptr_t ResolveWeakObjectPtr(uintptr_t weakPtrAddr);
    uintptr_t ResolveWeakObjectIndex(int32_t index, int32_t serial = 0);

    // ───── 장비 Dynamic Item Data (Container/Slot 자동 해소) ─────
    // 내부적으로 슬롯의 DynamicItemData TWeakObjectPtr 를 풀어 객체 주소를 얻은
    // 뒤 +0x78 / +0x84 멤버에 R/W. GUObjectArray 미설정 시 모두 -1 / false.
    uintptr_t GetItemSlotDynamicData(int containerIndex, int slotIndex);
    float     GetItemDurability(int containerIndex, int slotIndex);
    float     GetItemMaxDurability(int containerIndex, int slotIndex);
    bool      SetItemDurability(int containerIndex, int slotIndex, float value);
    int       GetItemRemainingBullets(int containerIndex, int slotIndex);
    bool      SetItemRemainingBullets(int containerIndex, int slotIndex, int value);

    // ───── 보유 팰 (Pal Box / Storage / Otomo party / 임의 컨테이너) ─────
    // 모든 헬퍼는 "컨테이너 베이스 = UPalIndividualCharacterContainer*" 위에서 동작.
    // 박스/파티/베이스캠프 어디 컨테이너든 같은 SlotArray@0x80 레이아웃이므로
    // 컨테이너 베이스만 알면 통일된 코드로 변조 가능.
    //
    //  - GetLocalPalContainer()       : PlayerState → PalStorage → TargetContainer
    //                                   (Pal Box, 기본값)
    //  - SetOtomoContainerOverride()  : Otomo(파티) 또는 임의 컨테이너 주소를 등록.
    //                                   설정되어 있으면 *FromOverride() / *AtParty()
    //                                   계열 헬퍼가 이 베이스를 사용.
    //
    // 체인: <Container> → SlotArray[index] → ReplicateIndividualParameter
    //       → (+0x388 = SaveParameter)
    uintptr_t GetLocalPalStorage();
    uintptr_t GetLocalPalContainer();

    // 외부에서 잡아낸 (또는 후속 AOB 후크가 캡처한) Otomo/임의 컨테이너의
    // 절대 주소를 등록한다. 0 을 넣으면 비활성화.
    void      SetOtomoContainerOverride(uintptr_t containerBase);
    uintptr_t GetOtomoContainerOverride();

    // ── Otomo 컨테이너 캡쳐 ──
    // 폐기됨: GUObjectArray 풀스캔 (AutoFindOtomoContainer). 두 스레드에서
    // 동시에 200K+ 객체 IsBadReadPtr 폭주를 일으켜 게임 크래시 → 대체 경로로
    // Cheats/HookCheats/OtomoHook (UPalOtomoHolderComponentBase 멤버 함수에
    // AOB 후크) 가 this+0x110 을 1 회 캡쳐한다. SetOtomoContainerOverride 는
    // 그대로 사용. 후크 설치가 안 됐을 땐 UI 의 수동 hex 입력으로 대체.

    // 컨테이너 베이스에서 슬롯/파라미터 추출 (일반화된 lower-level API).
    int       GetSlotCountIn(uintptr_t container);
    uintptr_t GetSlotInContainer(uintptr_t container, int slotIndex);
    uintptr_t GetIndividualParameterInContainer(uintptr_t container, int slotIndex);

    // Pal Box (기본) 편의 wrapper — 기존 호출 호환.
    int       GetPalSlotCount();
    uintptr_t GetPalSlotAt(int slotIndex);
    uintptr_t GetPalIndividualParameterAt(int slotIndex);
    bool      IsPalSlotEmpty(int slotIndex);

    // Otomo(파티) 편의 wrapper — Override 미설정 시 0/false.
    int       GetPartySlotCount();
    uintptr_t GetPartySlotAt(int slotIndex);
    uintptr_t GetPartyIndividualParameterAt(int slotIndex);
    bool      IsPartySlotEmpty(int slotIndex);

    // 보유 팰 슬롯의 SaveParameter 멤버 R/W. (Pal Box / Party 어디든 같은 함수;
    // container 베이스를 명시적으로 받는 *In() 폼과, 박스/파티 wrapper 형태로 노출.)

    // ── 일반화된 *In() (container-base 명시) ──
    int     GetPalLevelIn(uintptr_t container, int slot);
    bool    SetPalLevelIn(uintptr_t container, int slot, uint8_t value);
    int64_t GetPalExpIn(uintptr_t container, int slot);
    bool    SetPalExpIn(uintptr_t container, int slot, int64_t value);
    int64_t GetPalHPIn(uintptr_t container, int slot);
    bool    SetPalHPIn(uintptr_t container, int slot, int64_t value);
    int64_t GetPalMaxHPIn(uintptr_t container, int slot);
    int     GetPalTalentHPIn(uintptr_t container, int slot);
    int     GetPalTalentMeleeIn(uintptr_t container, int slot);
    int     GetPalTalentShotIn(uintptr_t container, int slot);
    int     GetPalTalentDefenseIn(uintptr_t container, int slot);
    bool    SetPalTalentsIn(uintptr_t container, int slot, uint8_t hp, uint8_t melee, uint8_t shot, uint8_t def);
    float   GetPalSanityIn(uintptr_t container, int slot);
    bool    SetPalSanityIn(uintptr_t container, int slot, float value);
    int64_t GetPalMPIn(uintptr_t container, int slot);
    bool    SetPalMPIn(uintptr_t container, int slot, int64_t value);

    // ── Pal Box wrapper (기존 시그니처 그대로 유지) ──
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
