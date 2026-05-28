#pragma once
#include <cstdint>
#include <cstddef>

// ────────────────────────────────────────────────────────────────────
// SaveSpaceHook — CT 의 "saveSpace" 포인터 캡처 패턴 헬퍼.
//
// CT 의 getPlayerPtr / getItemsInventoryOpen / technologyCostSubtract 등
// 다수 인젝션은 다음 구조를 공유한다:
//
//   newmem:
//     mov [<saveSpace>], rcx     ; or rdi / rax / rbx — 함수 진입 시 this 캡처
//   code:
//     <원본 명령어들>
//     jmp return
//
// 본 헬퍼는 토글 없는 (항상 ON) MidHook 변종으로, 트램폴린 안에서 지정된
// 레지스터를 자동 할당된 8바이트 슬롯에 저장한다. SDK 측은 이 슬롯을
// "GWorld 백업 경로" 로 사용할 수 있어, 게임 패치마다 깨지는 GWorld 의존성을
// 우회할 수 있다.
//
// ⚠ MidHook 과 마찬가지로 패치된 영역 안에 RIP-relative / 짧은 점프가 있으면
//    안 됨. 호출자가 patchLen 을 명령어 경계에 맞추는 책임.
// ────────────────────────────────────────────────────────────────────

namespace SaveSpaceHook
{
    // 캡처 대상 레지스터.
    enum class Reg { Rax, Rcx, Rdx, Rbx, Rsp, Rbp, Rsi, Rdi };

    struct Handle {
        uintptr_t targetAddr    = 0;
        size_t    patchLen      = 0;
        uint8_t*  trampoline    = nullptr;
        size_t    trampolineLen = 0;
        size_t    saveSlotOffset = 0;   // 트램폴린 안에서 8바이트 슬롯 위치
        uint8_t   originalBytes[64]{};
    };

    // sig 의 첫 매칭 위치 + sigOffset 에서 patchLen 바이트를 가로채, 트램폴린
    // 진입과 동시에 captureReg 값을 8바이트 슬롯에 저장하고 원본을 그대로 실행한
    // 뒤 복귀.
    //
    // sigOffset: AOB 매치가 함수 본체 직전 패딩(int3 등) 을 포함할 때 유용.
    //   예) CT 원본 sig "CC 48 8B 81 38 03 00 00 C3" 에서 mov 시작은 +1.
    //   기본 0 = 매치 위치 그대로 patch.
    //
    // 성공 시 Handle*, 시그니처 비어있거나 패턴 미발견 시 nullptr.
    Handle* Install(const char* hookName,
                    const char* moduleName,
                    const char* signature,
                    size_t      patchLen,
                    Reg         captureReg,
                    size_t      sigOffset = 0);

    // 현재 캡처된 포인터 값 (한 번도 함수가 호출 안 됐으면 nullptr).
    // 매 프레임 폴링해도 비용은 단순 read 1회.
    void* Current(const Handle* h);

    // 원본 바이트 복원 + 트램폴린 해제. 멱등.
    void Shutdown(Handle* h);
}
