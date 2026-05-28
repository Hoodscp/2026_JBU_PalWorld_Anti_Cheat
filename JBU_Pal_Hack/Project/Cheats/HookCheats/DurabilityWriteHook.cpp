#include "DurabilityWriteHook.h"
#include "../../Memory/NopHook.h"
#include "../../SDK/Signatures.h"
#include "../../GUI/Menu.h"

namespace DurabilityWriteHook
{
    // movss [rcx+0x10], xmm0 — 5바이트 (F3 0F 11 41 10). 통째로 NOP.
    // AOB 의 나머지 4바이트(F3 0F 11 51) 는 다음 명령어 시작이므로 침범 금지.
    static constexpr size_t kPatchLen    = 5;
    static constexpr size_t kNopInstrLen = 5;

    static MidHook::Handle* g_handle = nullptr;

    bool Install()
    {
        g_handle = NopHook::Install("DurabilityWriteHook", "Palworld-Win64-Shipping.exe",
                                    SDK::Signatures::DurabilityWrite, kPatchLen, kNopInstrLen);
        return true;
    }

    void Tick()
    {
        MidHook::SetEnabled(g_handle, Menu::Config.bHookInfDurability);
    }

    void Shutdown()
    {
        MidHook::Shutdown(g_handle);
        g_handle = nullptr;
    }
}
