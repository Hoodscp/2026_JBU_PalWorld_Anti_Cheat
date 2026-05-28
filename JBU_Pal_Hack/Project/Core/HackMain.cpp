#include "HackMain.h"
#include <stdio.h>
#include <iostream>
#include <atomic>
#include "../GUI/Overlay.h"
#include "../Memory/HookManager.h"
#include "../Memory/CursorHooks.h"
#include "../Cheats/MemoryCheats/GodMode.h"
#include "../Cheats/MemoryCheats/PlayerCheats.h"
#include "../Cheats/MemoryCheats/TechCheats.h"
#include "../Cheats/MemoryCheats/InventoryCheats.h"
#include "../Cheats/MemoryCheats/StatBoost.h"
#include "../Cheats/MemoryCheats/SocialCheats.h"
#include "../Cheats/MemoryCheats/PalCheats.h"
#include "../Cheats/MemoryCheats/ItemDurabilityCheats.h"
#include "../Cheats/HookCheats/ExampleHook.h"
#include "../Cheats/HookCheats/TemperatureHook.h"
// Track 2 / P0 — NOP 패치 후킹들
#include "../Cheats/HookCheats/StaminaSubHook.h"
#include "../Cheats/HookCheats/FoodHungerHook.h"
#include "../Cheats/HookCheats/FoodTimerHook.h"
#include "../Cheats/HookCheats/WriteItemsHook.h"
#include "../Cheats/HookCheats/DurabilityWriteHook.h"
#include "../Cheats/HookCheats/AmmoDecreaseHook.h"
// Track 2 / P1 — 포인터 캡처 후킹들 (토글 없이 항상 활성)
#include "../Cheats/HookCheats/PlayerPtrCaptureHook.h"
#include "../Cheats/HookCheats/ItemSlotCaptureHook.h"
#include "../Cheats/HookCheats/TechPointCaptureHook.h"
#include "../Cheats/HookCheats/ExpPtrCaptureHook.h"

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

        // 2-1. Install system-level hooks (cursor lock bypass for ImGui)
        CursorHooks::Install();

        // 2-2. Install game-function hooks (Track 2: HookCheats).
        // 새 후킹 치트는 여기에 한 줄씩 추가.
        ExampleHook::Install();
        TemperatureHook::Install();
        // P0 — NOP 패치 후킹 (토글 가능)
        StaminaSubHook::Install();
        FoodHungerHook::Install();
        FoodTimerHook::Install();
        WriteItemsHook::Install();
        DurabilityWriteHook::Install();
        AmmoDecreaseHook::Install();
        // P1 — 포인터 캡처 후킹 (항상 ON, GWorld 백업 경로 제공)
        PlayerPtrCaptureHook::Install();
        ItemSlotCaptureHook::Install();
        TechPointCaptureHook::Install();
        ExpPtrCaptureHook::Install();

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
        // mid-function patch 들은 직접 byte patch 라 HookManager 와 별개로 정리해야 함
        // (정리 누락 시 DLL 언로드 후 dangling jmp 로 게임 크래시).
        TemperatureHook::Shutdown();
        StaminaSubHook::Shutdown();
        FoodHungerHook::Shutdown();
        FoodTimerHook::Shutdown();
        WriteItemsHook::Shutdown();
        DurabilityWriteHook::Shutdown();
        AmmoDecreaseHook::Shutdown();
        PlayerPtrCaptureHook::Shutdown();
        ItemSlotCaptureHook::Shutdown();
        TechPointCaptureHook::Shutdown();
        ExpPtrCaptureHook::Shutdown();
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

            // 매 프레임 활성화된 메모리 치트들을 적용 (Track 1: MemoryCheats).
            // 새 메모리 치트는 여기에 한 줄씩 추가.
            GodMode::Tick();
            PlayerCheats::Tick();
            TechCheats::Tick();
            InventoryCheats::Tick();
            StatBoost::Tick();
            SocialCheats::Tick();
            PalCheats::Tick();
            ItemDurabilityCheats::Tick();

            // Track 2 후킹 중 매 프레임 toggle 동기화가 필요한 것만 호출.
            // (포인터 캡처 후킹은 토글 없이 항상 ON 이라 Tick 불필요.)
            TemperatureHook::Tick();
            StaminaSubHook::Tick();
            FoodHungerHook::Tick();
            FoodTimerHook::Tick();
            WriteItemsHook::Tick();
            DurabilityWriteHook::Tick();
            AmmoDecreaseHook::Tick();

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
