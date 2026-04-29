#pragma once

namespace Cheats
{
    // 매 MainLoop tick마다 호출되어 활성화된 치트들을 적용합니다.
    // GUI 가시성과 무관하게 동작해야 하므로 Menu::Render와 분리되었습니다.
    void Tick();
}
