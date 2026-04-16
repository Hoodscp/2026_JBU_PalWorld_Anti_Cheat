#pragma once
#include <windows.h>
#include <cstdint>
#include <vector>

namespace Scanner
{
    // Find pattern signature in memory
    uintptr_t FindPattern(const char* moduleName, const char* pattern, const char* mask);
    
    // Resolve relative virtual addresses (RVA to VA)
    uintptr_t ResolveRVA(uintptr_t address, int offset, int instructionSize);
}
