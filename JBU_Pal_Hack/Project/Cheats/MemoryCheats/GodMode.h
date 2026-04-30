#pragma once

namespace GodMode
{
    // 매 MainLoop tick마다 호출. Menu::Config.bGodMode가 true면 HP를 강제 값으로 덮어씀.
    void Tick();
}
