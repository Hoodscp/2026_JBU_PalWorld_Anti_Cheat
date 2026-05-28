#pragma once
#include <cstdint>
#include <cstddef>

// ────────────────────────────────────────────────────────────────────
// MidHook — 게임 명령어 시퀀스를 가로채는 일반화된 mid-function trampoline.
//
// 함수 단위 후킹은 HookManager(MinHook) 으로 처리하지만, CT 의 AssemblerScript
// 스타일 인젝션(임의 명령어 중간을 jmp 로 갈아치움) 은 MinHook 으로 표현이
// 어렵습니다. 본 헬퍼는 TemperatureHook 이 손으로 짠 패턴을 일반화한 것입니다:
//
//   AOB site (패치 사이트, 원본 N바이트):
//       <원본 명령어들...>
//       ↓
//       E9 xx xx xx xx 90 90 ... 90       ; jmp trampoline + NOP padding
//
//   Trampoline (AllocateNear 로 ±2GB 안에 할당):
//       push rax
//       mov al, byte ptr [rip + toggleDisp]
//       test al, al
//       pop rax                            ; (flags 보존)
//       je  Disabled
//       <bodyEnabled bytes>                ; toggle ON 시 실행
//       jmp returnAddr
//   Disabled:
//       <bodyDisabled bytes>               ; toggle OFF 시 실행 (보통 원본 N바이트)
//       jmp returnAddr
//   toggleByte: db 0
//
// ⚠ 제약:
//   - bodyEnabled / bodyDisabled 안에 RIP-relative 명령어, 짧은 점프(0x7?, 0xEB)
//     같은 위치 의존 명령어가 있으면 안 됨. 호출자가 책임.
//   - patchLen 은 명령어 경계에 맞아야 함 (호출자가 IDA 등으로 확인).
//   - SafeShutdown 누락 시 DLL 언로드 후 게임이 dangling jmp 로 진입 → 크래시.
// ────────────────────────────────────────────────────────────────────

namespace MidHook
{
    struct Handle {
        uintptr_t targetAddr   = 0;       // 패치 사이트 (게임 모듈 안)
        size_t    patchLen     = 0;       // 원본에서 덮어쓴 바이트 수
        uint8_t*  trampoline   = nullptr; // VirtualAlloc 영역 (PAGE_EXECUTE_READWRITE)
        size_t    trampolineLen = 0;
        size_t    toggleOffset = 0;       // 트램폴린 안에서 toggle byte 위치
        uint8_t   originalBytes[64]{};    // 최대 64바이트까지 백업 가능
    };

    // 패치 설치. 실패 시 nullptr 반환. 성공 시 호출자가 Handle 의 수명을 관리.
    // sig 가 빈 문자열이거나 패턴이 모듈에 없으면 nullptr (로깅만, 에러 아님).
    //
    // bodyEnabled / bodyDisabled 가 nullptr 이면 그 분기는 "아무것도 실행 안 함" 으로
    // 처리되어 곧바로 jmp return 만 실행됩니다. NOP 화 식의 토글에 유용.
    Handle* Install(
        const char* hookName,
        const char* moduleName,
        const char* signature,
        size_t      patchLen,
        const uint8_t* bodyEnabled,  size_t bodyEnabledLen,
        const uint8_t* bodyDisabled, size_t bodyDisabledLen);

    // 토글 byte 한 바이트 store. 매 프레임 호출해도 매우 저렴 (1 store).
    void SetEnabled(Handle* h, bool enabled);

    // 토글 byte 현재 값.
    bool IsEnabled(const Handle* h);

    // 원본 N바이트 복원 + 트램폴린 해제. 멱등. h 는 호출 후 사용 금지.
    void Shutdown(Handle* h);
}
