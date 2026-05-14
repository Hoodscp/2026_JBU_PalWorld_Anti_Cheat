#include "Menu.h"
#include "../SDK/Pal.h"
#include <imgui.h>
#include <windows.h>  // IsBadReadPtr (ContainerId 안전 읽기용)

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
        ImGui::SeparatorText("Source");
        ImGui::RadioButton("Pal Box (Storage)", &Config.PalSource, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Party (Otomo)",     &Config.PalSource, 1);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(
            "Party 컨테이너는 OtomoHook (UPalOtomoHolderComponentBase AOB 후크) 가\n"
            "자동 캡쳐합니다. 후크 시그니처가 비어 있을 땐 아래 hex 입력으로 직접 지정.");

        if (Config.PalSource == 1) {
            // OtomoHook 이 캡쳐한 컨테이너 표시. 캡쳐 전엔 0 → 안내 메시지.
            const uintptr_t captured = SDK::GetOtomoContainerOverride();
            if (captured) {
                // 자동 캡쳐 결과를 UI 입력란에도 반영 (시각적 일관성).
                Config.OtomoContainerAddress = captured;
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                                   "Captured: 0x%llX", (unsigned long long)captured);

                // 잡힌 컨테이너의 ContainerId(+0x38, 16 byte) 표시. 사용자는
                // 이걸 CE 등으로 잡은 로컬 OtomoCharacterContainerId 와 직접
                // 비교해 "내 컨테이너가 맞는지" 검증 가능. 두 값이 다르면
                // 다른 OtomoHolder 가 잘못 잡힌 것 → Clear 후 재시도.
                uint64_t idLo = 0, idHi = 0;
                if (!IsBadReadPtr((const void*)(captured + 0x38), 0x10)) {
                    idLo = *(const uint64_t*)(captured + 0x38);
                    idHi = *(const uint64_t*)(captured + 0x40);
                }
                ImGui::TextDisabled("ContainerId: %016llX %016llX",
                                    (unsigned long long)idHi,
                                    (unsigned long long)idLo);
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f),
                                   "Waiting for OtomoHook capture...");
                ImGui::TextDisabled("후크 시그니처가 비었거나 아직 컨테이너 생성 전.");
                ImGui::TextDisabled("아래 hex 입력란에 직접 주소를 넣으면 즉시 활성화됩니다.");
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear")) {
                SDK::SetOtomoContainerOverride(0);
                Config.OtomoContainerAddress = 0;
            }

            // 수동 hex 입력 (시그니처 미설정 시 / 후크가 잘못된 컨테이너를 잡았을
            // 때 강제 지정 / 외부 도구로 잡은 주소를 그대로 붙여넣는 fallback).
            ImGui::SetNextItemWidth(220);
            if (ImGui::InputScalar("Override Addr (hex)", ImGuiDataType_U64,
                                   &Config.OtomoContainerAddress, nullptr, nullptr, "%llX",
                                   ImGuiInputTextFlags_CharsHexadecimal)) {
                SDK::SetOtomoContainerOverride(Config.OtomoContainerAddress);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip(
                "0 이면 OtomoHook 의 다음 캡쳐를 기다림.\n"
                "수동 입력 시 즉시 등록됩니다.");
        }

        // Source 별 컨테이너 베이스 / 슬롯 카운트
        const uintptr_t container = (Config.PalSource == 1)
                                    ? SDK::GetOtomoContainerOverride()
                                    : SDK::GetLocalPalContainer();
        const int slotCount = SDK::GetSlotCountIn(container);

        ImGui::SeparatorText("Selection");
        if (!container || slotCount <= 0) {
            if (Config.PalSource == 0) {
                ImGui::TextDisabled("PalStorage not loaded (창고가 동기화되지 않음).");
                ImGui::TextDisabled("팰박스를 한 번 열거나 본거지로 이동한 뒤 다시 시도.");
            } else {
                ImGui::TextDisabled("Otomo 컨테이너가 아직 캡쳐되지 않았습니다.");
                ImGui::TextDisabled("팰을 한 번 소환/회수하면 OtomoHook 이 자동 등록합니다.");
                ImGui::TextDisabled("(또는 위 hex 입력란에 수동 주소 입력)");
            }
            return;
        }
        ImGui::Text("Container: 0x%llX   Slots: %d", (unsigned long long)container, slotCount);
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Pal Slot Index", &Config.SelectedPalSlot);
        if (Config.SelectedPalSlot < 0)            Config.SelectedPalSlot = 0;
        if (Config.SelectedPalSlot >= slotCount)   Config.SelectedPalSlot = slotCount - 1;

        const int  slot  = Config.SelectedPalSlot;
        const bool empty = SDK::GetIndividualParameterInContainer(container, slot) == 0;

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
        int       lv  = SDK::GetPalLevelIn        (container, slot);
        long long ex  = SDK::GetPalExpIn          (container, slot);
        long long hp  = SDK::GetPalHPIn           (container, slot);
        long long mxh = SDK::GetPalMaxHPIn        (container, slot);
        long long mp  = SDK::GetPalMPIn           (container, slot);
        float     san = SDK::GetPalSanityIn       (container, slot);
        int       thp = SDK::GetPalTalentHPIn     (container, slot);
        int       tm  = SDK::GetPalTalentMeleeIn  (container, slot);
        int       ts  = SDK::GetPalTalentShotIn   (container, slot);
        int       td  = SDK::GetPalTalentDefenseIn(container, slot);

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
        ImGui::SeparatorText("Target Slot");
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Container Index", &Config.SelectedContainerIdx);
        if (Config.SelectedContainerIdx < 0) Config.SelectedContainerIdx = 0;
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Slot Index", &Config.SelectedSlotIndex);
        if (Config.SelectedSlotIndex < 0) Config.SelectedSlotIndex = 0;

        ImGui::SeparatorText("Stack Count");
        ImGui::Checkbox("Force Item StackCount", &Config.bForceItemStack);
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Stack Count", &Config.TargetStackCount);

        int curStack = SDK::GetItemSlotStackCount(Config.SelectedContainerIdx, Config.SelectedSlotIndex);
        if (curStack >= 0) ImGui::Text("StackCount   : %d", curStack);
        else               ImGui::TextDisabled("StackCount   : <empty / out-of-range>");

        ImGui::SeparatorText("Food Freshness (Corruption = 0)");
        ImGui::Checkbox("Freeze Selected Slot",   &Config.bFreezeFoodFreshness);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(
            "선택 슬롯의 CorruptionProgressValue(+0x158) 를 매 프레임 0 으로 강제 → 부패 정지.");
        ImGui::Checkbox("Freeze ALL Slots in Container", &Config.bFreezeAllFoodFreshness);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(
            "현재 Container Index 의 슬롯 0~63 전부에 매 프레임 0 강제.\n"
            "출하/저장 컨테이너에 사용하면 음식 보존 자동화.");

        float curCorr = SDK::GetItemSlotCorruption(Config.SelectedContainerIdx, Config.SelectedSlotIndex);
        if (curCorr >= 0.0f) ImGui::Text("Corruption   : %.3f  (0=fresh, 1=spoiled)", curCorr);
        else                 ImGui::TextDisabled("Corruption   : <empty / out-of-range>");

        ImGui::SeparatorText("Equipment Durability / Bullets");
        ImGui::TextDisabled("선택된 Container/Slot 의 DynamicItemData 를 GUObjectArray 로 자동 해소.");
        ImGui::Checkbox("Force Durability = Max", &Config.bForceMaxDurability);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(
            "선택 슬롯이 장비(Armor/Weapon)일 때만 동작.\n"
            "MaxDurability 를 읽어 Durability(+0x78) 에 매 프레임 동기화.");
        ImGui::Checkbox("Force Remaining Bullets", &Config.bForceInfiniteBullets);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("##BulletsVal", &Config.TargetBullets);
        if (Config.TargetBullets < 0) Config.TargetBullets = 0;

        uintptr_t dynData = SDK::GetItemSlotDynamicData(Config.SelectedContainerIdx, Config.SelectedSlotIndex);
        if (dynData) {
            float dur = SDK::GetItemDurability   (Config.SelectedContainerIdx, Config.SelectedSlotIndex);
            float mx  = SDK::GetItemMaxDurability(Config.SelectedContainerIdx, Config.SelectedSlotIndex);
            int   bul = SDK::GetItemRemainingBullets(Config.SelectedContainerIdx, Config.SelectedSlotIndex);
            ImGui::Text("Dynamic Data : 0x%llX", (unsigned long long)dynData);
            ImGui::Text("Durability   : %.1f / %.1f", dur, mx);
            ImGui::Text("Bullets      : %d", bul);
        } else {
            ImGui::TextDisabled("Dynamic Data : <none>  (해당 슬롯이 비었거나 장비가 아님,");
            ImGui::TextDisabled("                       또는 GUObjectArray 미설정)");
        }
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
