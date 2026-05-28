#pragma once

// [Track 2 / P1] Inventory 오픈 시 컨테이너 root 포인터 캡처 (rcx).
// 캡처된 값은 인벤토리 슬롯 배열을 가리키며, MultiHelper / Container 체인의
// GWorld 의존성을 우회할 수 있음. 토글 없음.
namespace ItemSlotCaptureHook
{
    bool Install();
    void Shutdown();

    void* GetCapturedThis();
}
