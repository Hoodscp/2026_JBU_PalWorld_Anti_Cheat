#pragma once

namespace TemperatureHook
{
    // AOB로 온도 상태 변경 명령어 시퀀스를 찾아 14바이트 in-place mid-function
    // patch를 설치합니다. 패치 활성 시, 게임이 캐릭터 온도 상태(int @ +0x138)에
    // 쓰려는 새 값(eax)을 0(=Normal)으로 강제하여 항상 정상 온도 상태가 유지됩니다.
    //
    // ⚠ 일반 함수 후킹(MinHook)이 아닌 명령어 시퀀스 후킹이라 내부적으로 별도
    //    트램폴린을 VirtualAlloc 해서 처리합니다. Shutdown은 반드시 DLL 언로드
    //    전에 호출되어야 합니다 (그렇지 않으면 게임이 dangling jmp 로 진입).
    bool Install();

    // Menu::Config.bForceNormalTemp 를 트램폴린 내부 toggle 바이트에 매 프레임
    // 반영합니다. 매우 저렴 (1 바이트 store).
    void Tick();

    // 원본 14바이트 복원 + 트램폴린 해제. 멱등.
    void Shutdown();
}
