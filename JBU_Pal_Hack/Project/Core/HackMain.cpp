#include "HackMain.h"
#include <stdio.h>
#include <iostream>
#include "../GUI/Overlay.h"

namespace HackMain
{
    bool bIsRunning = true;

    void Initialize(HMODULE hMod)
    {
        // 1. Allocate Console for debug logging
        AllocConsole();
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);

        std::cout << "[+] JBU Palworld Internal Hack Loaded\n";

        // 2. Initialize Hooking (MinHook / Kiero)
        // HookManager::Initialize();
        
        // 3. Initialize GUI Overlay (ImGui Render Hook)
        Overlay::Initialize();

        // 4. Start main loop
        MainLoop();
    }

    void Shutdown()
    {
        bIsRunning = false;
        
        // Unhook ALL
        Overlay::Shutdown();
        // HookManager::Shutdown();

        // Free Console
        HWND consoleWnd = GetConsoleWindow();
        FreeConsole();
        if (consoleWnd) {
            PostMessage(consoleWnd, WM_CLOSE, 0, 0);
        }
    }

    void MainLoop()
    {
        while (bIsRunning)
        {
            // Add keybind to unload the hack
            if (GetAsyncKeyState(VK_END) & 1)
            {
                break;
            }

            // Execute core cheat logic (e.g. Health freeze, ammo hack)
            // MemoryUtils::ApplyCheats();

            Sleep(10); // Prevent CPU maxout
        }

        Shutdown();
    }
}
