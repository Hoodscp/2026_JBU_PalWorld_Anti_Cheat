#include "Pal.h"
#include "Offsets.h"
#include "../Memory/Scanner.h"
#include <windows.h>
#include <atomic>

namespace SDK
{
    namespace {
        // 다음 포인터 단계로 안전하게 진행. base가 0이면 0을 전파합니다.
        inline uintptr_t Step(uintptr_t base, uintptr_t offset) {
            if (!base) return 0;
            return Scanner::ReadMemory<uintptr_t>(base + offset);
        }

        // 일반화된 typed write 헬퍼.
        template <typename T>
        inline bool WriteAt(uintptr_t address, T value) {
            if (!address) return false;
            if (IsBadWritePtr((void*)address, sizeof(T))) return false;
            *(T*)address = value;
            return true;
        }
    }

    uintptr_t GetGWorld()
    {
        uintptr_t base = (uintptr_t)GetModuleHandle(NULL);
        return Step(base, Offsets::Module::GWorld);
    }

    uintptr_t GetGameState()
    {
        return Step(GetGWorld(), Offsets::World::GameState);
    }

    uintptr_t GetLocalPlayerState()
    {
        // TArray 헤더 레이아웃: { T* Data; int32 Num; int32 Max; }
        // Data 포인터는 +0, 그 첫 엘리먼트가 로컬 플레이어 상태.
        uintptr_t playerArray = Step(GetGameState(), Offsets::GameState::PlayerArray);
        return Step(playerArray, 0);
    }

    uintptr_t GetLocalPawn()
    {
        return Step(GetLocalPlayerState(), Offsets::PlayerState::PawnPrivate);
    }

    uintptr_t GetCharacterParameterComponent()
    {
        return Step(GetLocalPawn(), Offsets::Pawn::CharacterParameterComponent);
    }

    uintptr_t GetIndividualParameter()
    {
        return Step(GetCharacterParameterComponent(), Offsets::CharParamComp::IndividualParameter);
    }

    uintptr_t GetLocalTechnologyData()
    {
        return Step(GetLocalPlayerState(), Offsets::PlayerState::TechnologyData);
    }

    uintptr_t GetLocalInventoryData()
    {
        return Step(GetLocalPlayerState(), Offsets::PlayerState::InventoryData);
    }

    uintptr_t GetLocalInventoryMultiHelper()
    {
        return Step(GetLocalInventoryData(), Offsets::InventoryData::InventoryMultiHelper);
    }

    uintptr_t GetInventoryContainerAt(int containerIndex)
    {
        if (containerIndex < 0) return 0;
        uintptr_t helper = GetLocalInventoryMultiHelper();
        if (!helper) return 0;

        int32_t num = Scanner::ReadMemory<int32_t>(helper + Offsets::MultiHelper::Containers_Num);
        if (containerIndex >= num) return 0;

        uintptr_t arrData = Scanner::ReadMemory<uintptr_t>(helper + Offsets::MultiHelper::Containers_Data);
        if (!arrData) return 0;

        return Scanner::ReadMemory<uintptr_t>(arrData + (uintptr_t)containerIndex * sizeof(uintptr_t));
    }

    uintptr_t GetItemSlotAt(int containerIndex, int slotIndex)
    {
        if (slotIndex < 0) return 0;
        uintptr_t container = GetInventoryContainerAt(containerIndex);
        if (!container) return 0;

        int32_t num = Scanner::ReadMemory<int32_t>(container + Offsets::ItemContainer::ItemSlotArray_Num);
        if (slotIndex >= num) return 0;

        uintptr_t arrData = Scanner::ReadMemory<uintptr_t>(container + Offsets::ItemContainer::ItemSlotArray_Data);
        if (!arrData) return 0;

        return Scanner::ReadMemory<uintptr_t>(arrData + (uintptr_t)slotIndex * sizeof(uintptr_t));
    }

