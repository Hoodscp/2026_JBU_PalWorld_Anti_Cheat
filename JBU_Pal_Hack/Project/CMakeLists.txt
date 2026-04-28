cmake_minimum_required(VERSION 3.15)
project(JBU_Pal_Hack VERSION 1.0.0)

# Build DLL
add_library(${PROJECT_NAME} SHARED
    Core/dllmain.cpp
    Core/HackMain.cpp
    GUI/Menu.cpp
    GUI/Overlay.cpp
    Memory/Scanner.cpp
    SDK/Pal.cpp
    # Memory/HookManager.cpp
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

# Link essential D3D11 and Windows libs
target_link_libraries(${PROJECT_NAME} PRIVATE d3d11 d3dcompiler user32 kernel32)

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
