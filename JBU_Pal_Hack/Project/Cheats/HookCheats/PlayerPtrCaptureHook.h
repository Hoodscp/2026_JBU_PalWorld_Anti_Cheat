#pragma once

// [Track 2 / P1] Player this 포인터 캡처. 함수 진입 시 rcx (= APalCharacter*) 를
// 8바이트 슬롯에 저장해 게임 진행 내내 최신 포인터를 노출.
//
// 사용처: SDK::Pal.cpp 가 GWorld(0x090D3030) 포인터 체인 해소에 실패할 때
//   GetCapturedPlayer() 백업 경로를 사용. 게임 패치마다 GWorld 가 깨지지만,
//   이 후킹은 AOB 기반이라 패치 영향이 적음.
//
// 토글 없음 — 항상 활성. Tick 도 호출 불필요.
namespace PlayerPtrCaptureHook
{
    bool Install();
    void Shutdown();

    // 가장 최근 캡처된 player this 포인터. 한 번도 함수가 안 불렸으면 nullptr.
    // (게임 로딩 직후 또는 인게임 미진입 상태)
    void* GetCapturedThis();
}
