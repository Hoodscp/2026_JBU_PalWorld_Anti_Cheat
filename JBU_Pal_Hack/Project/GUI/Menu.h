#pragma once
#include <cstdint>

namespace Menu
{
    // Renders the ImGui Menu window
    void Render();

    // Cheat options state
    struct Settings {
        // ── 기존 ──
        bool bGodMode         = false;
        bool bInfiniteStamina = false;
        bool bInfiniteAmmo    = false;
        bool bShowMenu        = true;

        // ── 신규: 플레이어 본체 ──
        bool bNoHunger        = false; // FullStomach 강제 채움
        bool bShieldBoost     = false; // ShieldHP 강제 채움
        bool bFreeStatusPoint = false; // UnusedStatusPoint = 999

        // ── 신규: 자원/연구 ──
        bool bFreeTechPoint     = false; // TechnologyPoint = 9999
        bool bFreeBossTechPoint = false; // BossTechnologyPoint = 9999

        // ── 신규: 환경 (Track 2 mid-function patch) ──
        bool bForceNormalTemp = false; // 온도 상태 변경 명령에서 새 값 → 0(Normal) 강제

        // ── Track 2 / P0 AOB NOP 패치 (HookCheats/*Hook.cpp) ──
        // Track 1 의 폴링 방식과 별도 토글. 둘 다 켜놔도 무방 (서로 다른 메커니즘).
        bool bHookInfStamina        = false; // StaminaSubHook: subss xmm0,xmm2 무력화
        bool bHookNoHunger          = false; // FoodHungerHook: subss xmm2,xmm7 무력화
        bool bHookFreezeFoodTimer   = false; // FoodTimerHook: movss [rbx+0x158] write 무력화
        bool bHookItemsNoDecrease   = false; // WriteItemsHook: mov [rbx+0x154],r14d 무력화
        bool bHookInfDurability     = false; // DurabilityWriteHook: movss [rcx+0x10] 무력화
        bool bHookInfAmmo           = false; // AmmoDecreaseHook: lea/mov 쌍 통째로 NOP

        // ── Track 2 / P2 AOB 조건분기 강제 + 명령어 주입 ──
        bool bHookBuildAnywhere     = false; // BuildAnywhereHook: buildableTest 통과 강제
        bool bHookCanCatchBoss      = false; // CanCatchBossHook: 보스 차단 플래그 0 강제
        bool bHookPerfectPalStats   = false; // PalStatsCreationHook: 새 펄 IV 100 강제
        bool bHookQuickResearch     = false; // ResearchIncreaseHook: 진행도 제곱 가속

        // ── 신규: 인벤토리 ──
        bool bForceItemStack       = false;
        int  SelectedContainerIdx  = 0;
        int  SelectedSlotIndex     = 0;
        int  TargetStackCount      = 9999;

        // 음식 부패 진행도(0=신선) 동결
        bool bFreezeFoodFreshness     = false; // 선택된 단일 슬롯
        bool bFreezeAllFoodFreshness  = false; // 컨테이너 전체 슬롯 0~63

        // 장비 내구도 / 무기 잔탄 — 선택된 Container/Slot 슬롯의 DynamicItemData
        // 를 GUObjectArray 로 해소해 자동 접근. manual hex 입력은 제거됨.
        bool bForceMaxDurability    = false;  // Durability = MaxDurability
        bool bForceInfiniteBullets  = false;  // RemainingBullets = TargetBullets
        int  TargetBullets          = 999;

        // ── 신규: 스탯 부스트 (StatBoost.cpp) ──
        bool    bMaxSanity   = false;   // SanityValue = 100
        bool    bInfiniteMP  = false;   // MP = 1e9 (FFixedPoint64)
        bool    bMaxTalents  = false;   // Talent_HP/Melee/Shot/Defense = 100
        bool    bForceLevel  = false;
        int     TargetLevel  = 50;      // 1~255
        bool    bForceExp    = false;
        int64_t TargetExp    = 1'000'000;

        // ── 신규: 소셜/아레나 (SocialCheats.cpp) ──
        bool bForceFriendship = false;
        int  TargetFriendship = 9999;
        bool bForceArenaRP    = false;
        int  TargetArenaRP    = 9999;

        // ── 신규: 보유 팰 (PalCheats.cpp) — Pal Box 전용 ──
        int     SelectedPalSlot   = 0;
        bool    bPalGodMode       = false;   // HP = 1e9
        bool    bPalMaxSanity     = false;
        bool    bPalInfiniteMP    = false;
        bool    bPalMaxTalents    = false;
        bool    bPalForceLevel    = false;
        int     PalTargetLevel    = 50;
        bool    bPalForceExp      = false;
        int64_t PalTargetExp      = 1'000'000;
    };

    extern Settings Config;
}
