#include "MidHook.h"
#include "Scanner.h"
#include <windows.h>
#include <iostream>
#include <cstring>

namespace MidHook
{
    // ────────────────────────────────────────────────────────────
    // 32-bit 상대 jmp 가 도달 가능하도록 anchor ±2GB 안에서
    // VirtualAlloc 시도. 64KB granularity 단위 양방향 스캔.
    // (TemperatureHook.cpp 의 AllocateNear 와 동일 알고리즘)
    // ────────────────────────────────────────────────────────────
    static void* AllocateNear(uintptr_t anchor, size_t size)
    {
        SYSTEM_INFO si{};
        GetSystemInfo(&si);
        const uintptr_t granularity = si.dwAllocationGranularity;
        const uintptr_t lo = (anchor > 0x70000000ULL) ? (anchor - 0x70000000ULL) : 0;
        const uintptr_t hi = anchor + 0x70000000ULL;

        uintptr_t probe = (anchor & ~(granularity - 1));
        while (probe > lo) {
            void* p = VirtualAlloc((void*)probe, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
            if (p) return p;
            probe -= granularity;
        }
        probe = (anchor & ~(granularity - 1)) + granularity;
        while (probe < hi) {
            void* p = VirtualAlloc((void*)probe, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
            if (p) return p;
            probe += granularity;
        }
        return nullptr;
    }

    // ────────────────────────────────────────────────────────────
    // 트램폴린 사이즈 계산
    //   preamble (push rax + mov al,[rip+disp] + test + pop rax + je)
    //   + bodyEnabled + (jmp return)
    //   + bodyDisabled + (jmp return)
    //   + toggle byte
    // ────────────────────────────────────────────────────────────
    static constexpr size_t kPreambleLen = 1 /*push rax*/ + 7 /*mov al,[rip+d32]*/
                                         + 2 /*test al,al*/ + 1 /*pop rax*/
                                         + 2 /*je rel8*/;
    static constexpr size_t kJmpLen      = 5; // E9 + disp32

    Handle* Install(const char* hookName,
                    const char* moduleName,
                    const char* signature,
                    size_t      patchLen,
                    const uint8_t* bodyEnabled,  size_t bodyEnabledLen,
                    const uint8_t* bodyDisabled, size_t bodyDisabledLen)
    {
        if (!hookName) hookName = "MidHook";

        if (!signature || !*signature) {
            std::cout << "[*] " << hookName << ": signature empty, skip install.\n";
            return nullptr;
        }
        if (patchLen < kJmpLen || patchLen > sizeof(Handle{}.originalBytes)) {
            std::cout << "[-] " << hookName << ": invalid patchLen (" << patchLen << ").\n";
            return nullptr;
        }

        const uintptr_t addr = Scanner::FindPattern(moduleName, signature);
        if (!addr) {
            std::cout << "[-] " << hookName << ": pattern not found in " << moduleName << ".\n";
            return nullptr;
        }
        std::cout << "[+] " << hookName << ": target @ 0x" << std::hex << addr << std::dec << "\n";

        // 트램폴린 레이아웃 계산
        const size_t enLen  = bodyEnabledLen;
        const size_t disLen = bodyDisabledLen;
        // doDisabled 까지 거리 = bodyEnabled + jmp return 길이 = enLen + kJmpLen
        // (je rel8 = -126..+127). doDisabled 가 이 범위 안에 와야 함.
        const size_t jeOffsetToDisabled = enLen + kJmpLen;
        if (jeOffsetToDisabled > 127) {
            std::cout << "[-] " << hookName << ": bodyEnabled too large for je rel8 ("
                      << jeOffsetToDisabled << " bytes).\n";
            return nullptr;
        }

        const size_t trampolineLen = kPreambleLen + enLen + kJmpLen + disLen + kJmpLen + 1 /*toggle*/;

        uint8_t* tramp = (uint8_t*)AllocateNear(addr, trampolineLen);
        if (!tramp) {
            std::cout << "[-] " << hookName << ": AllocateNear failed.\n";
            return nullptr;
        }

        // ±2GB 도달 가능성 검증
        const intptr_t delta = (intptr_t)tramp - (intptr_t)addr;
        if (delta > (intptr_t)0x7FFFFFF0 || delta < -(intptr_t)0x7FFFFFF0) {
            std::cout << "[-] " << hookName << ": trampoline out of jmp range (delta=0x"
                      << std::hex << delta << std::dec << ").\n";
            VirtualFree(tramp, 0, MEM_RELEASE);
            return nullptr;
        }

        Handle* h = new Handle();
        h->targetAddr    = addr;
        h->patchLen      = patchLen;
        h->trampoline    = tramp;
        h->trampolineLen = trampolineLen;
        h->toggleOffset  = trampolineLen - 1;

        // 원본 백업
        std::memcpy(h->originalBytes, (const void*)addr, patchLen);

        // ── 트램폴린 작성 ──
        uint8_t* p = tramp;
        const uintptr_t returnAddr = addr + patchLen;

        // 0x00: push rax
        *p++ = 0x50;

        // 0x01: mov al, byte ptr [rip + disp32]
        *p++ = 0x8A;
        *p++ = 0x05;
        {
            // 다음 RIP = p + 4 (이 명령어 끝). 그 시점 기준으로 toggle 까지 거리.
            const uintptr_t nextRip = (uintptr_t)(p + 4);
            const int32_t disp = (int32_t)((intptr_t)(tramp + h->toggleOffset) - (intptr_t)nextRip);
            std::memcpy(p, &disp, sizeof(disp));
            p += 4;
        }

        // 0x08: test al, al
        *p++ = 0x84;
        *p++ = 0xC0;

        // 0x0A: pop rax (flags 보존)
        *p++ = 0x58;

        // 0x0B: je rel8 (Disabled 로 점프)
        *p++ = 0x74;
        *p++ = (uint8_t)jeOffsetToDisabled;
        // ── preamble 끝 (0x0D) ──

        // <bodyEnabled bytes>
        if (enLen) {
            std::memcpy(p, bodyEnabled, enLen);
            p += enLen;
        }

        // jmp returnAddr  (toggle ON 분기 끝)
        *p++ = 0xE9;
        {
            const uintptr_t nextRip = (uintptr_t)(p + 4);
            const int32_t disp = (int32_t)((intptr_t)returnAddr - (intptr_t)nextRip);
            std::memcpy(p, &disp, sizeof(disp));
            p += 4;
        }
        // 여기가 Disabled 라벨 위치 — je 가 이리로 점프함.

        // <bodyDisabled bytes>
        if (disLen) {
            std::memcpy(p, bodyDisabled, disLen);
            p += disLen;
        }

        // jmp returnAddr  (toggle OFF 분기 끝)
        *p++ = 0xE9;
        {
            const uintptr_t nextRip = (uintptr_t)(p + 4);
            const int32_t disp = (int32_t)((intptr_t)returnAddr - (intptr_t)nextRip);
            std::memcpy(p, &disp, sizeof(disp));
            p += 4;
        }

        // toggle byte (default OFF)
        *p++ = 0x00;

        FlushInstructionCache(GetCurrentProcess(), tramp, trampolineLen);

        // ── 패치 사이트에 jmp <tramp> + NOP padding ──
        DWORD oldProt = 0;
        if (!VirtualProtect((void*)addr, patchLen, PAGE_EXECUTE_READWRITE, &oldProt)) {
            std::cout << "[-] " << hookName << ": VirtualProtect(RWX) failed.\n";
            VirtualFree(tramp, 0, MEM_RELEASE);
            delete h;
            return nullptr;
        }
        {
            uint8_t* dst = (uint8_t*)addr;
            const int32_t rel = (int32_t)((intptr_t)tramp - (intptr_t)(addr + kJmpLen));
            dst[0] = 0xE9;
            std::memcpy(dst + 1, &rel, sizeof(rel));
            if (patchLen > kJmpLen) {
                std::memset(dst + kJmpLen, 0x90, patchLen - kJmpLen);
            }
        }
        VirtualProtect((void*)addr, patchLen, oldProt, &oldProt);
        FlushInstructionCache(GetCurrentProcess(), (void*)addr, patchLen);

        std::cout << "[+] " << hookName << " installed (trampoline @ 0x" << std::hex
                  << (uintptr_t)tramp << std::dec << ", " << trampolineLen << " bytes).\n";
        return h;
    }

    void SetEnabled(Handle* h, bool enabled)
    {
        if (!h || !h->trampoline) return;
        h->trampoline[h->toggleOffset] = enabled ? 1 : 0;
    }

    bool IsEnabled(const Handle* h)
    {
        if (!h || !h->trampoline) return false;
        return h->trampoline[h->toggleOffset] != 0;
    }

    void Shutdown(Handle* h)
    {
        if (!h) return;
        if (h->targetAddr) {
            DWORD oldProt = 0;
            if (VirtualProtect((void*)h->targetAddr, h->patchLen, PAGE_EXECUTE_READWRITE, &oldProt)) {
                std::memcpy((void*)h->targetAddr, h->originalBytes, h->patchLen);
                VirtualProtect((void*)h->targetAddr, h->patchLen, oldProt, &oldProt);
                FlushInstructionCache(GetCurrentProcess(), (void*)h->targetAddr, h->patchLen);
            }
            h->targetAddr = 0;
        }
        if (h->trampoline) {
            VirtualFree(h->trampoline, 0, MEM_RELEASE);
            h->trampoline = nullptr;
        }
        delete h;
    }
}
