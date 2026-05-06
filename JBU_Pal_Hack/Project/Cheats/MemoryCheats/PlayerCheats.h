#pragma once

// 플레이어 본체 관련 메모리 치트 모음 (Track 1).
// 각 Tick() 은 매 프레임 호출되며 Menu::Config 의 토글에 따라
// HP/Stamina/FullStomach/ShieldHP/UnusedStatusPoint 를 강제로 덮어씁니다.
namespace PlayerCheats
{
    void Tick();
}
