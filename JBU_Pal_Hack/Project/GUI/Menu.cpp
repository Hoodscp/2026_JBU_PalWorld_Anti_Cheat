#include "Menu.h"
#include <imgui.h>

namespace Menu
{
    Settings Config;

    void Render()
    {
        if (!Config.bShowMenu)
            return;

        // 예쁘고 투명도가 있는 프리미엄 디자인
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        
        ImGui::Begin("JBU Palworld Trainer", &Config.bShowMenu, ImGuiWindowFlags_NoCollapse);
        
        ImGui::Text("Press 'INSERT' to toggle this menu.");
        ImGui::Separator();

        ImGui::Text("Player Cheats");
        ImGui::Checkbox("God Mode", &Config.bGodMode);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Grants infinite health.");
        }
        
        ImGui::Checkbox("Infinite Stamina", &Config.bInfiniteStamina);
        ImGui::Checkbox("Infinite Ammo", &Config.bInfiniteAmmo);

        ImGui::Separator();
        ImGui::Text("Status: Injection Successful");

        ImGui::End();
    }
}
