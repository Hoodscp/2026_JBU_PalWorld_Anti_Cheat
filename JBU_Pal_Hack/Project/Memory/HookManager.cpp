#include "HookManager.h"
#include <iostream>
#include "MinHook.h"

namespace HookManager
{
    void Initialize()
    {
        MH_STATUS status = MH_Initialize();
        if (status == MH_OK || status == MH_ERROR_ALREADY_INITIALIZED) {
            std::cout << "[+] HookManager: MinHook ready.\n";
        } else {
            std::cout << "[-] HookManager: MH_Initialize failed (code " << status << ").\n";
        }
    }

    void Shutdown()
    {
        // Note: MH_Uninitialize is intentionally NOT called here.
        // Kiero owns the MinHook lifecycle in this project (KIERO_USE_MINHOOK=1)
        // and calls MH_Uninitialize from kiero::shutdown(). Calling it from both
        // sides causes a use-after-free on the Present hook trampoline.
        MH_DisableHook(MH_ALL_HOOKS);
    }

    bool CreateHook(void* target, void* detour, void** original)
    {
        if (!target || !detour || !original) return false;

        if (MH_CreateHook(target, detour, original) != MH_OK) return false;
        if (MH_EnableHook(target) != MH_OK) return false;
        return true;
    }
}
