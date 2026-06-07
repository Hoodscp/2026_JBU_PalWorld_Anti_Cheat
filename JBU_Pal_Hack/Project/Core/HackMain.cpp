#include "HackMain.h"
#include "Logger.h"
#include "CrashHandler.h"
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
// Track 2 / P2 — 조건분기 강제 / 명령어 주입 후킹들 (토글)
#include "../Cheats/HookCheats/BuildAnywhereHook.h"
#include "../Cheats/HookCheats/CanCatchBossHook.h"
#include "../Cheats/HookCheats/PalStatsCreationHook.h"
#include "../Cheats/HookCheats/ResearchIncreaseHook.h"

namespace HackMain
{
    bool bIsRunning = true;
    static HMODULE g_hModule = nullptr;
    static std::atomic<bool> g_shutdownDone{false};

    void Initialize(HMODULE hMod)
    {
        g_hModule = hMod;

        // 0. Logger / CrashHandler — 가장 먼저. 이후 단계 추적 + 크래시 캐치.
        //    파일: %TEMP%\JBU_Pal_Hack.log  (게임 튕겨도 매 줄 즉시 flush)
        Logger::Init();
        JBU_PHASE("Initialize:LoggerReady");
        JBU_LOG("Log file: %s", Logger::FilePath().c_str());
        CrashHandler::Install();

        // 1. Allocate Console for debug logging
        //    (콘솔 출력은 Tee 가 같은 파일에도 자동 복사함 → 후크 모듈의 cout 도 보존)
        JBU_PHASE("Initialize:AllocConsole");
        AllocConsole();
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);

        std::cout << "[+] JBU Palworld Internal Hack Loaded\n";

        // 2. Initialize Hooking (MinHook / Kiero)
        JBU_PHASE("Initialize:HookManager");
        HookManager::Initialize();

        // 2-1. Install system-level hooks (cursor lock bypass for ImGui)
        JBU_PHASE("Initialize:CursorHooks");
        CursorHooks::Install();

        // 2-2. Install game-function hooks (Track 2: HookCheats).
        // 새 후킹 치트는 여기에 한 줄씩 추가.
        // ※ 각 Install 직전 PHASE 갱신 — 어느 후킹의 AOB 스캔/트램펄린에서
        //    튕겼는지 로그 마지막 줄로 즉시 식별 가능.
        JBU_PHASE("Initialize:Install ExampleHook");        ExampleHook::Install();
        JBU_PHASE("Initialize:Install TemperatureHook");    TemperatureHook::Install();
        // P0 — NOP 패치 후킹 (토글 가능)
        JBU_PHASE("Initialize:Install StaminaSubHook");     StaminaSubHook::Install();
        JBU_PHASE("Initialize:Install FoodHungerHook");     FoodHungerHook::Install();
        JBU_PHASE("Initialize:Install FoodTimerHook");      FoodTimerHook::Install();
        JBU_PHASE("Initialize:Install WriteItemsHook");     WriteItemsHook::Install();
        JBU_PHASE("Initialize:Install DurabilityWriteHook");DurabilityWriteHook::Install();
        JBU_PHASE("Initialize:Install AmmoDecreaseHook");   AmmoDecreaseHook::Install();
        // P1 — 포인터 캡처 후킹 (항상 ON, GWorld 백업 경로 제공)
        JBU_PHASE("Initialize:Install PlayerPtrCaptureHook");PlayerPtrCaptureHook::Install();
        JBU_PHASE("Initialize:Install ItemSlotCaptureHook"); ItemSlotCaptureHook::Install();
        JBU_PHASE("Initialize:Install TechPointCaptureHook");TechPointCaptureHook::Install();
        JBU_PHASE("Initialize:Install ExpPtrCaptureHook");   ExpPtrCaptureHook::Install();
        // P2 — 조건분기 강제 / 명령어 주입 후킹 (토글)
        JBU_PHASE("Initialize:Install BuildAnywhereHook");   BuildAnywhereHook::Install();
        JBU_PHASE("Initialize:Install CanCatchBossHook");    CanCatchBossHook::Install();
        JBU_PHASE("Initialize:Install PalStatsCreationHook");PalStatsCreationHook::Install();
        JBU_PHASE("Initialize:Install ResearchIncreaseHook");ResearchIncreaseHook::Install();

        // 3. Initialize GUI Overlay (ImGui Render Hook)
        JBU_PHASE("Initialize:OverlayInit");
        Overlay::Initialize();

        // 4. Start main loop
        JBU_PHASE("Initialize:DONE — entering MainLoop");
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
        JBU_PHASE("Shutdown:begin");

