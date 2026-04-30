#include "CursorHooks.h"
#include "HookManager.h"
#include "../GUI/Menu.h"
#include <windows.h>
#include <iostream>
#include <vector>

namespace CursorHooks
{
    using SetCursorPos_t            = BOOL (WINAPI*)(int, int);
    using ClipCursor_t              = BOOL (WINAPI*)(const RECT*);
    using RegisterRawInputDevices_t = BOOL (WINAPI*)(PCRAWINPUTDEVICE, UINT, UINT);

    static SetCursorPos_t            oSetCursorPos            = nullptr;
    static ClipCursor_t              oClipCursor              = nullptr;
    static RegisterRawInputDevices_t oRegisterRawInputDevices = nullptr;

    static BOOL WINAPI hkSetCursorPos(int X, int Y) {
        if (Menu::Config.bShowMenu) return TRUE; // no-op: 게임의 중앙 고정 차단
        return oSetCursorPos(X, Y);
    }

    static BOOL WINAPI hkClipCursor(const RECT* lpRect) {
        if (Menu::Config.bShowMenu) return oClipCursor(nullptr); // 클립 강제 해제
        return oClipCursor(lpRect);
    }

    // UE5는 RIDEV_NOLEGACY로 raw input을 등록해 표준 WM_MOUSEMOVE/WM_LBUTTONDOWN
    // 흐름을 차단합니다. ImGui Win32 백엔드는 표준 메시지에 의존하므로 클릭이
    // ImGui로 전달되지 않습니다. NOLEGACY를 떼어내면 게임 카메라용 raw input은
    // 그대로 동작하면서 표준 메시지도 함께 발사됩니다.
    static BOOL WINAPI hkRegisterRawInputDevices(PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize) {
        if (!pRawInputDevices || uiNumDevices == 0) {
            return oRegisterRawInputDevices(pRawInputDevices, uiNumDevices, cbSize);
        }
        std::vector<RAWINPUTDEVICE> devices(pRawInputDevices, pRawInputDevices + uiNumDevices);
        for (auto& d : devices) {
            d.dwFlags &= ~RIDEV_NOLEGACY;
        }
        return oRegisterRawInputDevices(devices.data(), uiNumDevices, cbSize);
    }

    // 인젝션 시점엔 게임이 이미 NOLEGACY로 등록을 마친 상태이므로, 현재 등록
    // 정보를 읽어 NOLEGACY를 떼고 재등록해서 즉시 표준 메시지를 부활시킵니다.
    static void ForceLegacyMouseMessages() {
        UINT count = 0;
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1)) return;
        if (count == 0) return;

        std::vector<RAWINPUTDEVICE> devices(count);
        if (GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) == UINT(-1)) return;

        bool needsRefresh = false;
        for (auto& d : devices) {
            if (d.dwFlags & RIDEV_NOLEGACY) {
                d.dwFlags &= ~RIDEV_NOLEGACY;
                needsRefresh = true;
            }
        }
        if (!needsRefresh) {
            std::cout << "[+] CursorHooks: no NOLEGACY registrations found (" << count << " device(s) checked)\n";
            return;
        }

        // hook을 우회해 원본 직접 호출 (이미 NOLEGACY를 떼어낸 상태)
        if (oRegisterRawInputDevices(devices.data(), count, sizeof(RAWINPUTDEVICE))) {
            std::cout << "[+] CursorHooks: stripped RIDEV_NOLEGACY from " << count << " device(s)\n";
        } else {
            std::cout << "[-] CursorHooks: re-registration failed (GetLastError=" << GetLastError() << ")\n";
        }
    }

    bool Install()
    {
        HMODULE user32 = GetModuleHandleA("user32.dll");
        if (!user32) {
            std::cout << "[-] CursorHooks: user32.dll not loaded\n";
            return false;
        }

        void* pSetCursorPos            = (void*)GetProcAddress(user32, "SetCursorPos");
        void* pClipCursor              = (void*)GetProcAddress(user32, "ClipCursor");
        void* pRegisterRawInputDevices = (void*)GetProcAddress(user32, "RegisterRawInputDevices");
        if (!pSetCursorPos || !pClipCursor || !pRegisterRawInputDevices) {
            std::cout << "[-] CursorHooks: failed to resolve user32 exports\n";
            return false;
        }

        const bool ok =
            HookManager::CreateHook(pSetCursorPos,            &hkSetCursorPos,            (void**)&oSetCursorPos) &&
            HookManager::CreateHook(pClipCursor,              &hkClipCursor,              (void**)&oClipCursor) &&
            HookManager::CreateHook(pRegisterRawInputDevices, &hkRegisterRawInputDevices, (void**)&oRegisterRawInputDevices);

        if (!ok) {
            std::cout << "[-] CursorHooks install failed\n";
            return false;
        }

        std::cout << "[+] CursorHooks installed (SetCursorPos, ClipCursor, RegisterRawInputDevices)\n";
        ForceLegacyMouseMessages();
        return true;
    }
}
