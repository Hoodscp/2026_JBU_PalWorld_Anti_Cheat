#pragma once

// ────────────────────────────────────────────────────────────────────
// CrashHandler — VEH(Vectored Exception Handler) 로 치명 예외 가로채기.
//
// 목표:
//   게임이 튕길 때 *어디서* 튕겼는지 알기. 콘솔이 닫혀도 Logger 가
//   파일로 즉시 flush 하므로, 핸들러가 마지막에 한 번만 잘 기록하면 된다.
//
// 기록 내용:
//   - 예외 코드 / 발생 주소(VA)
//   - 그 주소가 속한 모듈명 + RVA  → 게임 .text 안인지, 우리 트램펄린인지,
//     우리 DLL 안인지 즉시 구분 가능
//   - 핵심 컨텍스트 레지스터(RIP/RSP/RAX/RCX/RDX/RBX) — Track 2 패치에서
//     this 캡처 실패/포인터 오염 진단에 결정적
//   - Logger::CurrentPhase() — 마지막으로 진입한 단계명
//   - Backtrace 8 프레임 — RtlCaptureStackBackTrace, 각 프레임을 모듈+RVA 로
//
// 동작:
//   - 우선순위 1 로 VEH 등록 → 게임의 SEH 보다 먼저 호출됨.
//   - 치명 예외(AV/IllegalInstr/StackOverflow/HeapCorruption 등)만 로그.
//     C++ 예외(0xE06D7363) 같은 일상적 예외는 무시 — 오탐 폭주 방지.
//   - 로그 후엔 항상 EXCEPTION_CONTINUE_SEARCH 반환. 우리는 가로채지 않고
//     "관찰" 만 한다 → 게임/디버거 SEH 동작에 영향 없음.
//   - 동일 예외가 재진입해도 한 번만 기록하도록 reentrance guard.
//
// 한계:
//   - 우리 DLL 의 트램펄린 안에서 stack overflow 가 나면 핸들러 본체조차
//     실행 못 할 수 있음 (스택 부족). 이 경우 마지막으로 적힌 단계명이
//     단서가 됨 (Logger 가 매 PHASE 마다 즉시 flush).
// ────────────────────────────────────────────────────────────────────

namespace CrashHandler {

    // VEH 등록. Logger::Init() 직후에 호출 권장. 멱등.
    bool Install();

    // VEH 해제. Logger::Shutdown() 직전에 호출. 멱등.
    void Shutdown();

}  // namespace CrashHandler
