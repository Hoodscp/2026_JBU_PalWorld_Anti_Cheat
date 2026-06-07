#include "ResearchIncreaseHook.h"
#include "../../Memory/MidHook.h"
#include "../../SDK/Signatures.h"
#include "../../GUI/Menu.h"

// 패치 사이트(원본 5바이트):
//   F3 0F 58 77 14            addss xmm6, [rdi+0x14]
//
// bodyEnabled (9바이트):
//   F3 0F 58 77 14            addss xmm6, [rdi+0x14]     (원본)
//   F3 0F 59 F6               mulss xmm6, xmm6           (4B, 강제 제곱)
//
// bodyDisabled (5바이트): 원본 addss 그대로.
//
// patchLen = 5 (정확히 jmp 공간). 다음 명령어는 사이트 외부라 안전.
namespace ResearchIncreaseHook
{
    static constexpr size_t kPatchLen = 5;

    static const uint8_t kBodyEnabled[] = {
        // addss xmm6, [rdi+0x14]   (원본)
        0xF3, 0x0F, 0x58, 0x77, 0x14,
        // mulss xmm6, xmm6
        0xF3, 0x0F, 0x59, 0xF6,
    };
    static const uint8_t kBodyDisabled[] = {
        0xF3, 0x0F, 0x58, 0x77, 0x14,
    };

    static MidHook::Handle* g_handle = nullptr;

    bool Install()
    {
        g_handle = MidHook::Install("ResearchIncreaseHook",
                                    "Palworld-Win64-Shipping.exe",
                                    SDK::Signatures::ResearchIncrease,
                                    kPatchLen,
                                    kBodyEnabled,  sizeof(kBodyEnabled),
                                    kBodyDisabled, sizeof(kBodyDisabled));
        return true;
    }

    void Tick()
    {
        MidHook::SetEnabled(g_handle, Menu::Config.bHookQuickResearch);
    }

    void Shutdown()
    {
        MidHook::Shutdown(g_handle);
        g_handle = nullptr;
    }
}
