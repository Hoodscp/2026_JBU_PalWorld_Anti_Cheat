cmake_minimum_required(VERSION 3.15)
project(JBU_Pal_Hack VERSION 1.0.0)

# Build DLL
add_library(${PROJECT_NAME} SHARED
    # Core
    Core/dllmain.cpp
    Core/HackMain.cpp
    # GUI
    GUI/Menu.cpp
    GUI/Overlay.cpp
    # 인프라 (스캐너, 후킹 매니저, 시스템 후킹)
    Memory/Scanner.cpp
    Memory/HookManager.cpp
    Memory/CursorHooks.cpp
    # SDK (오프셋 기반 게임 데이터 접근)
    SDK/Pal.cpp
    # Track 1: 오프셋 기반 메모리 치트 (각 기능 = 1 파일)
    Cheats/MemoryCheats/GodMode.cpp
    Cheats/MemoryCheats/PlayerCheats.cpp
    Cheats/MemoryCheats/TechCheats.cpp
    Cheats/MemoryCheats/InventoryCheats.cpp
    # Track 2: AOB + 함수 후킹 치트 (각 기능 = 1 파일)
    Cheats/HookCheats/ExampleHook.cpp
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    Core
    GUI
    Memory
    SDK
    fixed_lib
    Vendor/ImGui
    Vendor/MinHook/include
    Vendor/kiero
)

# Link essential D3D11 and Windows libs (psapi: GetModuleInformation in Scanner)
target_link_libraries(${PROJECT_NAME} PRIVATE d3d11 d3dcompiler user32 kernel32 psapi)

# ----- ImGui -----
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ImGui/imgui.cpp")
    target_sources(${PROJECT_NAME} PRIVATE
        Vendor/ImGui/imgui.cpp
        Vendor/ImGui/imgui_draw.cpp
        Vendor/ImGui/imgui_tables.cpp
        Vendor/ImGui/imgui_widgets.cpp
        Vendor/ImGui/backends/imgui_impl_dx11.cpp
        Vendor/ImGui/backends/imgui_impl_win32.cpp
    )
endif()

# ----- Kiero -----
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/fixed_lib/kiero.cpp")
    target_sources(${PROJECT_NAME} PRIVATE fixed_lib/kiero.cpp)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/kiero/kiero.cpp")
    target_sources(${PROJECT_NAME} PRIVATE Vendor/kiero/kiero.cpp)
endif()

# ----- MinHook (Required for Kiero and general hooking) -----
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/MinHook/src/hook.c")
    target_sources(${PROJECT_NAME} PRIVATE
        Vendor/MinHook/src/buffer.c
        Vendor/MinHook/src/hook.c
        Vendor/MinHook/src/trampoline.c
        Vendor/MinHook/src/hde/hde32.c
        Vendor/MinHook/src/hde/hde64.c
    )
endif()
