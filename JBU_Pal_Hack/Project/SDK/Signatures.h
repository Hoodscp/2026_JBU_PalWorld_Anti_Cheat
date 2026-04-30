#pragma once

// ────────────────────────────────────────────────────────────────────
// Palworld 함수 AOB(Array of Bytes) 시그니처 모음.
//
// 사용처: HookCheats 트랙. 함수의 첫 N바이트를 패턴으로 등록하면
// Scanner가 게임 모듈 메모리에서 일치 위치를 찾아 함수 주소를 반환합니다.
//
// 패턴 형식 (IDA-style 문자열):
//   "48 89 5C 24 ? 57 48 83 EC 20"
//     - 공백 구분
//     - "?" 또는 "??" : 와일드카드 (해당 바이트 무시)
//     - 그 외 두 자리 hex
//
// ⚠ 빈 문자열 = "아직 분석 안 함" → ExampleHook이 감지하고 install skip.
//   값을 채우는 즉시 다음 실행에서 자동으로 활성화됩니다.
// ────────────────────────────────────────────────────────────────────

namespace SDK::Signatures
{
    inline constexpr const char* TakeDamage = "";

    // TODO(Team C): 분석 후 추가
    //   inline constexpr const char* ConsumeAmmo       = "...";
    //   inline constexpr const char* StaminaDecrement  = "...";
    //   inline constexpr const char* APalCharacterTick = "...";
}
