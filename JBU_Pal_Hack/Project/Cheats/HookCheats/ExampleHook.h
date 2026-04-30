#pragma once

namespace ExampleHook
{
    // AOB 시그니처로 게임 함수를 찾아 MinHook으로 후킹.
    // SDK::Signatures::TakeDamage가 비어 있으면 skip하고 true 반환 (placeholder 모드).
    bool Install();
}
