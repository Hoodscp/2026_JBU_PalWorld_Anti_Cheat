#include "Overlay.h"
#include "Menu.h"
#include <iostream>
#include <atomic>
#include <d3d11.h>

#include "kiero.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef HRESULT(__stdcall* Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

Present oPresent = nullptr;
HWND window = NULL;
WNDPROC oWndProc = nullptr;
ID3D11Device* pDevice = nullptr;
ID3D11DeviceContext* pContext = nullptr;
ID3D11RenderTargetView* mainRenderTargetView = nullptr;

std::atomic<bool> init{false};
std::atomic<bool> g_shuttingDown{false};

namespace Overlay
{
    LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        // 메뉴 단축키(VK_INSERT) 토글
        if (uMsg == WM_KEYDOWN && wParam == VK_INSERT) {
            Menu::Config.bShowMenu = !Menu::Config.bShowMenu;
            return 1; // 입력 무시
        }

        // 메뉴가 켜져있을 경우 게임 시점 조작을 멈추고 ImGui가 마우스를 처리하도록 함
        if (Menu::Config.bShowMenu && init.load()) {
            ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

            // 윈도우 그래픽 처리(최소/최대화 등)는 통과시키고,
            // 게임 내부로 향하는 마우스/키보드 입력 신호만 차단합니다.
            if ((uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) ||
                (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST) ||
                (uMsg == WM_CHAR) || (uMsg == WM_INPUT)) {
                return 1;
            }
        }

        return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
    }

    HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
    {
        // Shutdown이 시작되면 ImGui 컨텍스트가 해제 중이므로 즉시 원본 Present로 패스.
        // 이 가드가 hkPresent ↔ Shutdown race를 막아줍니다.
        if (g_shuttingDown.load()) {
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        if (!init.load())
        {
            if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
            {
                pDevice->GetImmediateContext(&pContext);
                DXGI_SWAP_CHAIN_DESC sd;
                pSwapChain->GetDesc(&sd);
                window = sd.OutputWindow;
                ID3D11Texture2D* pBackBuffer;
                pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
                pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
                pBackBuffer->Release();
                oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);

                // ImGui 초기화
                ImGui::CreateContext();
                ImGuiIO& io = ImGui::GetIO();
                io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange; // 자체 커서 관리
                ImGui::StyleColorsDark();

                ImGui_ImplWin32_Init(window);
                ImGui_ImplDX11_Init(pDevice, pContext);
                init.store(true);
            }
            else return oPresent(pSwapChain, SyncInterval, Flags);
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 실제 메뉴 렌더링 호출 (치트 적용은 Cheats::Tick이 별도로 처리)
        Menu::Render();

        ImGui::Render();
        pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        return oPresent(pSwapChain, SyncInterval, Flags);
    }

    void Initialize()
    {
        std::cout << "[+] Initializing DirectX 11 Hook / ImGui Overlay..." << std::endl;
        bool init_hook = false;
        int last_error = -1;

        // Kiero가 DirectX 인스턴스를 찾을 때까지 무한 재시도 (게임 로딩 지연 대비)
        do
        {
            kiero::Status::Enum status = kiero::init(kiero::RenderType::D3D11);
            if (status == kiero::Status::Success)
            {
                std::cout << "[+] Kiero Initialized! Binding D3D11 Present..." << std::endl;
                kiero::bind(8, (void**)&oPresent, hkPresent);
                init_hook = true;
            }
            else
            {
                if (last_error != status) {
                    std::cout << "[-] Kiero Init (D3D11) Status: " << (int)status << " | Retrying..." << std::endl;
                    last_error = status;
                }
                Sleep(1000);
            }
        } while (!init_hook);
    }

    void Shutdown()
    {
        std::cout << "[+] Shutting down Overlay...\n";

        // 1. 신규 hkPresent 진입을 즉시 차단 (in-flight 호출은 원본 Present로 패스)
        g_shuttingDown.store(true);

        // 2. Kiero가 Present 후킹과 MinHook lifecycle을 함께 정리.
        kiero::shutdown();

        // 3. MinHook 내부적으로 in-flight 함수 종료를 기다리지만, 이미 NewFrame
        // 단계에 진입한 hkPresent가 끝까지 실행되도록 보수적 대기.
        Sleep(100);

        // 4. 가로채었던 창 입력 신호(WndProc)를 게임 원본으로 원상 복구
        if (window && oWndProc) {
            SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)oWndProc);
            oWndProc = nullptr;
        }

        // 5. 안전하게 ImGui 메모리 할당 해제
        if (init.load()) {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            init.store(false);
        }

        // 6. D3D 객체 레퍼런스 카운트 해제 (재주입 시 누적 누수 방지)
        if (mainRenderTargetView) { mainRenderTargetView->Release(); mainRenderTargetView = nullptr; }
        if (pContext)             { pContext->Release();             pContext = nullptr; }
        if (pDevice)              { pDevice->Release();              pDevice = nullptr; }
    }
}
