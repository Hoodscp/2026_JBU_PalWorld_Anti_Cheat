#include "Pal.h"
#include "Offsets.h"
#include "../Memory/Scanner.h"
#include <windows.h>

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
}
