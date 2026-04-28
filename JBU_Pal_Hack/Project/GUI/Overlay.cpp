#include "Overlay.h"
#include "Menu.h"
#include <iostream>
#include <d3d11.h>

#include "kiero.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef HRESULT(__stdcall* Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef void(__stdcall* DrawIndexed)(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

bool init = false;

namespace Overlay
{
    LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        // 메뉴 단축키(VK_INSERT) 토글
        if (uMsg == WM_KEYDOWN && wParam == VK_INSERT) {
            Menu::Config.bShowMenu = !Menu::Config.bShowMenu;
            return true; // 입력 무시
        }

        // 메뉴가 켜져있을 경우 게임 시점 조작을 멈추고 ImGui가 마우스를 처리하도록 함
        if (Menu::Config.bShowMenu) {
            ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
            return true; // 게임에게 입력을 넘기지 않음
        }

        return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
    }

    HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
    {
        if (!init)
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

                // 스타일 (옵션)
                ImGui::StyleColorsDark();

                ImGui_ImplWin32_Init(window);
                ImGui_ImplDX11_Init(pDevice, pContext);
                init = true;
            }
            else return oPresent(pSwapChain, SyncInterval, Flags);
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 실제 메뉴 렌더링 호출
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
                Sleep(1000); // 1초 대기 후 재시도
            }
        } while (!init_hook);
    }

    void Shutdown()
    {
        std::cout << "[+] Shutting down Overlay...\n";
        if (init) {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }
        if (window && oWndProc) {
            SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)oWndProc);
        }
        kiero::shutdown();
    }
}
