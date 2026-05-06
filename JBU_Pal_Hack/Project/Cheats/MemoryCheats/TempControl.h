#pragma once

// CurrentTemperature 를 쾌적 범위(예: 22℃)로 고정.
// PlayerCharacter::BodyTemperatureComponent 오프셋이 미해결(0)이면 NOP.
namespace TempControl
{
    void Tick();
}
