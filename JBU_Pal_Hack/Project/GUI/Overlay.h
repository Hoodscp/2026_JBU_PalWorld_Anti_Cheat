#pragma once
#include <windows.h>

namespace Overlay
{
    // Hooks DirectX Present to draw ImGui
    void Initialize();
    
    // Unhooks and cleans up ImGui
    void Shutdown();
}
