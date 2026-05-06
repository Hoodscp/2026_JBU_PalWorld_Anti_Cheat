#pragma once

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
        bool bFreeTechPoint   = false; // TechnologyPoint = 9999

        // ── 신규: 인벤토리 ──
        bool bForceItemStack    = false;
        int  SelectedSlotIndex  = 0;
        int  TargetStackCount   = 9999;

        // ── 신규: 환경 ──
        bool bForceTemperature  = false;
        int  TargetTemperature  = 22; // ℃
    };

    extern Settings Config;
}
