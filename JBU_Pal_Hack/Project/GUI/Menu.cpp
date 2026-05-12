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

        ImGui::Checkbox("Free Boss Technology Points", &Config.bFreeBossTechPoint);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("BossTechnologyPoint(0x154) 를 9999 로 유지");

        int tp  = SDK::GetLocalTechnologyPoint();
        int btp = SDK::GetLocalBossTechnologyPoint();
        if (tp >= 0)  ImGui::Text("Current TechPoint     : %d", tp);
        else          ImGui::TextDisabled("Current TechPoint     : <unavailable>");
        if (btp >= 0) ImGui::Text("Current BossTechPoint : %d", btp);
        else          ImGui::TextDisabled("Current BossTechPoint : <unavailable>");
    }

    static void DrawStatBoostSection()
    {
        ImGui::SeparatorText("Stat Boost");

        ImGui::Checkbox("Max Sanity (=100)", &Config.bMaxSanity);
        ImGui::Checkbox("Infinite MP",       &Config.bInfiniteMP);
        ImGui::Checkbox("Max Talents (IV)",  &Config.bMaxTalents);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(
            "Talent_HP/Melee/Shot/Defense 를 100 으로 고정합니다.\n"
            "본인 캐릭터 슬롯의 SaveParameter 에 적용 → 데미지/HP 계수 상승.");

        ImGui::Checkbox("Force Level", &Config.bForceLevel);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("##LevelVal", &Config.TargetLevel);
        if (Config.TargetLevel < 1)   Config.TargetLevel = 1;
        if (Config.TargetLevel > 255) Config.TargetLevel = 255;

        ImGui::Checkbox("Force Exp", &Config.bForceExp);
        ImGui::SameLine();
        // ImGui 는 int64 InputScalar 가 별도 함수. int 64 InputScalar 로 표시.
        ImGui::SetNextItemWidth(160);
        ImGui::InputScalar("##ExpVal", ImGuiDataType_S64, &Config.TargetExp);
        if (Config.TargetExp < 0) Config.TargetExp = 0;

        // 실시간 상태 표시
        int  lv  = SDK::GetLocalPlayerLevel();
        long long ex = SDK::GetLocalPlayerExp();
        float san = SDK::GetLocalPlayerSanity();
        long long mp  = SDK::GetLocalPlayerMP();
        long long mxmp= SDK::GetLocalPlayerMaxMP();
        int  thp = SDK::GetLocalPlayerTalentHP();
        int  tm  = SDK::GetLocalPlayerTalentMelee();
        int  ts  = SDK::GetLocalPlayerTalentShot();
        int  td  = SDK::GetLocalPlayerTalentDefense();

        if (lv  >= 0) ImGui::Text("Level   : %d", lv);
        if (ex  >= 0) ImGui::Text("Exp     : %lld", ex);
        if (san >= 0) ImGui::Text("Sanity  : %.1f", san);
        if (mp  >= 0) ImGui::Text("MP      : %lld / %lld", mp, mxmp);
        if (thp >= 0) ImGui::Text("Talents : HP %d / Mel %d / Shot %d / Def %d", thp, tm, ts, td);
    }

    static void DrawSocialSection()
    {
        ImGui::SeparatorText("Social / Arena");

        ImGui::Checkbox("Force Friendship", &Config.bForceFriendship);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("##FpVal", &Config.TargetFriendship);
        if (Config.TargetFriendship < 0) Config.TargetFriendship = 0;

        ImGui::Checkbox("Force Arena RP", &Config.bForceArenaRP);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("##ArpVal", &Config.TargetArenaRP);
        if (Config.TargetArenaRP < 0) Config.TargetArenaRP = 0;

        int fp  = SDK::GetLocalPlayerFriendshipPoint();
        int arp = SDK::GetLocalPlayerArenaRankPoint();
        if (fp  >= 0) ImGui::Text("Friendship    : %d", fp);
        else          ImGui::TextDisabled("Friendship    : <unavailable>");
        if (arp >= 0) ImGui::Text("Arena RankPt  : %d", arp);
        else          ImGui::TextDisabled("Arena RankPt  : <unavailable>");
    }

    static void DrawEnvironmentSection()
    {
        ImGui::SeparatorText("Environment");
        ImGui::Checkbox("Always Normal Temperature", &Config.bForceNormalTemp);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(
            "온도 상태 변경 명령(0x138 멤버 write)을 가로채 새 값을 0(Normal)으로 강제합니다.\n"
            "지역 진입/장비 변경 등으로 온도 단계가 바뀌려 할 때만 동작 → 매우 가벼움.");
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
        DrawStatBoostSection();
        DrawTechSection();
        DrawSocialSection();
        DrawEnvironmentSection();
        DrawInventorySection();
        DrawStatusSection();

        ImGui::Separator();
        ImGui::Text("Status: Injection Successful");

        ImGui::End();
    }
}
