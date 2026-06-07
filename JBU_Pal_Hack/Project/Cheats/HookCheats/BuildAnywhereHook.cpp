#include "BuildAnywhereHook.h"
#include "../../Memory/MidHook.h"
#include "../../SDK/Signatures.h"
#include "../../GUI/Menu.h"

// 패치 사이트(원본 7바이트):
//   40 38 B1 E4 00 00 00      cmp byte ptr [rcx+0xE4], sil
//
// bodyEnabled (14바이트) — sil 을 같은 메모리에 미리 write 해 cmp 통과 강제:
//   40 88 B1 E4 00 00 00      mov  byte ptr [rcx+0xE4], sil   (강제 동기화)
//   40 38 B1 E4 00 00 00      cmp  byte ptr [rcx+0xE4], sil   (원본 재실행)
//
// bodyDisabled (7바이트) — 원본 cmp 그대로.
//
// ※ MidHook 은 bodyEnabled / bodyDisabled 가 패치 사이트의 원본을 "재실행" 한다고
//    가정하지 않는다. 호출자가 직접 포함시킨다(TemperatureHook 패턴 그대로).
namespace BuildAnywhereHook
{
    static constexpr size_t kPatchLen = 7;

    static const uint8_t kBodyEnabled[] = {
        // mov byte ptr [rcx+0xE4], sil
        0x40, 0x88, 0xB1, 0xE4, 0x00, 0x00, 0x00,
        // cmp byte ptr [rcx+0xE4], sil   (원본)
        0x40, 0x38, 0xB1, 0xE4, 0x00, 0x00, 0x00,
    };
    static const uint8_t kBodyDisabled[] = {
        // 원본 cmp 만 실행
        0x40, 0x38, 0xB1, 0xE4, 0x00, 0x00, 0x00,
    };

    static MidHook::Handle* g_handle = nullptr;

    bool Install()
    {
        g_handle = MidHook::Install("BuildAnywhereHook",
                                    "Palworld-Win64-Shipping.exe",
                                    SDK::Signatures::BuildableTest,
                                    kPatchLen,
                                    kBodyEnabled,  sizeof(kBodyEnabled),
                                    kBodyDisabled, sizeof(kBodyDisabled));
        return true;
    }

    void Tick()
    {
        MidHook::SetEnabled(g_handle, Menu::Config.bHookBuildAnywhere);
    }

    void Shutdown()
    {
        MidHook::Shutdown(g_handle);
        g_handle = nullptr;
    }
}
