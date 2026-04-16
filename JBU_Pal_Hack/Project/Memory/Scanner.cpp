#include "Scanner.h"
#include <psapi.h>

namespace Scanner
{
    uintptr_t FindPattern(const char* moduleName, const char* pattern, const char* mask)
    {
        // Simple AOB Scanner logic placeholder
        // ...
        return 0;
    }

    uintptr_t ResolveRVA(uintptr_t address, int offset, int instructionSize)
    {
        if (!address) return 0;
        int32_t relativeAddress = *reinterpret_cast<int32_t*>(address + offset);
        return address + instructionSize + relativeAddress;
    }
}
