#include "Menu.h"
// #include <imgui.h> // Uncomment after setting up ImGui

namespace Menu
{
    Settings Config;

    void Render()
    {
        if (!Config.bShowMenu)
            return;

        /*
        ImGui::Begin("JBU Palworld Trainer", &Config.bShowMenu);
        
        ImGui::Text("Player Cheats");
        ImGui::Checkbox("God Mode", &Config.bGodMode);
        ImGui::Checkbox("Infinite Stamina", &Config.bInfiniteStamina);
        ImGui::Checkbox("Infinite Ammo", &Config.bInfiniteAmmo);

        ImGui::End();
        */
    }
}
