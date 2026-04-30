#include "CursorHooks.h"
#include "HookManager.h"
#include "../GUI/Menu.h"
#include <windows.h>
#include <iostream>

namespace CursorHooks
{
    using SetCursorPos_t = BOOL (WINAPI*)(int, int);
    using ClipCursor_t   = BOOL (WINAPI*)(const RECT*);

    static SetCursorPos_t oSetCursorPos = nullptr;
    static ClipCursor_t   oClipCursor   = nullptr;

    static bool ShouldUnlock() {
        return Menu::Config.bShowMenu || Menu::Config.bFreeMouse;
    }

    static BOOL WINAPI hkSetCursorPos(int X, int Y) {
        if (ShouldUnlock()) return TRUE; // no-op: 게임의 중앙 고정 차단
        return oSetCursorPos(X, Y);
    }

    static BOOL WINAPI hkClipCursor(const RECT* lpRect) {
        if (ShouldUnlock()) return oClipCursor(nullptr); // 클립 강제 해제
        return oClipCursor(lpRect);
    }

    bool Install()
    {
        HMODULE user32 = GetModuleHandleA("user32.dll");
        if (!user32) {
            std::cout << "[-] CursorHooks: user32.dll not loaded\n";
            return false;
        }

        void* pSetCursorPos = (void*)GetProcAddress(user32, "SetCursorPos");
        void* pClipCursor   = (void*)GetProcAddress(user32, "ClipCursor");
        if (!pSetCursorPos || !pClipCursor) {
            std::cout << "[-] CursorHooks: failed to resolve user32 exports\n";
            return false;
        }

        const bool ok =
            HookManager::CreateHook(pSetCursorPos, &hkSetCursorPos, (void**)&oSetCursorPos) &&
            HookManager::CreateHook(pClipCursor,   &hkClipCursor,   (void**)&oClipCursor);

        std::cout << (ok ? "[+] CursorHooks installed (SetCursorPos, ClipCursor)\n"
                         : "[-] CursorHooks install failed\n");
        return ok;
    }
}