        // 게임 함수 후킹을 먼저 풀고, 그 다음 렌더 후킹을 정리합니다.
        // Overlay::Shutdown 내부의 kiero::shutdown이 MH_Uninitialize를 소유하므로
        // HookManager::Shutdown은 그 전에 와야 합니다.
        // mid-function patch 들은 직접 byte patch 라 HookManager 와 별개로 정리해야 함
        // (정리 누락 시 DLL 언로드 후 dangling jmp 로 게임 크래시).
        JBU_PHASE("Shutdown:TemperatureHook");      TemperatureHook::Shutdown();
        JBU_PHASE("Shutdown:StaminaSubHook");       StaminaSubHook::Shutdown();
        JBU_PHASE("Shutdown:FoodHungerHook");       FoodHungerHook::Shutdown();
        JBU_PHASE("Shutdown:FoodTimerHook");        FoodTimerHook::Shutdown();
        JBU_PHASE("Shutdown:WriteItemsHook");       WriteItemsHook::Shutdown();
        JBU_PHASE("Shutdown:DurabilityWriteHook"); DurabilityWriteHook::Shutdown();
        JBU_PHASE("Shutdown:AmmoDecreaseHook");     AmmoDecreaseHook::Shutdown();
        JBU_PHASE("Shutdown:PlayerPtrCaptureHook");PlayerPtrCaptureHook::Shutdown();
        JBU_PHASE("Shutdown:ItemSlotCaptureHook"); ItemSlotCaptureHook::Shutdown();
        JBU_PHASE("Shutdown:TechPointCaptureHook");TechPointCaptureHook::Shutdown();
        JBU_PHASE("Shutdown:ExpPtrCaptureHook");   ExpPtrCaptureHook::Shutdown();
        JBU_PHASE("Shutdown:BuildAnywhereHook");   BuildAnywhereHook::Shutdown();
        JBU_PHASE("Shutdown:CanCatchBossHook");    CanCatchBossHook::Shutdown();
        JBU_PHASE("Shutdown:PalStatsCreationHook");PalStatsCreationHook::Shutdown();
        JBU_PHASE("Shutdown:ResearchIncreaseHook");ResearchIncreaseHook::Shutdown();
        JBU_PHASE("Shutdown:HookManager");          HookManager::Shutdown();
        JBU_PHASE("Shutdown:Overlay");              Overlay::Shutdown();

        // CrashHandler 는 마지막에 — 그 사이 발생할 예외도 잡기 위해.
        JBU_PHASE("Shutdown:CrashHandler");
        CrashHandler::Shutdown();
        JBU_PHASE("Shutdown:DONE");
        Logger::Shutdown();  // 마지막. 이후 로그 X.

        // Free Console (PostMessage는 invalid HWND 위험이 있어 제거)
        FreeConsole();
    }

    void MainLoop()
    {
        // 매 프레임 phase 갱신은 quiet — 파일 flush 없음(atomic store 1회).
        // 크래시 핸들러는 마지막으로 갱신된 이름을 그대로 읽어 보고.
        while (bIsRunning)
        {
            if (GetAsyncKeyState(VK_END) & 1)
            {
                break;
            }

            // 매 프레임 활성화된 메모리 치트들을 적용 (Track 1: MemoryCheats).
            // 새 메모리 치트는 여기에 한 줄씩 추가.
            JBU_PHASE_Q("Tick:GodMode");             GodMode::Tick();
            JBU_PHASE_Q("Tick:PlayerCheats");        PlayerCheats::Tick();
            JBU_PHASE_Q("Tick:TechCheats");          TechCheats::Tick();
            JBU_PHASE_Q("Tick:InventoryCheats");     InventoryCheats::Tick();
            JBU_PHASE_Q("Tick:StatBoost");           StatBoost::Tick();
            JBU_PHASE_Q("Tick:SocialCheats");        SocialCheats::Tick();
            JBU_PHASE_Q("Tick:PalCheats");           PalCheats::Tick();
            JBU_PHASE_Q("Tick:ItemDurabilityCheats");ItemDurabilityCheats::Tick();

            // Track 2 후킹 중 매 프레임 toggle 동기화가 필요한 것만 호출.
            // (포인터 캡처 후킹은 토글 없이 항상 ON 이라 Tick 불필요.)
            JBU_PHASE_Q("Tick:TemperatureHook");     TemperatureHook::Tick();
            JBU_PHASE_Q("Tick:StaminaSubHook");      StaminaSubHook::Tick();
            JBU_PHASE_Q("Tick:FoodHungerHook");      FoodHungerHook::Tick();
            JBU_PHASE_Q("Tick:FoodTimerHook");       FoodTimerHook::Tick();
            JBU_PHASE_Q("Tick:WriteItemsHook");      WriteItemsHook::Tick();
            JBU_PHASE_Q("Tick:DurabilityWriteHook");DurabilityWriteHook::Tick();
            JBU_PHASE_Q("Tick:AmmoDecreaseHook");    AmmoDecreaseHook::Tick();
            JBU_PHASE_Q("Tick:BuildAnywhereHook");   BuildAnywhereHook::Tick();
            JBU_PHASE_Q("Tick:CanCatchBossHook");    CanCatchBossHook::Tick();
            JBU_PHASE_Q("Tick:PalStatsCreationHook");PalStatsCreationHook::Tick();
            JBU_PHASE_Q("Tick:ResearchIncreaseHook");ResearchIncreaseHook::Tick();
            JBU_PHASE_Q("Tick:Sleep");

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
