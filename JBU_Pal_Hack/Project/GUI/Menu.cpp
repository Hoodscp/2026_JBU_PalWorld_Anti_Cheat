#include "Menu.h"
#include "../SDK/Pal.h"
#include <imgui.h>

namespace Menu
{
    Settings Config;

    void Render()
    {
        if (!Config.bShowMenu)
            return;

        ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_FirstUseEver);
        
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

        // ----------------------------------------------------
        // 모듈화된 메모리 접근 구조 반영
        // ----------------------------------------------------
        ImGui::Text("[ Real-Time Memory Status ]");
        
        // GUI는 오직 SDK에게 데이터만 받아옵니다 (오프셋이나 메모리 계산 전혀 알 필요 없음)
        int64_t playerHp = SDK::GetLocalPlayerHealth();
        
        if (playerHp != -1) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Local Player Found!");
            ImGui::Text("Raw HP Value: %lld", playerHp);
            ImGui::Text("Display HP: %.1f", (float)(playerHp / 1000.0f));
            // 치트 적용은 Cheats::Tick()이 매 프레임 처리 (메뉴 닫혀도 동작 유지).
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Player Not Found in World.");
        }

        ImGui::Separator();
        ImGui::Text("Status: Injection Successful");

        ImGui::End();
    }
}
