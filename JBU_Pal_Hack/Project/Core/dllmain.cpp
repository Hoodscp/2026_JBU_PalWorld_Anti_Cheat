#include <windows.h>
#include <thread>
#include "HackMain.h"

// Main thread thread entry point
DWORD WINAPI MainThread(LPVOID lpReserved)
{
    // Initialize Hack System
    HackMain::Initialize(static_cast<HMODULE>(lpReserved));

    // Optional: FreeLibraryAndExitThread when ejecting
    // FreeLibraryAndExitThread(static_cast<HMODULE>(lpReserved), 0);
    return TRUE;
}

// DLL Entry Point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        // Create new thread for our hack so we don't block the game's loader
        CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        HackMain::Shutdown();
        break;
    }
    return TRUE;
}
