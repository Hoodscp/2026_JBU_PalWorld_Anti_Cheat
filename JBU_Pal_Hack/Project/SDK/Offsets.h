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
        constexpr uintptr_t GWorld        = 0x090D3030;
        // FUObjectArray 전역. TWeakObjectPtr → UObject* 해소에 사용.
        // 게임 패치마다 깨지므로 AOB 스캐너가 도입되기 전까지는 이 값을
        // 수동으로 갱신해야 한다. 0 이면 weak-ptr 해소가 모두 실패하고
        // Equipment Durability 자동 모드도 비활성 → 메뉴가 hex fallback 안내.
        constexpr uintptr_t GUObjectArray = 0x09197040; // ← placeholder, 빌드별 갱신
    }

    // FUObjectArray / FChunkedFixedUObjectArray 내부 레이아웃 (UE5.x 공통).
    // 일반적으로 한 게임 빌드 내내 안정적이며, GUObjectArray 베이스만 잘
    // 잡으면 그대로 동작한다.
    namespace UObjectArray {
        // FUObjectArray 안에서 FChunkedFixedUObjectArray 시작 오프셋.
        constexpr uintptr_t ObjObjects        = 0x10;
        // FChunkedFixedUObjectArray 안의 멤버들:
        constexpr uintptr_t ChunkArrayPtr     = 0x00; // FUObjectItem** 첫 청크들
        constexpr uintptr_t NumElements       = 0x14; // int32
        // FUObjectItem 레이아웃:
        constexpr uintptr_t Item_Object       = 0x00; // UObject*
        constexpr uintptr_t Item_SerialNumber = 0x14; // int32
        constexpr uintptr_t Item_Size         = 0x18; // sizeof(FUObjectItem)
        // 한 청크당 항목 수.
        constexpr int       NumPerChunk       = 64 * 1024;
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
        constexpr uintptr_t OtomoData      = 0x5C8; // UPalPlayerOtomoData*
        constexpr uintptr_t InventoryData  = 0x5D8; // UPalPlayerInventoryData*
        constexpr uintptr_t PalStorage     = 0x5E0; // UPalPlayerDataPalStorage*
        constexpr uintptr_t TechnologyData = 0x5E8; // UPalTechnologyData*
    }
    namespace PlayerOtomoData {
        // UPalPlayerOtomoData :
        //   +0x28  FPalContainerId  OtomoCharacterContainerId  (16 byte)
        constexpr uintptr_t OtomoCharacterContainerId = 0x28;
    }
    namespace ContainerBase {
        // UPalContainerBase 의 ID 멤버. FPalContainerId 와 동일한 16 byte.
        // UPalIndividualCharacterContainer 도 이를 상속하므로 +0x38 동일.
        constexpr uintptr_t ID      = 0x38;
        constexpr uintptr_t ID_Size = 0x10; // 16 byte
    }
    namespace Pawn {
        constexpr uintptr_t CharacterParameterComponent = 0x628;
    }
    namespace CharParamComp {
        constexpr uintptr_t IndividualParameter = 0x130;
        constexpr uintptr_t SP                  = 0x328; // FFixedPoint64 (Stamina)
    }
    namespace InventoryData {
        constexpr uintptr_t InventoryMultiHelper = 0x190; // UPalItemContainerMultiHelper*
    }
    namespace MultiHelper {
        // TArray<UPalItemContainer*> Containers;
        constexpr uintptr_t Containers_Data = 0x38;
        constexpr uintptr_t Containers_Num  = 0x40;
    }

    // ───── 게임 데이터 (실제 치트 타겟) ─────
    // SaveParameter 임베드 베이스 = 0x388
    namespace IndividualParam {
        // 모두 SaveParameter(0x388) + 멤버 오프셋 으로 계산된 절대 오프셋.
        // (SaveParameter 멤버 오프셋은 FPalIndividualCharacterSaveParameter 덤프 기준)
        constexpr uintptr_t Level             = 0x3A8; // uint8   (SaveParam +0x020)
        constexpr uintptr_t Exp               = 0x3B0; // int64   (SaveParam +0x028)
        constexpr uintptr_t HP                = 0x400; // FFixedPoint64       (+0x078)
        constexpr uintptr_t Talent_HP         = 0x408; // uint8   (SaveParam +0x080)
        constexpr uintptr_t Talent_Melee      = 0x409; // uint8   (SaveParam +0x081)
        constexpr uintptr_t Talent_Shot       = 0x40A; // uint8   (SaveParam +0x082)
        constexpr uintptr_t Talent_Defense    = 0x40B; // uint8   (SaveParam +0x083)
        constexpr uintptr_t FullStomach       = 0x40C; // float               (+0x084)
        constexpr uintptr_t MP                = 0x430; // FFixedPoint64       (+0x0A8)
        constexpr uintptr_t MaxHP             = 0x468; // FFixedPoint64       (+0x0E0)
        constexpr uintptr_t ShieldHP          = 0x488; // FFixedPoint64       (+0x100)
        constexpr uintptr_t ShieldMaxHP       = 0x490; // FFixedPoint64       (+0x108)
        constexpr uintptr_t MaxMP             = 0x4A0; // FFixedPoint64       (+0x118)
        constexpr uintptr_t SanityValue       = 0x4B4; // float               (+0x12C)
        constexpr uintptr_t MaxFullStomach    = 0x4F4; // float               (+0x16C)
        constexpr uintptr_t UnusedStatusPoint = 0x4FC; // uint16              (+0x174)
        constexpr uintptr_t FriendshipPoint   = 0x65C; // int32   (SaveParam +0x2D4)
        constexpr uintptr_t ArenaRankPoint    = 0x670; // int32   (SaveParam +0x2E8)
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
        constexpr uintptr_t StackCount              = 0x154; // int32
        constexpr uintptr_t CorruptionProgressValue = 0x158; // float (음식 부패 진행도, 0=신선)
        // DynamicItemData @ +0x190 은 TWeakObjectPtr — UObjectArray 네임스페이스로
        // GUObjectArray 를 통해 자동 해소. (구버전의 "수동 hex 주소" 경로는 제거됨.)
        constexpr uintptr_t DynamicItemData_WeakPtr = 0x190; // FWeakObjectPtr 시작 (Index@+0, Serial@+4)
    }
    // FWeakObjectPtr 내부 레이아웃.
    namespace WeakObjectPtr {
        constexpr uintptr_t ObjectIndex        = 0x00; // int32
        constexpr uintptr_t ObjectSerialNumber = 0x04; // int32
    }

    // UPalDynamicArmorItemDataBase / UPalDynamicWeaponItemDataBase 공통 레이아웃.
    // 둘 다 UPalDynamicItemDataBase(0x70) 위에 +0x08 패딩 후 멤버 시작.
    namespace DynamicItemData {
        constexpr uintptr_t Durability       = 0x78; // float
        constexpr uintptr_t MaxDurability    = 0x7C; // float
        constexpr uintptr_t RemainingBullets = 0x84; // int32 (Weapon 전용)
    }

    // ───── 보유 팰 (Pal Box / Storage) 체인 ─────
    // PlayerState → PalStorage → TargetContainer → SlotArray[i] → ReplicateIndividualParameter
    namespace PalStorage {
        constexpr uintptr_t TargetContainer = 0x30; // UPalIndividualCharacterContainer*
    }
    namespace PalCharContainer {
        // UPalIndividualCharacterContainer : UPalContainerBase
        // TArray<UPalIndividualCharacterSlot*> SlotArray; @ 0x80
        constexpr uintptr_t SlotArray_Data = 0x80;
        constexpr uintptr_t SlotArray_Num  = 0x88;
    }
    namespace PalCharSlot {
        constexpr uintptr_t Handle                       = 0x60; // UPalIndividualCharacterHandle*
        constexpr uintptr_t ReplicateIndividualParameter = 0x98; // UPalIndividualCharacterParameter*
    }
}
