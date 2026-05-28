#pragma once
#include "MidHook.h"

// ────────────────────────────────────────────────────────────────────
// NopHook — MidHook 위에 얹은 "토글로 한 명령어 NOP화" 한 줄 헬퍼.
//
// 가장 흔한 케이스 (CT 의 staminaSub, durabilityWrite, writesToItems 등 ~10여
// 인젝션) 를 한 함수 호출로 처리하기 위한 sugar.
//
// 동작:
//   - patchLen 바이트를 패치 사이트에서 가로채 트램폴린으로 jmp
//   - 트램폴린 안에서 toggle byte 검사
//     · ON  → 첫 nopInstrLen 바이트를 "NOP" 처리(=실행 skip).
//             나머지 tail 바이트는 그대로 실행하고 return.
//     · OFF → patchLen 전체를 그대로 실행하고 return.
//
// 제약 (호출자가 보장):
//   - tail = patchLen - nopInstrLen 바이트 안에 RIP-relative 명령어나
//     짧은 점프(0x7?/0xEB)가 있으면 안 됨. patchLen 을 거기서 끊을 것.
//   - patchLen >= 5 (jmp 공간), patchLen <= 64 (Handle 백업 크기).
// ────────────────────────────────────────────────────────────────────

namespace NopHook
{
    // Install:
    //   hookName     : 로깅용 라벨
    //   moduleName   : 보통 "Palworld-Win64-Shipping.exe"
    //   signature    : SDK::Signatures 의 IDA-style AOB
    //   patchLen     : 패치 사이트에서 가로챌 총 바이트 (>= 5)
    //   nopInstrLen  : 그 중 토글 ON 시 skip 할 leading 바이트 (== patchLen 이면 tail 없음)
    //
    // 반환: 성공 시 MidHook::Handle*, 실패/스킵 시 nullptr.
    MidHook::Handle* Install(const char* hookName,
                             const char* moduleName,
                             const char* signature,
                             size_t      patchLen,
                             size_t      nopInstrLen);
}
