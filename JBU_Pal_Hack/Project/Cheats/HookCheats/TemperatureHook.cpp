#include "TemperatureHook.h"
#include "../../GUI/Menu.h"
#include "../../Memory/Scanner.h"
#include "../../SDK/Signatures.h"
#include <windows.h>
#include <iostream>
#include <cstdint>
#include <cstring>

// ────────────────────────────────────────────────────────────────────
// 트램폴린 레이아웃 (총 0x22 = 34 바이트)
//   00  50                            push rax
//   01  8A 05 1A 00 00 00              mov al, byte ptr [rip + 0x1A]   ; ← toggle 읽기
//   07  84 C0                          test al, al
//   09  58                             pop rax                          ; flags 보존
//   0A  74 02                          je  doOriginal
//   0C  33 C0                          xor eax, eax                     ; ★ 강제 0
//   0E  48 8B 9C 24 B0 00 00 00        mov rbx, [rsp+0xB0]              ; (원본)
//   16  39 87 38 01 00 00              cmp [rdi+0x138], eax             ; (원본)
//   1C  E9 xx xx xx xx                 jmp <returnAddr>
//   21  00                             db 0  ; toggle byte
// ────────────────────────────────────────────────────────────────────

namespace TemperatureHook
{
    static constexpr size_t kPatchLen      = 14;     // AOB 길이 = 패치 영역
    static constexpr size_t kTrampolineLen = 0x22;   // 34 바이트
    static constexpr size_t kToggleOffset  = 0x21;   // 트램폴린 내 toggle byte 위치

    static uintptr_t g_targetAddr   = 0;
    static uint8_t*  g_trampoline   = nullptr;
    static uint8_t   g_originalBytes[kPatchLen] = {};

