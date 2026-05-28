#include "WriteItemsHook.h"
#include "../../Memory/NopHook.h"
#include "../../SDK/Signatures.h"
#include "../../GUI/Menu.h"

namespace WriteItemsHook
{
    // mov [rbx+0x154], r14d — 7바이트. 마지막 0x45 는 AOB uniqueness 용 다음 instr REX.
    // patchLen = 7 (다음 instr 침범 X).
    static constexpr size_t kPatchLen    = 7;
    static constexpr size_t kNopInstrLen = 7;

    static MidHook::Handle* g_handle = nullptr;

    bool Install()
    {
        g_handle = NopHook::Install("WriteItemsHook", "Palworld-Win64-Shipping.exe",
                                    SDK::Signatures::WritesToItems, kPatchLen, kNopInstrLen);
        return true;
    }

    void Tick()
    {
        MidHook::SetEnabled(g_handle, Menu::Config.bHookItemsNoDecrease);
    }

    void Shutdown()
    {
        MidHook::Shutdown(g_handle);
        g_handle = nullptr;
    }
}
