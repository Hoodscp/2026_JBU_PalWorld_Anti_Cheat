#pragma once
#include <windows.h>
#include <d3d11.h>

namespace Overlay
{
    // Initializes the DirectX hook and ImGui
    void Initialize();
    
    // Shuts down ImGui and clears the hook
    void Shutdown();
}
