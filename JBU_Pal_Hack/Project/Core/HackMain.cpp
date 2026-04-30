#include "HackMain.h"
#include <stdio.h>
#include <iostream>
#include <atomic>
#include "../GUI/Overlay.h"
#include "../Memory/HookManager.h"
#include "../Memory/CursorHooks.h"
#include "../Memory/Cheats.h"

namespace HackMain
{
    bool bIsRunning = true;
    static HMODULE g_hModule = nullptr;
    static std::atomic<bool> g_shutdownDone{false};

    void Initialize(HMODULE hMod)
    {
        g_hModule = hMod;

        // 1. Allocate Console for debug logging
        AllocConsole();
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);

        std::cout << "[+] JBU Palworld Internal Hack Loaded\n";

        // 2. Initialize Hooking (MinHook / Kiero)
        HookManager::Initialize();

        // 2-1. Install function hooks (cursor lock bypass for ImGui)
        CursorHooks::Install();

        // 3. Initialize GUI Overlay (ImGui Render Hook)
        Overlay::Initialize();

        // 4. Start main loop
        MainLoop();
    }

    void Shutdown()
    {
        // Idempotent: MainLoop의 명시 호출 경로와 DLL_PROCESS_DETACH 경로 모두에서
        // Shutdown이 호출될 수 있으므로 첫 번째 호출만 실제 정리를 수행합니다.
        bool expected = false;
        if (!g_shutdownDone.compare_exchange_strong(expected, true)) {
            return;
        }
        bIsRunning = false;

        // 게임 함수 후킹을 먼저 풀고, 그 다음 렌더 후킹을 정리합니다.
        // Overlay::Shutdown 내부의 kiero::shutdown이 MH_Uninitialize를 소유하므로
        // HookManager::Shutdown은 그 전에 와야 합니다.
        HookManager::Shutdown();
        Overlay::Shutdown();

        // Free Console (PostMessage는 invalid HWND 위험이 있어 제거)
        FreeConsole();
    }

    void MainLoop()
    {
        while (bIsRunning)
        {
            if (GetAsyncKeyState(VK_END) & 1)
            {
                break;
            }

            // 매 프레임 활성화된 치트들을 적용 (Menu visibility와 무관)
            Cheats::Tick();

            Sleep(10); // Prevent CPU maxout
        }

        // 명시적 정리 후 DLL을 자연스럽게 언로드합니다.
        // FreeLibraryAndExitThread는 DLL_PROCESS_DETACH를 트리거하지만
        // Shutdown은 idempotent하므로 두 번째 호출은 즉시 리턴됩니다.
        Shutdown();

        if (g_hModule) {
            FreeLibraryAndExitThread(g_hModule, 0);
        }
    }
}
