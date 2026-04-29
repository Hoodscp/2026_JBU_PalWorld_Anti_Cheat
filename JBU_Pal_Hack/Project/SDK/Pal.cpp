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

        uintptr_t hpAddress = param + Offsets::IndividualParam::HP;
        if (IsBadWritePtr((void*)hpAddress, sizeof(int64_t))) return false;

        *(int64_t*)hpAddress = hpVal;
        return true;
    }
}
