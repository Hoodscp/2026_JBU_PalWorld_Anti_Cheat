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

    // Always-Normal-Temperature mid-function patch.
    // 출처: Reference/CT Files/Palworld-Win64-Shipping.CT (symbol: temperature)
    // 매치 위치: Palworld-Win64-Shipping.exe.text+299D501 (게임 패치마다 변동)
    // 매치되는 14바이트:
    //   48 8B 9C 24 B0 00 00 00   mov rbx,[rsp+0xB0]
    //   39 87 38 01 00 00          cmp [rdi+0x138],eax
    // 사용처: Cheats/HookCheats/TemperatureHook.cpp (함수가 아닌 명령어 시퀀스를 후킹)
    inline constexpr const char* Temperature =
        "48 8B 9C 24 B0 00 00 00 39 87 38 01 00 00";

    // Otomo (party) 컨테이너 자동 캡쳐 후크.
    // ────────────────────────────────────────────────────────────
    // 목적: GUObjectArray 풀스캔을 폐기하고, UPalOtomoHolderComponentBase 의
    //       no-arg 멤버 함수에 후크를 걸어 진입 시 this+0x110 (CharacterContainer)
    //       을 1 회 캡쳐해 SDK::SetOtomoContainerOverride 에 등록한다.
    //
    // 후보 함수 (전부 인자 없음, void Method() — 후크 detour 가 __fastcall(self)
    // 시그니처라 반드시 no-arg 메서드여야 함. 인자 있는 메서드 (예:
    // ActivateCurrentOtomo(const FTransform&)) 에 걸면 rdx 가 깨져 게임 크래시).
    //
    //   1순위:  void UPalOtomoHolderComponentBase::OnRep_CharacterContainer()
    //           → CharacterContainer 가 RepNotify 로 갱신될 때마다 호출. 클라
    //             진영에선 항상 발사. 캡쳐 타이밍 가장 안전.
    //
    //   2순위:  void UPalOtomoHolderComponentBase::OnCreatedCharacterContainer()
    //           → 서버 측 컨테이너 생성 직후 호출. 솔로/호스트 케이스 보강용.
    //
    //   3순위:  void UPalOtomoHolderComponentBase::Initialize()
    //           → 컴포넌트 초기화 시점. 이 시점엔 CharacterContainer 가 아직
    //             null 일 수 있어 첫 호출에선 캡쳐 실패할 수 있음 (재호출 대비
    //             OtomoHook 측에서 g_captured false 면 계속 시도).
    //
    // 분석 절차: x64dbg / IDA 에서 위 후보 함수 중 하나를 찾아 첫 N(≥12) 바이트를
    // IDA-style 패턴으로 적는다. 빈 문자열이면 OtomoHook 이 install skip 하고
    // UI 의 수동 hex 입력만 살아 있음. ExampleHook / Temperature 동일 패턴.
    //
    // 현 시그니처: void UPalOtomoHolderComponentBase::OnRep_CharacterContainer()
    //   - 분석 빌드: Palworld-Win64-Shipping.exe (2026-05-14 dump)
    //   - 실 impl 위치: .text @ 0x142ABFA40 (exec 썽크 0x142911E50 가 호출)
    //   - 첫 20 바이트(노 relocation):
    //       40 56                     push    rsi
    //       57                        push    rdi
    //       48 83 EC 58               sub     rsp, 58h
    //       48 8D B1 10 01 00 00      lea     rsi, [rcx+110h]   ← CharacterContainer
    //       48 8B F9                  mov     rdi, rcx
    //       48 8B 16                  mov     rdx, [rsi]
    //   - lea rsi, [rcx+0x110] 은 멤버 오프셋이 박혀 있어 이 함수의 고유 지문.
    //     게임 패치 후 stack 크기(0x58) 나 dest 레지스터가 바뀌면 해당 자리만
    //     ?? 로 풀고 재스캔 (예: "40 56 57 48 83 EC ?? 48 8D ?? 10 01 00 00 ...").
    inline constexpr const char* OtomoContainerCapture =
        "40 56 57 48 83 EC 58 48 8D B1 10 01 00 00 48 8B F9 48 8B 16";

    // TODO(Team C): 분석 후 추가
    //   inline constexpr const char* ConsumeAmmo       = "...";
    //   inline constexpr const char* StaminaDecrement  = "...";
    //   inline constexpr const char* APalCharacterTick = "...";
}
