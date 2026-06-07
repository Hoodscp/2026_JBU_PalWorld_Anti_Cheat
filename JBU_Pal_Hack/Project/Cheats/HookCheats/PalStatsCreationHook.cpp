#include "PalStatsCreationHook.h"
#include "../../Memory/MidHook.h"
#include "../../SDK/Signatures.h"
#include "../../GUI/Menu.h"

// 패치 사이트(원본 6바이트, AOB 의 첫 단일 명령어):
//   41 BE 65 00 00 00         mov r14d, 0x65
//
// bodyEnabled (11바이트):
//   BF 64 00 00 00            mov  edi, 0x64           (강제 주입; 5B)
//   41 BE 65 00 00 00         mov  r14d, 0x65          (원본 재실행; 6B)
//
// bodyDisabled (6바이트): 원본 mov r14d 그대로.
//
// AOB 의 sig 는 uniqueness 보장 위해 13B 까지 매치하지만, patchLen 은 첫
// 명령어 6B 까지만 가져온다. 다음 명령어(44 2B F7 = sub r14d, edi) 가
// 우리가 강제한 edi 값을 사용한다 — 효과의 핵심.
namespace PalStatsCreationHook
{
    static constexpr size_t kPatchLen = 6;

    static const uint8_t kBodyEnabled[] = {
        // mov edi, 0x64        ; 강제 주입
        0xBF, 0x64, 0x00, 0x00, 0x00,
        // mov r14d, 0x65       ; 원본 재실행
        0x41, 0xBE, 0x65, 0x00, 0x00, 0x00,
    };
    static const uint8_t kBodyDisabled[] = {
        0x41, 0xBE, 0x65, 0x00, 0x00, 0x00,
    };

    static MidHook::Handle* g_handle = nullptr;

    bool Install()
    {
        g_handle = MidHook::Install("PalStatsCreationHook",
                                    "Palworld-Win64-Shipping.exe",
                                    SDK::Signatures::PalStatsCreation,
                                    kPatchLen,
                                    kBodyEnabled,  sizeof(kBodyEnabled),
                                    kBodyDisabled, sizeof(kBodyDisabled));
        return true;
    }

    void Tick()
    {
        MidHook::SetEnabled(g_handle, Menu::Config.bHookPerfectPalStats);
    }

    void Shutdown()
    {
        MidHook::Shutdown(g_handle);
        g_handle = nullptr;
    }
}
