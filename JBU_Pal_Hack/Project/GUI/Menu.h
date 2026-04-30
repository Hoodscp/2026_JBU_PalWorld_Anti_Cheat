#pragma once

namespace Menu
{
    // Renders the ImGui Menu window
    void Render();

    // Cheat options state
    struct Settings {
        bool bGodMode = false;
        bool bInfiniteStamina = false;
        bool bInfiniteAmmo = false;
        bool bShowMenu = true;
        bool bFreeMouse = false; // 메뉴 닫혀도 커서 잠금 해제 유지
    };

    extern Settings Config;
}
