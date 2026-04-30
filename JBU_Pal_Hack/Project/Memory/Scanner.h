#pragma once
#include <windows.h>
#include <cstdint>

namespace Scanner
{
    // IDA-style 패턴으로 모듈 메모리에서 일치 위치를 찾아 가상주소를 반환.
    // 형식: "48 89 5C 24 ? 57 48 83 EC 20"  (공백 구분, ?/?? 는 와일드카드)
    // 미일치 / 모듈 미발견 시 0 반환.
    uintptr_t FindPattern(const char* moduleName, const char* pattern);

    // 상대 주소(RVA) → 절대 주소(VA) 변환.
    // address: 명령어 시작 주소, offset: displacement 위치, instructionSize: 명령어 전체 길이
    uintptr_t ResolveRVA(uintptr_t address, int offset, int instructionSize);

    // 안전한 메모리 읽기.
    template <typename T>
    T ReadMemory(uintptr_t address) {
        if (!address || IsBadReadPtr((const void*)address, sizeof(T))) return T{};
        return *(T*)address;
    }
}
