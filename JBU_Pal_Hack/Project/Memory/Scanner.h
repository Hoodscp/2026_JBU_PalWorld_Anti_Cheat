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

    // 안전한 메모리 읽기를 위한 간단한 템플릿 함수
    template <typename T>
    T ReadMemory(uintptr_t address) {
        if (!address || IsBadReadPtr((const void*)address, sizeof(T))) return T{};
        return *(T*)address;
    }
}
