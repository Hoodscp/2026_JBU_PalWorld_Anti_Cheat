#pragma once

// FPalIndividualCharacterSaveParameter 위에 얹는 스탯 강제 치트 모음.
// - Sanity / MP / Talents : 토글 → 고정 상수 유지
// - Level / Exp           : 토글 → Menu::Config 의 InputInt 값으로 유지
namespace StatBoost
{
    void Tick();
}