    // ───── HP ─────
    int64_t GetLocalPlayerHealth()
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return -1;
        return Scanner::ReadMemory<int64_t>(param + Offsets::IndividualParam::HP);
    }

    bool SetLocalPlayerHealth(int64_t hpVal)
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return false;
        return WriteAt<int64_t>(param + Offsets::IndividualParam::HP, hpVal);
    }

    int64_t GetLocalPlayerMaxHP()
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return -1;
        return Scanner::ReadMemory<int64_t>(param + Offsets::IndividualParam::MaxHP);
    }

    // ───── Stamina (SP) ─────
    int64_t GetLocalPlayerStamina()
    {
        uintptr_t comp = GetCharacterParameterComponent();
        if (!comp) return -1;
        return Scanner::ReadMemory<int64_t>(comp + Offsets::CharParamComp::SP);
    }

    bool SetLocalPlayerStamina(int64_t spVal)
    {
        uintptr_t comp = GetCharacterParameterComponent();
        if (!comp) return false;
        return WriteAt<int64_t>(comp + Offsets::CharParamComp::SP, spVal);
    }

    // ───── FullStomach ─────
    float GetLocalPlayerFullStomach()
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return -1.0f;
        return Scanner::ReadMemory<float>(param + Offsets::IndividualParam::FullStomach);
    }

    bool SetLocalPlayerFullStomach(float value)
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return false;
        return WriteAt<float>(param + Offsets::IndividualParam::FullStomach, value);
    }

    float GetLocalPlayerMaxFullStomach()
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return -1.0f;
        return Scanner::ReadMemory<float>(param + Offsets::IndividualParam::MaxFullStomach);
    }

    // ───── ShieldHP ─────
    int64_t GetLocalPlayerShieldHP()
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return -1;
        return Scanner::ReadMemory<int64_t>(param + Offsets::IndividualParam::ShieldHP);
    }

    bool SetLocalPlayerShieldHP(int64_t value)
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return false;
        return WriteAt<int64_t>(param + Offsets::IndividualParam::ShieldHP, value);
    }

    int64_t GetLocalPlayerShieldMaxHP()
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return -1;
        return Scanner::ReadMemory<int64_t>(param + Offsets::IndividualParam::ShieldMaxHP);
    }

    // ───── UnusedStatusPoint (uint16) ─────
    int GetLocalPlayerUnusedStatusPoint()
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return -1;
        return (int)Scanner::ReadMemory<uint16_t>(param + Offsets::IndividualParam::UnusedStatusPoint);
    }

    bool SetLocalPlayerUnusedStatusPoint(uint16_t value)
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return false;
        return WriteAt<uint16_t>(param + Offsets::IndividualParam::UnusedStatusPoint, value);
    }

    // ───── Level / Exp ─────
    int GetLocalPlayerLevel()
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return -1;
        return (int)Scanner::ReadMemory<uint8_t>(param + Offsets::IndividualParam::Level);
    }

    bool SetLocalPlayerLevel(uint8_t value)
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return false;
        return WriteAt<uint8_t>(param + Offsets::IndividualParam::Level, value);
    }

    int64_t GetLocalPlayerExp()
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return -1;
        return Scanner::ReadMemory<int64_t>(param + Offsets::IndividualParam::Exp);
    }

    bool SetLocalPlayerExp(int64_t value)
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return false;
        return WriteAt<int64_t>(param + Offsets::IndividualParam::Exp, value);
    }

    // ───── Talents ─────
    int GetLocalPlayerTalentHP()
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return -1;
        return (int)Scanner::ReadMemory<uint8_t>(param + Offsets::IndividualParam::Talent_HP);
    }

    int GetLocalPlayerTalentMelee()
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return -1;
        return (int)Scanner::ReadMemory<uint8_t>(param + Offsets::IndividualParam::Talent_Melee);
    }

    int GetLocalPlayerTalentShot()
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return -1;
        return (int)Scanner::ReadMemory<uint8_t>(param + Offsets::IndividualParam::Talent_Shot);
    }

    int GetLocalPlayerTalentDefense()
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return -1;
        return (int)Scanner::ReadMemory<uint8_t>(param + Offsets::IndividualParam::Talent_Defense);
    }

    bool SetLocalPlayerTalents(uint8_t hp, uint8_t melee, uint8_t shot, uint8_t def)
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return false;
        bool ok = true;
        ok &= WriteAt<uint8_t>(param + Offsets::IndividualParam::Talent_HP,      hp);
        ok &= WriteAt<uint8_t>(param + Offsets::IndividualParam::Talent_Melee,   melee);
        ok &= WriteAt<uint8_t>(param + Offsets::IndividualParam::Talent_Shot,    shot);
        ok &= WriteAt<uint8_t>(param + Offsets::IndividualParam::Talent_Defense, def);
        return ok;
    }

    // ───── MP ─────
    int64_t GetLocalPlayerMP()
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return -1;
        return Scanner::ReadMemory<int64_t>(param + Offsets::IndividualParam::MP);
    }

    bool SetLocalPlayerMP(int64_t value)
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return false;
        return WriteAt<int64_t>(param + Offsets::IndividualParam::MP, value);
    }

    int64_t GetLocalPlayerMaxMP()
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return -1;
        return Scanner::ReadMemory<int64_t>(param + Offsets::IndividualParam::MaxMP);
    }

    // ───── Sanity ─────
    float GetLocalPlayerSanity()
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return -1.0f;
        return Scanner::ReadMemory<float>(param + Offsets::IndividualParam::SanityValue);
    }

    bool SetLocalPlayerSanity(float value)
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return false;
        return WriteAt<float>(param + Offsets::IndividualParam::SanityValue, value);
    }

    // ───── Friendship / Arena RP ─────
    int GetLocalPlayerFriendshipPoint()
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return -1;
        return Scanner::ReadMemory<int32_t>(param + Offsets::IndividualParam::FriendshipPoint);
    }

    bool SetLocalPlayerFriendshipPoint(int value)
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return false;
        return WriteAt<int32_t>(param + Offsets::IndividualParam::FriendshipPoint, value);
    }

    int GetLocalPlayerArenaRankPoint()
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return -1;
        return Scanner::ReadMemory<int32_t>(param + Offsets::IndividualParam::ArenaRankPoint);
    }

    bool SetLocalPlayerArenaRankPoint(int value)
    {
        uintptr_t param = GetIndividualParameter();
        if (!param) return false;
        return WriteAt<int32_t>(param + Offsets::IndividualParam::ArenaRankPoint, value);
    }

    // ───── TechnologyPoint ─────
    int GetLocalTechnologyPoint()
    {
        uintptr_t tech = GetLocalTechnologyData();
        if (!tech) return -1;
        return Scanner::ReadMemory<int32_t>(tech + Offsets::TechnologyData::TechnologyPoint);
    }

    bool SetLocalTechnologyPoint(int value)
    {
        uintptr_t tech = GetLocalTechnologyData();
        if (!tech) return false;
        return WriteAt<int32_t>(tech + Offsets::TechnologyData::TechnologyPoint, value);
    }

    // ───── BossTechnologyPoint ─────
    int GetLocalBossTechnologyPoint()
    {
        uintptr_t tech = GetLocalTechnologyData();
        if (!tech) return -1;
        return Scanner::ReadMemory<int32_t>(tech + Offsets::TechnologyData::BossTechnologyPoint);
    }

    bool SetLocalBossTechnologyPoint(int value)
    {
        uintptr_t tech = GetLocalTechnologyData();
        if (!tech) return false;
        return WriteAt<int32_t>(tech + Offsets::TechnologyData::BossTechnologyPoint, value);
    }

    // ───── Inventory ─────
    int GetItemSlotStackCount(int containerIndex, int slotIndex)
    {
        uintptr_t slot = GetItemSlotAt(containerIndex, slotIndex);
        if (!slot) return -1;
        return Scanner::ReadMemory<int32_t>(slot + Offsets::ItemSlot::StackCount);
    }

    bool SetItemSlotStackCount(int containerIndex, int slotIndex, int value)
    {
        uintptr_t slot = GetItemSlotAt(containerIndex, slotIndex);
        if (!slot) return false;
        return WriteAt<int32_t>(slot + Offsets::ItemSlot::StackCount, value);
    }

    // ── 음식 부패 진행도 ──
    float GetItemSlotCorruption(int containerIndex, int slotIndex)
    {
        uintptr_t slot = GetItemSlotAt(containerIndex, slotIndex);
        if (!slot) return -1.0f;
        return Scanner::ReadMemory<float>(slot + Offsets::ItemSlot::CorruptionProgressValue);
    }

    bool SetItemSlotCorruption(int containerIndex, int slotIndex, float value)
    {
        uintptr_t slot = GetItemSlotAt(containerIndex, slotIndex);
        if (!slot) return false;
        return WriteAt<float>(slot + Offsets::ItemSlot::CorruptionProgressValue, value);
    }

    // ── UE5 TWeakObjectPtr 해소 ──
    uintptr_t ResolveWeakObjectIndex(int32_t index, int32_t serial)
    {
        if (index < 0) return 0;
        uintptr_t modBase = (uintptr_t)GetModuleHandle(NULL);
        uintptr_t guoa    = modBase + Offsets::Module::GUObjectArray;
        if (!Offsets::Module::GUObjectArray) return 0;

        // FUObjectArray → FChunkedFixedUObjectArray (ObjObjects)
        const uintptr_t objObjects = guoa + Offsets::UObjectArray::ObjObjects;

        int32_t numElements = Scanner::ReadMemory<int32_t>(objObjects + Offsets::UObjectArray::NumElements);
        if (index >= numElements) return 0;

        uintptr_t chunkArray = Scanner::ReadMemory<uintptr_t>(objObjects + Offsets::UObjectArray::ChunkArrayPtr);
        if (!chunkArray) return 0;

        const int chunkIdx   = index / Offsets::UObjectArray::NumPerChunk;
        const int withinIdx  = index % Offsets::UObjectArray::NumPerChunk;

        uintptr_t chunkBase = Scanner::ReadMemory<uintptr_t>(chunkArray + (uintptr_t)chunkIdx * sizeof(uintptr_t));
        if (!chunkBase) return 0;

        const uintptr_t itemAddr = chunkBase + (uintptr_t)withinIdx * Offsets::UObjectArray::Item_Size;

        // Serial 검증은 옵션. serial==0 이면 신뢰하고 건너뜀.
        if (serial != 0) {
            int32_t storedSerial = Scanner::ReadMemory<int32_t>(itemAddr + Offsets::UObjectArray::Item_SerialNumber);
            if (storedSerial != serial) return 0;
        }
        return Scanner::ReadMemory<uintptr_t>(itemAddr + Offsets::UObjectArray::Item_Object);
    }

    uintptr_t ResolveWeakObjectPtr(uintptr_t weakPtrAddr)
    {
        if (!weakPtrAddr) return 0;
        int32_t index  = Scanner::ReadMemory<int32_t>(weakPtrAddr + Offsets::WeakObjectPtr::ObjectIndex);
        int32_t serial = Scanner::ReadMemory<int32_t>(weakPtrAddr + Offsets::WeakObjectPtr::ObjectSerialNumber);
        return ResolveWeakObjectIndex(index, serial);
    }

    // ── 장비 Dynamic Item Data (슬롯 자동 해소) ──
    uintptr_t GetItemSlotDynamicData(int containerIndex, int slotIndex)
    {
        uintptr_t slot = GetItemSlotAt(containerIndex, slotIndex);
        if (!slot) return 0;
        return ResolveWeakObjectPtr(slot + Offsets::ItemSlot::DynamicItemData_WeakPtr);
    }

    float GetItemDurability(int containerIndex, int slotIndex)
    {
        uintptr_t data = GetItemSlotDynamicData(containerIndex, slotIndex);
        if (!data) return -1.0f;
        return Scanner::ReadMemory<float>(data + Offsets::DynamicItemData::Durability);
    }

    float GetItemMaxDurability(int containerIndex, int slotIndex)
    {
        uintptr_t data = GetItemSlotDynamicData(containerIndex, slotIndex);
        if (!data) return -1.0f;
        return Scanner::ReadMemory<float>(data + Offsets::DynamicItemData::MaxDurability);
    }

    bool SetItemDurability(int containerIndex, int slotIndex, float value)
    {
        uintptr_t data = GetItemSlotDynamicData(containerIndex, slotIndex);
        if (!data) return false;
        return WriteAt<float>(data + Offsets::DynamicItemData::Durability, value);
    }

    int GetItemRemainingBullets(int containerIndex, int slotIndex)
    {
        uintptr_t data = GetItemSlotDynamicData(containerIndex, slotIndex);
        if (!data) return -1;
        return Scanner::ReadMemory<int32_t>(data + Offsets::DynamicItemData::RemainingBullets);
    }

    bool SetItemRemainingBullets(int containerIndex, int slotIndex, int value)
    {
        uintptr_t data = GetItemSlotDynamicData(containerIndex, slotIndex);
        if (!data) return false;
        return WriteAt<int32_t>(data + Offsets::DynamicItemData::RemainingBullets, value);
    }

    // ───── 보유 팰 (Box / Otomo / 임의 컨테이너) ─────
    namespace {
        // Otomo(데려다니는 팰) 또는 임의 컨테이너 베이스. 정적 포인터 경로가
        // 없으므로 Cheats/HookCheats/OtomoHook (UPalOtomoHolderComponentBase
        // 멤버 함수 AOB 후크) 가 1 회 캡쳐해 등록한다. 후크 시그니처가 비어
        // 있을 땐 UI 의 수동 hex 입력으로 대체. 0 이면 Otomo 기능 비활성.
        //
        // UI 렌더 스레드와 후크 detour(게임 메인 스레드) 가 동시 접근하므로
        // std::atomic 필수 (torn 64-bit read / write 차단).
        std::atomic<uintptr_t> g_OtomoContainerOverride{0};
    }

    uintptr_t GetLocalPalStorage()
    {
        return Step(GetLocalPlayerState(), Offsets::PlayerState::PalStorage);
    }

    uintptr_t GetLocalPalContainer()
    {
        return Step(GetLocalPalStorage(), Offsets::PalStorage::TargetContainer);
    }

    void SetOtomoContainerOverride(uintptr_t containerBase)
    {
        g_OtomoContainerOverride.store(containerBase);
    }

    uintptr_t GetOtomoContainerOverride()
    {
        return g_OtomoContainerOverride.load();
    }

    // ── 일반화된 컨테이너 → Slot / IndividualParameter ──
    int GetSlotCountIn(uintptr_t container)
    {
        if (!container) return 0;
        return (int)Scanner::ReadMemory<int32_t>(container + Offsets::PalCharContainer::SlotArray_Num);
    }

    uintptr_t GetSlotInContainer(uintptr_t container, int slotIndex)
    {
        if (!container || slotIndex < 0) return 0;
        int32_t num = Scanner::ReadMemory<int32_t>(container + Offsets::PalCharContainer::SlotArray_Num);
        if (slotIndex >= num) return 0;
        uintptr_t arrData = Scanner::ReadMemory<uintptr_t>(container + Offsets::PalCharContainer::SlotArray_Data);
        if (!arrData) return 0;
        return Scanner::ReadMemory<uintptr_t>(arrData + (uintptr_t)slotIndex * sizeof(uintptr_t));
    }

    uintptr_t GetIndividualParameterInContainer(uintptr_t container, int slotIndex)
    {
        uintptr_t slot = GetSlotInContainer(container, slotIndex);
        if (!slot) return 0;
        return Scanner::ReadMemory<uintptr_t>(slot + Offsets::PalCharSlot::ReplicateIndividualParameter);
    }

    // ── Pal Box wrapper ──
    int       GetPalSlotCount()                            { return GetSlotCountIn(GetLocalPalContainer()); }
    uintptr_t GetPalSlotAt(int s)                          { return GetSlotInContainer(GetLocalPalContainer(), s); }
    uintptr_t GetPalIndividualParameterAt(int s)           { return GetIndividualParameterInContainer(GetLocalPalContainer(), s); }
    bool      IsPalSlotEmpty(int s)                        { return GetPalIndividualParameterAt(s) == 0; }

    // ── Otomo(파티) wrapper ──
    int       GetPartySlotCount()                          { return GetSlotCountIn(g_OtomoContainerOverride.load()); }
    uintptr_t GetPartySlotAt(int s)                        { return GetSlotInContainer(g_OtomoContainerOverride.load(), s); }
    uintptr_t GetPartyIndividualParameterAt(int s)         { return GetIndividualParameterInContainer(g_OtomoContainerOverride.load(), s); }
    bool      IsPartySlotEmpty(int s)                      { return GetPartyIndividualParameterAt(s) == 0; }

    // ── per-Pal stat helpers (container-base 명시) ──
    int GetPalLevelIn(uintptr_t c, int s)
    {
        uintptr_t p = GetIndividualParameterInContainer(c, s);
        if (!p) return -1;
        return (int)Scanner::ReadMemory<uint8_t>(p + Offsets::IndividualParam::Level);
    }
    bool SetPalLevelIn(uintptr_t c, int s, uint8_t v)
    {
        uintptr_t p = GetIndividualParameterInContainer(c, s);
        if (!p) return false;
        return WriteAt<uint8_t>(p + Offsets::IndividualParam::Level, v);
    }

    int64_t GetPalExpIn(uintptr_t c, int s)
    {
        uintptr_t p = GetIndividualParameterInContainer(c, s);
        if (!p) return -1;
        return Scanner::ReadMemory<int64_t>(p + Offsets::IndividualParam::Exp);
    }
    bool SetPalExpIn(uintptr_t c, int s, int64_t v)
    {
        uintptr_t p = GetIndividualParameterInContainer(c, s);
        if (!p) return false;
        return WriteAt<int64_t>(p + Offsets::IndividualParam::Exp, v);
    }

    int64_t GetPalHPIn(uintptr_t c, int s)
    {
        uintptr_t p = GetIndividualParameterInContainer(c, s);
        if (!p) return -1;
        return Scanner::ReadMemory<int64_t>(p + Offsets::IndividualParam::HP);
    }
    bool SetPalHPIn(uintptr_t c, int s, int64_t v)
    {
        uintptr_t p = GetIndividualParameterInContainer(c, s);
        if (!p) return false;
        return WriteAt<int64_t>(p + Offsets::IndividualParam::HP, v);
    }

    int64_t GetPalMaxHPIn(uintptr_t c, int s)
    {
        uintptr_t p = GetIndividualParameterInContainer(c, s);
        if (!p) return -1;
        return Scanner::ReadMemory<int64_t>(p + Offsets::IndividualParam::MaxHP);
    }

    int GetPalTalentHPIn(uintptr_t c, int s)
    {
        uintptr_t p = GetIndividualParameterInContainer(c, s);
        if (!p) return -1;
        return (int)Scanner::ReadMemory<uint8_t>(p + Offsets::IndividualParam::Talent_HP);
    }
    int GetPalTalentMeleeIn(uintptr_t c, int s)
    {
        uintptr_t p = GetIndividualParameterInContainer(c, s);
        if (!p) return -1;
        return (int)Scanner::ReadMemory<uint8_t>(p + Offsets::IndividualParam::Talent_Melee);
    }
    int GetPalTalentShotIn(uintptr_t c, int s)
    {
        uintptr_t p = GetIndividualParameterInContainer(c, s);
        if (!p) return -1;
        return (int)Scanner::ReadMemory<uint8_t>(p + Offsets::IndividualParam::Talent_Shot);
    }
    int GetPalTalentDefenseIn(uintptr_t c, int s)
    {
        uintptr_t p = GetIndividualParameterInContainer(c, s);
        if (!p) return -1;
        return (int)Scanner::ReadMemory<uint8_t>(p + Offsets::IndividualParam::Talent_Defense);
    }
    bool SetPalTalentsIn(uintptr_t c, int s, uint8_t hp, uint8_t mel, uint8_t shot, uint8_t def)
    {
        uintptr_t p = GetIndividualParameterInContainer(c, s);
        if (!p) return false;
        bool ok = true;
        ok &= WriteAt<uint8_t>(p + Offsets::IndividualParam::Talent_HP,      hp);
        ok &= WriteAt<uint8_t>(p + Offsets::IndividualParam::Talent_Melee,   mel);
        ok &= WriteAt<uint8_t>(p + Offsets::IndividualParam::Talent_Shot,    shot);
        ok &= WriteAt<uint8_t>(p + Offsets::IndividualParam::Talent_Defense, def);
        return ok;
    }

    float GetPalSanityIn(uintptr_t c, int s)
    {
        uintptr_t p = GetIndividualParameterInContainer(c, s);
        if (!p) return -1.0f;
        return Scanner::ReadMemory<float>(p + Offsets::IndividualParam::SanityValue);
    }
    bool SetPalSanityIn(uintptr_t c, int s, float v)
    {
        uintptr_t p = GetIndividualParameterInContainer(c, s);
        if (!p) return false;
        return WriteAt<float>(p + Offsets::IndividualParam::SanityValue, v);
    }

    int64_t GetPalMPIn(uintptr_t c, int s)
    {
        uintptr_t p = GetIndividualParameterInContainer(c, s);
        if (!p) return -1;
        return Scanner::ReadMemory<int64_t>(p + Offsets::IndividualParam::MP);
    }
    bool SetPalMPIn(uintptr_t c, int s, int64_t v)
    {
        uintptr_t p = GetIndividualParameterInContainer(c, s);
        if (!p) return false;
        return WriteAt<int64_t>(p + Offsets::IndividualParam::MP, v);
    }

    // ── Pal Box wrapper (기존 시그니처 유지) ──
    int     GetPalLevel(int s)                                  { return GetPalLevelIn(GetLocalPalContainer(), s); }
    bool    SetPalLevel(int s, uint8_t v)                       { return SetPalLevelIn(GetLocalPalContainer(), s, v); }
    int64_t GetPalExp(int s)                                    { return GetPalExpIn(GetLocalPalContainer(), s); }
    bool    SetPalExp(int s, int64_t v)                         { return SetPalExpIn(GetLocalPalContainer(), s, v); }
    int64_t GetPalHP(int s)                                     { return GetPalHPIn(GetLocalPalContainer(), s); }
    bool    SetPalHP(int s, int64_t v)                          { return SetPalHPIn(GetLocalPalContainer(), s, v); }
    int64_t GetPalMaxHP(int s)                                  { return GetPalMaxHPIn(GetLocalPalContainer(), s); }
    int     GetPalTalentHP(int s)                               { return GetPalTalentHPIn(GetLocalPalContainer(), s); }
    int     GetPalTalentMelee(int s)                            { return GetPalTalentMeleeIn(GetLocalPalContainer(), s); }
    int     GetPalTalentShot(int s)                             { return GetPalTalentShotIn(GetLocalPalContainer(), s); }
    int     GetPalTalentDefense(int s)                          { return GetPalTalentDefenseIn(GetLocalPalContainer(), s); }
    bool    SetPalTalents(int s, uint8_t hp, uint8_t m, uint8_t sh, uint8_t d) { return SetPalTalentsIn(GetLocalPalContainer(), s, hp, m, sh, d); }
    float   GetPalSanity(int s)                                 { return GetPalSanityIn(GetLocalPalContainer(), s); }
    bool    SetPalSanity(int s, float v)                        { return SetPalSanityIn(GetLocalPalContainer(), s, v); }
    int64_t GetPalMP(int s)                                     { return GetPalMPIn(GetLocalPalContainer(), s); }
    bool    SetPalMP(int s, int64_t v)                          { return SetPalMPIn(GetLocalPalContainer(), s, v); }
}
