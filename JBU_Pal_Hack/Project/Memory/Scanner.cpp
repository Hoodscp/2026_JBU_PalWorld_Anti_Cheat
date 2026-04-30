#include "Scanner.h"
#include <psapi.h>
#include <vector>

namespace Scanner
{
    namespace {
        struct PatternByte {
            uint8_t value;
            bool    wildcard;
        };

        inline int HexVal(char c) {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        }

        std::vector<PatternByte> ParsePattern(const char* pattern)
        {
            std::vector<PatternByte> out;
            for (const char* p = pattern; *p; ) {
                if (*p == ' ' || *p == '\t') { ++p; continue; }
                if (*p == '?') {
                    out.push_back({0, true});
                    ++p;
                    if (*p == '?') ++p; // "??" 도 한 바이트 와일드카드
                    continue;
                }
                int hi = HexVal(*p++);
                if (hi < 0) return {}; // 잘못된 문자 → 빈 결과
                int lo = HexVal(*p);
                if (lo < 0) {
                    out.push_back({(uint8_t)hi, false});
                } else {
                    out.push_back({(uint8_t)((hi << 4) | lo), false});
                    ++p;
                }
            }
            return out;
        }
    }

    uintptr_t FindPattern(const char* moduleName, const char* pattern)
    {
        HMODULE hModule = GetModuleHandleA(moduleName);
        if (!hModule) return 0;

        MODULEINFO modInfo{};
        if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo))) return 0;

        const auto bytes = ParsePattern(pattern);
        if (bytes.empty()) return 0;

        const auto*  base   = reinterpret_cast<const uint8_t*>(hModule);
        const size_t size   = modInfo.SizeOfImage;
        const size_t patLen = bytes.size();
        if (size < patLen) return 0;

        for (size_t i = 0; i <= size - patLen; ++i) {
            bool match = true;
            for (size_t j = 0; j < patLen; ++j) {
                if (!bytes[j].wildcard && base[i + j] != bytes[j].value) {
                    match = false;
                    break;
                }
            }
            if (match) return reinterpret_cast<uintptr_t>(base + i);
        }
        return 0;
    }

    uintptr_t ResolveRVA(uintptr_t address, int offset, int instructionSize)
    {
        if (!address) return 0;
        int32_t relativeAddress = *reinterpret_cast<int32_t*>(address + offset);
        return address + instructionSize + relativeAddress;
    }
}