    // ────────────────────────────────────────────────────────────
    // 32-bit 상대 jmp 가 도달 가능하도록 anchor ±2GB 안에서
    // VirtualAlloc 시도. 64KB granularity 단위 스캔.
    // ────────────────────────────────────────────────────────────
    static void* AllocateNear(uintptr_t anchor, size_t size)
    {
        SYSTEM_INFO si{};
        GetSystemInfo(&si);
        const uintptr_t granularity = si.dwAllocationGranularity;
        const uintptr_t lo = (anchor > 0x70000000ULL) ? (anchor - 0x70000000ULL) : 0;
        const uintptr_t hi = anchor + 0x70000000ULL;

        // anchor 보다 낮은 쪽부터 → 높은 쪽 순서로 시도.
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

    static void WriteTrampoline(uint8_t* tramp, uintptr_t returnAddr)
    {
        uint8_t* p = tramp;

        // 0x00: push rax
        *p++ = 0x50;

        // 0x01: mov al, byte ptr [rip + disp32]
        // 명령어 6바이트 → 다음 RIP = 0x07. toggle은 0x21 → disp = 0x1A.
        *p++ = 0x8A;
        *p++ = 0x05;
        const int32_t togDisp = (int32_t)kToggleOffset - (int32_t)0x07;
        std::memcpy(p, &togDisp, sizeof(togDisp));
        p += 4;

        // 0x07: test al, al
        *p++ = 0x84;
        *p++ = 0xC0;

        // 0x09: pop rax  (flags 변경 X)
        *p++ = 0x58;

        // 0x0A: je +2 (skip xor)
        *p++ = 0x74;
        *p++ = 0x02;

        // 0x0C: xor eax, eax
        *p++ = 0x33;
        *p++ = 0xC0;

        // 0x0E: 원본 - mov rbx, [rsp+0xB0]
        static const uint8_t kOrigMov[] = { 0x48, 0x8B, 0x9C, 0x24, 0xB0, 0x00, 0x00, 0x00 };
        std::memcpy(p, kOrigMov, sizeof(kOrigMov));
        p += sizeof(kOrigMov);

        // 0x16: 원본 - cmp [rdi+0x138], eax
        static const uint8_t kOrigCmp[] = { 0x39, 0x87, 0x38, 0x01, 0x00, 0x00 };
        std::memcpy(p, kOrigCmp, sizeof(kOrigCmp));
        p += sizeof(kOrigCmp);

        // 0x1C: jmp <returnAddr>
        *p++ = 0xE9;
        const int32_t retDisp = (int32_t)((intptr_t)returnAddr - (intptr_t)(tramp + 0x21));
        std::memcpy(p, &retDisp, sizeof(retDisp));
        p += 4;

        // 0x21: toggle byte (default OFF)
        *p++ = 0x00;
    }

    bool Install()
    {
        if (g_trampoline) {
            std::cout << "[*] TemperatureHook: already installed.\n";
            return true;
        }

        const char* sig = SDK::Signatures::Temperature;
        if (!sig || !*sig) {
            std::cout << "[*] TemperatureHook: signature empty, skip install.\n";
            return true; // placeholder 모드 — 실패 아님
        }

        const uintptr_t addr = Scanner::FindPattern("Palworld-Win64-Shipping.exe", sig);
        if (!addr) {
            std::cout << "[-] TemperatureHook: pattern not found.\n";
            return false;
        }
        std::cout << "[+] TemperatureHook: target @ 0x" << std::hex << addr << std::dec << "\n";

        uint8_t* tramp = (uint8_t*)AllocateNear(addr, kTrampolineLen);
        if (!tramp) {
            std::cout << "[-] TemperatureHook: AllocateNear failed.\n";
            return false;
        }

        // 32-bit 상대 jmp 도달 검증 (양 방향 모두 ±2GB 안)
        const intptr_t delta = (intptr_t)tramp - (intptr_t)addr;
        if (delta >  (intptr_t)0x7FFFFFF0 || delta < -(intptr_t)0x7FFFFFF0) {
            std::cout << "[-] TemperatureHook: trampoline out of jmp range (delta=0x"
                      << std::hex << delta << std::dec << ").\n";
            VirtualFree(tramp, 0, MEM_RELEASE);
            return false;
        }

        // 원본 14바이트 백업
        std::memcpy(g_originalBytes, (const void*)addr, kPatchLen);

        // 트램폴린 작성 (returnAddr = 패치 사이트 + 14)
        WriteTrampoline(tramp, addr + kPatchLen);
        FlushInstructionCache(GetCurrentProcess(), tramp, kTrampolineLen);

        // 패치 사이트에 jmp <tramp> + NOP*9 작성
        DWORD oldProt = 0;
        if (!VirtualProtect((void*)addr, kPatchLen, PAGE_EXECUTE_READWRITE, &oldProt)) {
            std::cout << "[-] TemperatureHook: VirtualProtect (RWX) failed.\n";
            VirtualFree(tramp, 0, MEM_RELEASE);
            return false;
        }
        {
            uint8_t* dst = (uint8_t*)addr;
            const int32_t rel = (int32_t)((intptr_t)tramp - (intptr_t)(addr + 5));
            dst[0] = 0xE9;
            std::memcpy(dst + 1, &rel, sizeof(rel));
            std::memset(dst + 5, 0x90, kPatchLen - 5);  // NOP * 9
        }
        VirtualProtect((void*)addr, kPatchLen, oldProt, &oldProt);
        FlushInstructionCache(GetCurrentProcess(), (void*)addr, kPatchLen);

        g_targetAddr = addr;
        g_trampoline = tramp;
        std::cout << "[+] TemperatureHook installed (trampoline @ 0x" << std::hex
                  << (uintptr_t)tramp << std::dec << ").\n";
        return true;
    }

    void Tick()
    {
        if (!g_trampoline) return;
        // 활성/비활성 byte 1개 store. trampoline의 mov al 명령이 매번 이걸 읽어
        // 분기 결정 → C++ 측에선 이 한 줄만 매 프레임 하면 충분.
        g_trampoline[kToggleOffset] = Menu::Config.bForceNormalTemp ? 1 : 0;
    }

    void Shutdown()
    {
        if (g_targetAddr) {
            DWORD oldProt = 0;
            if (VirtualProtect((void*)g_targetAddr, kPatchLen, PAGE_EXECUTE_READWRITE, &oldProt)) {
                std::memcpy((void*)g_targetAddr, g_originalBytes, kPatchLen);
                VirtualProtect((void*)g_targetAddr, kPatchLen, oldProt, &oldProt);
                FlushInstructionCache(GetCurrentProcess(), (void*)g_targetAddr, kPatchLen);
            }
            g_targetAddr = 0;
        }
        if (g_trampoline) {
            VirtualFree(g_trampoline, 0, MEM_RELEASE);
            g_trampoline = nullptr;
        }
    }
}
