#pragma once

// [Track 2 / P0] Items Don't Decrease — mov [rbx+0x154], r14d (스택카운트 write)
// 무력화. 아이템 사용 시 수량 차감이 일어나지 않음.
// Menu::Config.bHookItemsNoDecrease 토글.
namespace WriteItemsHook
{
    bool Install();
    void Tick();
    void Shutdown();
}
