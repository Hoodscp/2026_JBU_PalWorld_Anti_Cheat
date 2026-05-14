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
        // Otomo(데려다니는 팰) 또는 임의 컨테이너 베이스. SDK 덤프상 직접 멤버
        // 오프셋이 없으므로 외부(Cheat Engine 사용자 입력 또는 향후 AOB 후크)
        // 에서 한 번 잡아 등록한다. 0 이면 Otomo 기능 자체가 비활성.
        // UI 렌더 스레드와 MainLoop 가 동시 접근하므로 std::atomic 필수
        // (torn 64-bit read / write 차단).
        std::atomic<uintptr_t> g_OtomoContainerOverride{0};

        // AutoFindOtomoContainer 의 동시 실행 방지 + 실패 쿨다운.
        //  - g_otomoScanInProgress: false→true CAS 에 실패한 동시 호출자는
        //    즉시 0 반환 → 두 스레드가 동시에 200K~ 풀스캔 도는 사고 차단.
        //  - g_otomoLastScanTick   : 마지막 시도 시각(GetTickCount). 이로부터
        //    kScanCooldownMs 미만이면 force=false 호출은 즉시 0 반환 → 매
        //    프레임 폭주 차단. 사용자가 누른 Re-scan 만 force=true 로 우회.
        std::atomic<bool>  g_otomoScanInProgress{false};
        std::atomic<DWORD> g_otomoLastScanTick{0};
        constexpr DWORD    kScanCooldownMs = 2000;
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

    // ── Otomo 컨테이너 자동 탐색 ──
    // 동작 흐름:
    //   1) PlayerState → OtomoData(+0x5C8) → OtomoCharacterContainerId(+0x28, 16B)
    //   2) GUObjectArray 순회: 각 객체 +0x38 (UPalContainerBase::ID) 와 16B 비교
    //   3) 후보 발견 시 sanity check: SlotArray.Num(+0x88) 이 1~256 범위 → 합격
    //   4) Override 캐시에 등록 후 주소 반환
    //
    // 비용: 보통 5만~20만 객체. 1 회 호출만 비싸고 결과는 캐시되므로 매 프레임
    //       호출하지 않는다. UI 토글/버튼에서만 호출하고, 매 Tick 폴백 경로
    //       에서는 호출 금지 (PalCheats::Tick 은 캐시만 사용).
    //
    // 폭주 방지:
    //   - g_otomoScanInProgress 로 동시 실행 차단 (UI/Tick 두 스레드 race 보호).
    //   - g_otomoLastScanTick 으로 실패 후 kScanCooldownMs(2 초) 동안 재시도 차단.
    //   - GUObjectArray 가 placeholder 라 numElements 가 비정상이면 (>500K)
    //     스캔 자체를 거부 → 잘못된 베이스 주소에서 2M 회 IsBadReadPtr 폭주 차단.
    //   - force=true 는 쿨다운만 우회 (in-progress 가드는 항상 적용).
    uintptr_t AutoFindOtomoContainer(bool force)
    {
        if (!Offsets::Module::GUObjectArray) return 0;

        // 동시 실행 차단: 다른 스레드가 이미 스캔 중이면 즉시 포기.
        bool expected = false;
        if (!g_otomoScanInProgress.compare_exchange_strong(expected, true)) {
            return 0;
        }
        // 어떤 경로(early return / 정상 종료)로 빠져나가든 in-progress 해제.
        struct InProgressGuard {
            std::atomic<bool>& flag;
            ~InProgressGuard() { flag.store(false); }
        } guard{g_otomoScanInProgress};

        // 쿨다운: 마지막 시도 후 2 초 이내라면 force=false 호출은 즉시 0.
        // 시작 시점에 시각을 기록 → 실패/성공 무관하게 다음 자동 호출은 2 초 후.
        const DWORD now  = GetTickCount();
        const DWORD last = g_otomoLastScanTick.load();
        if (!force && last != 0 && (now - last) < kScanCooldownMs) return 0;
        g_otomoLastScanTick.store(now);

        // 1) Otomo ContainerId 16B 읽기
        uintptr_t ps = GetLocalPlayerState();
        if (!ps) return 0;
        uintptr_t otomoData = Scanner::ReadMemory<uintptr_t>(ps + Offsets::PlayerState::OtomoData);
        if (!otomoData) return 0;

        const uintptr_t idAddr = otomoData + Offsets::PlayerOtomoData::OtomoCharacterContainerId;
        if (IsBadReadPtr((const void*)idAddr, Offsets::ContainerBase::ID_Size)) return 0;
        uint64_t targetIdLo = *(const uint64_t*)(idAddr + 0x0);
        uint64_t targetIdHi = *(const uint64_t*)(idAddr + 0x8);
        // 전부 0 인 ID 는 false-positive 위험 너무 큼 → 거부.
        if (targetIdLo == 0 && targetIdHi == 0) return 0;

        // 2) GUObjectArray 순회 준비
        uintptr_t modBase    = (uintptr_t)GetModuleHandle(NULL);
        uintptr_t objObjects = modBase + Offsets::Module::GUObjectArray + Offsets::UObjectArray::ObjObjects;
        int32_t   numElements = Scanner::ReadMemory<int32_t>(objObjects + Offsets::UObjectArray::NumElements);
        uintptr_t chunkArray  = Scanner::ReadMemory<uintptr_t>(objObjects + Offsets::UObjectArray::ChunkArrayPtr);
        if (!chunkArray || numElements <= 0) return 0;
        // 비정상적으로 큰 값이면 GUObjectArray 베이스가 이번 빌드와 안 맞는다고
        // 판단하고 스캔 자체를 거부. 정상 UE5 게임은 ~20 만, 빌드 차이 여유까지
        // 50 만으로 클램프. 이전엔 200 만까지 강행 → 잘못된 청크 주소들을 줄줄이
        // IsBadReadPtr 로 두드리면서 게임 크래시.
        if (numElements > 500'000) return 0;

        for (int32_t i = 0; i < numElements; ++i)
        {
            const int chunkIdx  = i / Offsets::UObjectArray::NumPerChunk;
            const int withinIdx = i % Offsets::UObjectArray::NumPerChunk;

            uintptr_t chunkBase = Scanner::ReadMemory<uintptr_t>(chunkArray + (uintptr_t)chunkIdx * sizeof(uintptr_t));
            if (!chunkBase) continue;

            uintptr_t obj = Scanner::ReadMemory<uintptr_t>(
                chunkBase + (uintptr_t)withinIdx * Offsets::UObjectArray::Item_Size);
            if (!obj) continue;

            // ID 비교 (16 byte). IsBadReadPtr 한 번으로 안전 확인 후 8+8.
            const uintptr_t objIdAddr = obj + Offsets::ContainerBase::ID;
            if (IsBadReadPtr((const void*)objIdAddr, Offsets::ContainerBase::ID_Size)) continue;
            if (*(const uint64_t*)(objIdAddr + 0x0) != targetIdLo) continue;
            if (*(const uint64_t*)(objIdAddr + 0x8) != targetIdHi) continue;

            // 후보 발견. SlotArray.Num sanity check.
            int32_t num = Scanner::ReadMemory<int32_t>(obj + Offsets::PalCharContainer::SlotArray_Num);
            if (num <= 0 || num > 256) continue;
            uintptr_t arrData = Scanner::ReadMemory<uintptr_t>(obj + Offsets::PalCharContainer::SlotArray_Data);
            if (!arrData) continue;

            SetOtomoContainerOverride(obj);
            return obj;
        }
        return 0;
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
