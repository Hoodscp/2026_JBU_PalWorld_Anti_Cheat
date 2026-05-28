#pragma once

// [Track 2 / P1] 인벤토리 오픈 시 현재 EXP 포인터 캡처. 캡처된 rcx + 0x338 가
// 현재 경험치 객체. Track 1 의 Exp 변조 백업 경로.
namespace ExpPtrCaptureHook
{
    bool Install();
    void Shutdown();

    void* GetCapturedThis();
}
