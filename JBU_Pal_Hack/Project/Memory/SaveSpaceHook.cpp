#include "SaveSpaceHook.h"
#include "Scanner.h"
#include <windows.h>
#include <iostream>
#include <cstring>

namespace SaveSpaceHook
{
    static constexpr size_t kJmpLen      = 5;  // E9 + disp32
    static constexpr size_t kMovMemRegLen = 7; // REX.W + 89 + ModR/M + disp32

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

    // ModR/M = (reg << 3) | 0b101  → RIP-relative + reg encoding.
    // x64 reg 인덱스: rax=0, rcx=1, rdx=2, rbx=3, rsp=4, rbp=5, rsi=6, rdi=7
    static uint8_t ModRMForReg(Reg r)
    {
        switch (r) {
            case Reg::Rax: return 0x05;
            case Reg::Rcx: return 0x0D;
            case Reg::Rdx: return 0x15;
            case Reg::Rbx: return 0x1D;
            case Reg::Rsp: return 0x25;
            case Reg::Rbp: return 0x2D;
            case Reg::Rsi: return 0x35;
            case Reg::Rdi: return 0x3D;
        }
        return 0x05;
    }

    Handle* Install(const char* hookName,
                    const char* moduleName,
                    const char* signature,
                    size_t      patchLen,
                    Reg         captureReg,
                    size_t      sigOffset)
    {
        if (!hookName) hookName = "SaveSpaceHook";

        if (!signature || !*signature) {
            std::cout << "[*] " << hookName << ": signature empty, skip install.\n";
            return nullptr;
        }
        if (patchLen < kJmpLen || patchLen > sizeof(Handle{}.originalBytes)) {
            std::cout << "[-] " << hookName << ": invalid patchLen (" << patchLen << ").\n";
            return nullptr;
        }

        const uintptr_t matchAddr = Scanner::FindPattern(moduleName, signature);
        if (!matchAddr) {
            std::cout << "[-] " << hookName << ": pattern not found in " << moduleName << ".\n";
            return nullptr;
        }
        const uintptr_t addr = matchAddr + sigOffset;
        std::cout << "[+] " << hookName << ": match @ 0x" << std::hex << matchAddr
                  << " (patch @ +0x" << sigOffset << " = 0x" << addr << ")" << std::dec << "\n";

        // 트램폴린: mov [rip+disp], reg (7) + orig (patchLen) + jmp ret (5) + slot (8 — 8-byte 정렬)
        // 슬롯을 8-byte 정렬하기 위해 약간의 padding 추가.
        const size_t headerLen = kMovMemRegLen + patchLen + kJmpLen;
        const size_t alignedSlotOff = (headerLen + 7u) & ~7u; // 8-byte align
        const size_t trampolineLen = alignedSlotOff + 8;

        uint8_t* tramp = (uint8_t*)AllocateNear(addr, trampolineLen);
        if (!tramp) {
            std::cout << "[-] " << hookName << ": AllocateNear failed.\n";
            return nullptr;
        }

        const intptr_t delta = (intptr_t)tramp - (intptr_t)addr;
        if (delta > (intptr_t)0x7FFFFFF0 || delta < -(intptr_t)0x7FFFFFF0) {
            std::cout << "[-] " << hookName << ": trampoline out of jmp range.\n";
            VirtualFree(tramp, 0, MEM_RELEASE);
            return nullptr;
        }

        Handle* h = new Handle();
        h->targetAddr     = addr;
        h->patchLen       = patchLen;
        h->trampoline     = tramp;
        h->trampolineLen  = trampolineLen;
        h->saveSlotOffset = alignedSlotOff;
        std::memcpy(h->originalBytes, (const void*)addr, patchLen);

        // 슬롯 0 초기화
        *(uint64_t*)(tramp + alignedSlotOff) = 0;

        // ── 트램폴린 작성 ──
        uint8_t* p = tramp;
        const uintptr_t returnAddr = addr + patchLen;
        const uintptr_t slotAddr   = (uintptr_t)(tramp + alignedSlotOff);

        // 0x00: mov [rip + disp32], <reg>
        *p++ = 0x48; // REX.W
        *p++ = 0x89; // MOV r/m64, r64
        *p++ = ModRMForReg(captureReg);
        {
            const uintptr_t nextRip = (uintptr_t)(p + 4);
            const int32_t disp = (int32_t)((intptr_t)slotAddr - (intptr_t)nextRip);
            std::memcpy(p, &disp, sizeof(disp));
            p += 4;
        }

        // <original bytes>
        std::memcpy(p, h->originalBytes, patchLen);
        p += patchLen;

        // jmp returnAddr
        *p++ = 0xE9;
        {
            const uintptr_t nextRip = (uintptr_t)(p + 4);
            const int32_t disp = (int32_t)((intptr_t)returnAddr - (intptr_t)nextRip);
            std::memcpy(p, &disp, sizeof(disp));
            p += 4;
        }

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
                  << (uintptr_t)tramp << std::dec << ", saveSlot @ +0x"
                  << std::hex << alignedSlotOff << std::dec << ").\n";
        return h;
    }

    void* Current(const Handle* h)
    {
        if (!h || !h->trampoline) return nullptr;
        return (void*)*(uint64_t*)(h->trampoline + h->saveSlotOffset);
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
