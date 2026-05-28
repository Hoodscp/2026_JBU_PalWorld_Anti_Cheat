#pragma once

// [Track 2 / P1] Technology cost subtract 시점에서 rdi 캡처 (= UPalTechnologyData*).
// 캡처된 객체의 +0x150 = TechnologyPoint, +0x154 = BossTechnologyPoint.
// TechCheats(Track 1) 가 GWorld 깨질 때 백업 경로로 활용.
namespace TechPointCaptureHook
{
    bool Install();
    void Shutdown();

    void* GetCapturedThis();
}
