#include "CanCatchBossHook.h"
#include "../../Memory/MidHook.h"
#include "../../SDK/Signatures.h"
#include "../../GUI/Menu.h"

// 패치 사이트(원본 14바이트):
//   48 8B 83 30 06 00 00            mov rax, [rbx+0x630]
//   80 B8 98 05 00 00 00            cmp byte ptr [rax+0x598], 0   (imm8=0)
//
// bodyEnabled (22바이트):
//   48 8B 83 30 06 00 00            mov  rax, [rbx+0x630]          (원본)
//   C6 80 98 05 00 00 00 00         mov  byte ptr [rax+0x598], 0   (8B, 강제 0)
//   80 B8 98 05 00 00 00            cmp  byte ptr [rax+0x598], 0   (원본 재실행)
//
//   합계 22B → MidHook 가 `je rel8 → disabled` 로 점프할 거리는
//   22 + 5(jmp return) = 27 (≤ 127, OK).
//
// bodyDisabled (14바이트): 원본 mov + cmp 그대로.
namespace CanCatchBossHook
{
    static constexpr size_t kPatchLen = 14;

    static const uint8_t kBodyEnabled[] = {
        // mov rax, [rbx+0x630]                       (원본)
        0x48, 0x8B, 0x83, 0x30, 0x06, 0x00, 0x00,
        // mov byte ptr [rax+0x598], 0                (강제 0)
        0xC6, 0x80, 0x98, 0x05, 0x00, 0x00, 0x00, 0x00,
        // cmp byte ptr [rax+0x598], 0                (원본 재실행)
        0x80, 0xB8, 0x98, 0x05, 0x00, 0x00, 0x00,
    };
    static const uint8_t kBodyDisabled[] = {
        // mov rax, [rbx+0x630]
        0x48, 0x8B, 0x83, 0x30, 0x06, 0x00, 0x00,
        // cmp byte ptr [rax+0x598], 0
        0x80, 0xB8, 0x98, 0x05, 0x00, 0x00, 0x00,
    };

    static MidHook::Handle* g_handle = nullptr;

    bool Install()
    {
        g_handle = MidHook::Install("CanCatchBossHook",
                                    "Palworld-Win64-Shipping.exe",
                                    SDK::Signatures::CanCatchBossPal,
                                    kPatchLen,
                                    kBodyEnabled,  sizeof(kBodyEnabled),
                                    kBodyDisabled, sizeof(kBodyDisabled));
        return true;
    }

    void Tick()
    {
        MidHook::SetEnabled(g_handle, Menu::Config.bHookCanCatchBoss);
    }

    void Shutdown()
    {
        MidHook::Shutdown(g_handle);
        g_handle = nullptr;
    }
}
