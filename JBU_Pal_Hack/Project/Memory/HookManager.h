#pragma once

namespace HookManager
{
    // Initialize MinHook and set up hooks
    void Initialize();

    // Remove all hooks
    void Shutdown();

    // Helper template/function to create a hook
    bool CreateHook(void* target, void* detour, void** original);
}
