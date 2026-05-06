#include "Menu.h"
#include "../SDK/Pal.h"
#include <imgui.h>

namespace Menu
{
    Settings Config;

    static void DrawPlayerSection()
    {
        ImGui::SeparatorText("Player");
        ImGui::Checkbox("God Mode", &Config.bGodMode);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("HP 를 매 프레임 큰 값으로 덮어씁니다.");

        ImGui::Checkbox("Infinite Stamina (SP)", &Config.bInfiniteStamina);
        ImGui::Checkbox("Infinite Ammo",         &Config.bInfiniteAmmo);
        ImGui::Checkbox("No Hunger (FullStomach)", &Config.bNoHunger);
        ImGui::Checkbox("Shield Boost (ShieldHP)", &Config.bShieldBoost);
        ImGui::Checkbox("Free Status Points",      &Config.bFreeStatusPoint);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("UnusedStatusPoint 를 999 로 유지 → 무한 분배");
    }

    static void DrawTechSection()
    {
        ImGui::SeparatorText("Technology");
        ImGui::Checkbox("Free Technology Points", &Config.bFreeTechPoint);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("TechnologyPoint 를 9999 로 유지");

        int tp = SDK::GetLocalTechnologyPoint();
        if (tp >= 0) ImGui::Text("Current TechPoint: %d", tp);
        else         ImGui::TextDisabled("Current TechPoint: <unavailable>");
    }

    static void DrawInventorySection()
    {
        ImGui::SeparatorText("Inventory");
        ImGui::Checkbox("Force Item StackCount", &Config.bForceItemStack);
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Container Index", &Config.SelectedContainerIdx);
        if (Config.SelectedContainerIdx < 0) Config.SelectedContainerIdx = 0;
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Slot Index", &Config.SelectedSlotIndex);
        if (Config.SelectedSlotIndex < 0) Config.SelectedSlotIndex = 0;
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Stack Count", &Config.TargetStackCount);

        int curStack = SDK::GetItemSlotStackCount(Config.SelectedContainerIdx, Config.SelectedSlotIndex);
        if (curStack >= 0) ImGui::Text("Container[%d].Slot[%d] StackCount: %d",
                                       Config.SelectedContainerIdx, Config.SelectedSlotIndex, curStack);
        else               ImGui::TextDisabled("Container[%d].Slot[%d]: empty / out-of-range",
                                                Config.SelectedContainerIdx, Config.SelectedSlotIndex);
    }

    static void DrawStatusSection()
    {
        ImGui::SeparatorText("Real-Time Memory Status");
        int64_t hp     = SDK::GetLocalPlayerHealth();
        int64_t maxHp  = SDK::GetLocalPlayerMaxHP();
        int64_t sp     = SDK::GetLocalPlayerStamina();
        float   stom   = SDK::GetLocalPlayerFullStomach();
        float   stomMx = SDK::GetLocalPlayerMaxFullStomach();
        int64_t shield = SDK::GetLocalPlayerShieldHP();
        int     usp    = SDK::GetLocalPlayerUnusedStatusPoint();

        if (hp != -1) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Local Player Found!");
            ImGui::Text("HP    : %lld / %lld", hp, maxHp);
            ImGui::Text("SP    : %lld", sp);
            ImGui::Text("Food  : %.1f / %.1f", stom, stomMx);
            ImGui::Text("Shield: %lld", shield);
            ImGui::Text("UnusedStatusPoint: %d", usp);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Player Not Found in World.");
        }
    }

    void Render()
    {
        if (!Config.bShowMenu) return;

        ImGui::SetNextWindowSize(ImVec2(440, 640), ImGuiCond_FirstUseEver);
        ImGui::Begin("JBU Palworld Trainer", &Config.bShowMenu, ImGuiWindowFlags_NoCollapse);

        ImGui::Text("Press 'INSERT' to toggle this menu.");

        DrawPlayerSection();
        DrawTechSection();
        DrawInventorySection();
        DrawStatusSection();

        ImGui::Separator();
        ImGui::Text("Status: Injection Successful");

        ImGui::End();
    }
}
