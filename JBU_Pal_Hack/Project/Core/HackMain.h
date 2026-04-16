#pragma once
#include <windows.h>

namespace HackMain
{
    // Initializes the core logic, allocates console, hooks etc.
    void Initialize(HMODULE hMod);

    // Cleans up memory and unhooks
    void Shutdown();

    // The main loop for hack logic like memory writing, aob updates
    void MainLoop();

    // Used to signal the main loop to stop
    extern bool bIsRunning;
}
