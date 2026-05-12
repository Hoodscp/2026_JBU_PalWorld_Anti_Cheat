#include "Menu.h"
#include "../SDK/Pal.h"
#include <imgui.h>

namespace Menu
{
    Settings Config;

    // ─────────────────────────────────────────────────────────────────
    //                          Player 탭
    // ─────────────────────────────────────────────────────────────────
    static void DrawPlayerTab()
    {
        ImGui::SeparatorText("Toggles");
        ImGui::Checkbox("God Mode",                &Config.bGodMode);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("HP 를 매 프레임 큰 값으로 덮어씁니다.");
        ImGui::Checkbox("Infinite Stamina (SP)",   &Config.bInfiniteStamina);
        ImGui::Checkbox("Infinite Ammo",           &Config.bInfiniteAmmo);
        ImGui::Checkbox("No Hunger (FullStomach)", &Config.bNoHunger);
        ImGui::Checkbox("Shield Boost (ShieldHP)", &Config.bShieldBoost);
        ImGui::Checkbox("Free Status Points",      &Config.bFreeStatusPoint);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("UnusedStatusPoint 를 999 로 유지 → 무한 분배");

        ImGui::SeparatorText("Live Stats");
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

    // ─────────────────────────────────────────────────────────────────
    //                       Stat Boost 탭
    // ─────────────────────────────────────────────────────────────────
    static void DrawStatBoostTab()
    {
        ImGui::SeparatorText("Toggles");
        ImGui::Checkbox("Max Sanity (=100)", &Config.bMaxSanity);
        ImGui::Checkbox("Infinite MP",       &Config.bInfiniteMP);
        ImGui::Checkbox("Max Talents (IV)",  &Config.bMaxTalents);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(
            "Talent_HP/Melee/Shot/Defense 를 100 으로 고정합니다.");

        ImGui::Checkbox("Force Level", &Config.bForceLevel);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("##LevelVal", &Config.TargetLevel);
        if (Config.TargetLevel < 1)   Config.TargetLevel = 1;
        if (Config.TargetLevel > 255) Config.TargetLevel = 255;

        ImGui::Checkbox("Force Exp", &Config.bForceExp);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160);
        ImGui::InputScalar("##ExpVal", ImGuiDataType_S64, &Config.TargetExp);
        if (Config.TargetExp < 0) Config.TargetExp = 0;

        ImGui::SeparatorText("Live Stats");
        int  lv  = SDK::GetLocalPlayerLevel();
        long long ex   = SDK::GetLocalPlayerExp();
        float    san  = SDK::GetLocalPlayerSanity();
        long long mp   = SDK::GetLocalPlayerMP();
        long long mxmp = SDK::GetLocalPlayerMaxMP();
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

    // ─────────────────────────────────────────────────────────────────
    //                       Tech 탭
    // ─────────────────────────────────────────────────────────────────
    static void DrawTechTab()
    {
        ImGui::SeparatorText("Toggles");
        ImGui::Checkbox("Free Technology Points",      &Config.bFreeTechPoint);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("TechnologyPoint 를 9999 로 유지");
        ImGui::Checkbox("Free Boss Technology Points", &Config.bFreeBossTechPoint);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("BossTechnologyPoint(0x154) 를 9999 로 유지");

        ImGui::SeparatorText("Live Stats");
        int tp  = SDK::GetLocalTechnologyPoint();
        int btp = SDK::GetLocalBossTechnologyPoint();
        if (tp >= 0)  ImGui::Text("Current TechPoint     : %d", tp);
        else          ImGui::TextDisabled("Current TechPoint     : <unavailable>");
        if (btp >= 0) ImGui::Text("Current BossTechPoint : %d", btp);
        else          ImGui::TextDisabled("Current BossTechPoint : <unavailable>");
    }

    // ─────────────────────────────────────────────────────────────────
    //                       Social 탭
    // ─────────────────────────────────────────────────────────────────
    static void DrawSocialTab()
    {
        ImGui::SeparatorText("Toggles");
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

        ImGui::SeparatorText("Live Stats");
        int fp  = SDK::GetLocalPlayerFriendshipPoint();
        int arp = SDK::GetLocalPlayerArenaRankPoint();
        if (fp  >= 0) ImGui::Text("Friendship    : %d", fp);
        else          ImGui::TextDisabled("Friendship    : <unavailable>");
        if (arp >= 0) ImGui::Text("Arena RankPt  : %d", arp);
        else          ImGui::TextDisabled("Arena RankPt  : <unavailable>");
    }

    // ─────────────────────────────────────────────────────────────────
    //                       Pals 탭 (보유 팰)
    // ─────────────────────────────────────────────────────────────────
    static void DrawPalsTab()
    {
        const int slotCount = SDK::GetPalSlotCount();

        ImGui::SeparatorText("Selection");
        if (slotCount <= 0) {
            ImGui::TextDisabled("PalStorage not loaded (창고가 동기화되지 않음).");
            ImGui::TextDisabled("팰박스를 한 번 열거나 본거지로 이동한 뒤 다시 시도하세요.");
            return;
        }
        ImGui::Text("Total slots: %d", slotCount);
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Pal Slot Index", &Config.SelectedPalSlot);
        if (Config.SelectedPalSlot < 0)            Config.SelectedPalSlot = 0;
        if (Config.SelectedPalSlot >= slotCount)   Config.SelectedPalSlot = slotCount - 1;

        const int slot = Config.SelectedPalSlot;
        const bool empty = SDK::IsPalSlotEmpty(slot);

        if (empty) {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f),
                               "Slot[%d] is EMPTY — 토글이 무시됩니다.", slot);
        } else {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                               "Slot[%d] occupied.", slot);
        }

        ImGui::SeparatorText("Toggles");
        ImGui::Checkbox("Pal God Mode (HP = 1e9)", &Config.bPalGodMode);
        ImGui::Checkbox("Pal Max Sanity",          &Config.bPalMaxSanity);
        ImGui::Checkbox("Pal Infinite MP",         &Config.bPalInfiniteMP);
        ImGui::Checkbox("Pal Max Talents (IV)",    &Config.bPalMaxTalents);

        ImGui::Checkbox("Pal Force Level", &Config.bPalForceLevel);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("##PalLevelVal", &Config.PalTargetLevel);
        if (Config.PalTargetLevel < 1)   Config.PalTargetLevel = 1;
        if (Config.PalTargetLevel > 255) Config.PalTargetLevel = 255;

        ImGui::Checkbox("Pal Force Exp", &Config.bPalForceExp);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160);
        ImGui::InputScalar("##PalExpVal", ImGuiDataType_S64, &Config.PalTargetExp);
        if (Config.PalTargetExp < 0) Config.PalTargetExp = 0;

        ImGui::SeparatorText("Live Stats");
        if (empty) {
            ImGui::TextDisabled("<empty>");
            return;
        }
        int      lv   = SDK::GetPalLevel(slot);
        long long ex  = SDK::GetPalExp(slot);
        long long hp  = SDK::GetPalHP(slot);
        long long mxh = SDK::GetPalMaxHP(slot);
        long long mp  = SDK::GetPalMP(slot);
        float     san = SDK::GetPalSanity(slot);
        int thp = SDK::GetPalTalentHP(slot);
        int tm  = SDK::GetPalTalentMelee(slot);
        int ts  = SDK::GetPalTalentShot(slot);
        int td  = SDK::GetPalTalentDefense(slot);

        if (lv  >= 0) ImGui::Text("Level   : %d", lv);
        if (ex  >= 0) ImGui::Text("Exp     : %lld", ex);
        if (hp  >= 0) ImGui::Text("HP      : %lld / %lld", hp, mxh);
        if (mp  >= 0) ImGui::Text("MP      : %lld", mp);
        if (san >= 0) ImGui::Text("Sanity  : %.1f", san);
        if (thp >= 0) ImGui::Text("Talents : HP %d / Mel %d / Shot %d / Def %d", thp, tm, ts, td);
    }

    // ─────────────────────────────────────────────────────────────────
    //                       Environment 탭
    // ─────────────────────────────────────────────────────────────────
    static void DrawEnvironmentTab()
    {
        ImGui::SeparatorText("Toggles");
        ImGui::Checkbox("Always Normal Temperature", &Config.bForceNormalTemp);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(
            "온도 상태 변경 명령(0x138 멤버 write)을 가로채 새 값을 0(Normal)으로 강제합니다.\n"
            "지역 진입/장비 변경 등으로 온도 단계가 바뀌려 할 때만 동작 → 매우 가벼움.");
    }

    // ─────────────────────────────────────────────────────────────────
    //                       Inventory 탭
    // ─────────────────────────────────────────────────────────────────
    static void DrawInventoryTab()
    {
        ImGui::SeparatorText("Force Stack");
        ImGui::Checkbox("Force Item StackCount", &Config.bForceItemStack);
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Container Index", &Config.SelectedContainerIdx);
        if (Config.SelectedContainerIdx < 0) Config.SelectedContainerIdx = 0;
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Slot Index", &Config.SelectedSlotIndex);
        if (Config.SelectedSlotIndex < 0) Config.SelectedSlotIndex = 0;
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Stack Count", &Config.TargetStackCount);

        ImGui::SeparatorText("Live");
        int curStack = SDK::GetItemSlotStackCount(Config.SelectedContainerIdx, Config.SelectedSlotIndex);
        if (curStack >= 0) ImGui::Text("Container[%d].Slot[%d] StackCount: %d",
                                       Config.SelectedContainerIdx, Config.SelectedSlotIndex, curStack);
        else               ImGui::TextDisabled("Container[%d].Slot[%d]: empty / out-of-range",
                                                Config.SelectedContainerIdx, Config.SelectedSlotIndex);
    }

    // ─────────────────────────────────────────────────────────────────
    void Render()
    {
        if (!Config.bShowMenu) return;

        ImGui::SetNextWindowSize(ImVec2(520, 540), ImGuiCond_FirstUseEver);
        ImGui::Begin("JBU Palworld Trainer", &Config.bShowMenu, ImGuiWindowFlags_NoCollapse);

        ImGui::Text("Press 'INSERT' to toggle this menu.");
        ImGui::Separator();

        if (ImGui::BeginTabBar("##MainTabs", ImGuiTabBarFlags_Reorderable)) {
            if (ImGui::BeginTabItem("Player"))      { DrawPlayerTab();      ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Stat Boost"))  { DrawStatBoostTab();   ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Tech"))        { DrawTechTab();        ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Social"))      { DrawSocialTab();      ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Pals"))        { DrawPalsTab();        ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Environment")) { DrawEnvironmentTab(); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Inventory"))   { DrawInventoryTab();   ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        }

        ImGui::Separator();
        ImGui::Text("Status: Injection Successful");

        ImGui::End();
    }
}
