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
    };

    extern Settings Config;
}
