cmake_minimum_required(VERSION 3.15)
project(JBU_Pal_Hack VERSION 1.0.0)

# Build DLL
add_library(${PROJECT_NAME} SHARED
    Core/dllmain.cpp
    Core/HackMain.cpp
    GUI/Menu.cpp
    GUI/Overlay.cpp
    Memory/Scanner.cpp
    # Memory/HookManager.cpp
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    Core
    GUI
    Memory
    SDK
    Vendor/ImGui
    Vendor/MinHook/include
    Vendor/kiero
)

# Linking DirectX 11/12 depends on your overlay strategy. Basic dx11 here.
target_link_libraries(${PROJECT_NAME} PRIVATE d3d11 d3dcompiler user32 kernel32)

# It's recommended to add the cpp files of ImGui directly here
# Normally you'd add Vendor/ImGui/imgui.cpp, imgui_draw.cpp, etc. 
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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/kiero/kiero.cpp")
    target_sources(${PROJECT_NAME} PRIVATE Vendor/kiero/kiero.cpp)
endif()
