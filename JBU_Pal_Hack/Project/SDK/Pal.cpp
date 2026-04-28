#include "Pal.h"
#include "../Memory/Scanner.h"
#include <windows.h>

namespace SDK
{
    // Caching base addresses temporarily (in advanced hacks, GWorld is found via sig-scan)
    uintptr_t GetIndividualParameter()
    {
        uintptr_t baseAddress = (uintptr_t)GetModuleHandle(NULL);
        if (!baseAddress) return 0;

        uintptr_t gWorld = Scanner::ReadMemory<uintptr_t>(baseAddress + 0x090D3030);
        if (!gWorld) return 0;
        
        uintptr_t gameState = Scanner::ReadMemory<uintptr_t>(gWorld + 0x158);
        if (!gameState) return 0;
        
        uintptr_t playerArray = Scanner::ReadMemory<uintptr_t>(gameState + 0x2A8);
        if (!playerArray) return 0;
        
        uintptr_t playerState = Scanner::ReadMemory<uintptr_t>(playerArray + 0x0);
        if (!playerState) return 0;
        
        uintptr_t pawnPrivate = Scanner::ReadMemory<uintptr_t>(playerState + 0x308);
        if (!pawnPrivate) return 0;
        
        uintptr_t charParamComp = Scanner::ReadMemory<uintptr_t>(pawnPrivate + 0x628);
        if (!charParamComp) return 0;
        
        uintptr_t individualParam = Scanner::ReadMemory<uintptr_t>(charParamComp + 0x130);
        return individualParam;
    }

    int64_t GetLocalPlayerHealth()
    {
        uintptr_t individualParam = GetIndividualParameter();
        if (individualParam) {
            // 0x388 + 0x78 = 0x400
            return Scanner::ReadMemory<int64_t>(individualParam + 0x400);
        }
        return -1;
    }

    bool SetLocalPlayerHealth(int64_t hpVal)
    {
        uintptr_t individualParam = GetIndividualParameter();
        if (individualParam) {
            // 주소가 유효한지 확인 후 쓰기
            uintptr_t hpAddress = individualParam + 0x400;
            if (!IsBadWritePtr((void*)hpAddress, sizeof(int64_t))) {
                *(int64_t*)hpAddress = hpVal;
                return true;
            }
        }
        return false;
    }
}
