#pragma once

namespace CursorHooks
{
    // user32의 SetCursorPos / ClipCursor를 후킹하여 메뉴/Free Mouse 활성 시
    // 게임이 매 프레임 커서를 화면 중앙으로 끌어다 놓는 동작을 차단합니다.
    // 정리는 HookManager::Shutdown의 MH_DisableHook(MH_ALL_HOOKS)이 처리.
    bool Install();
}
