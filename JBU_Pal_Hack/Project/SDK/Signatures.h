#pragma once

// ────────────────────────────────────────────────────────────────────
// Palworld 함수/명령어 AOB(Array of Bytes) 시그니처 모음.
//
// 사용처: HookCheats 트랙.
//   - "함수 진입" 후킹은 HookManager(MinHook) 로 처리
//   - "함수 중간 명령어 시퀀스" 후킹은 MidHook / NopHook / SaveSpaceHook 으로 처리
//
// 패턴 형식 (IDA-style 문자열):
//   "48 89 5C 24 ? 57 48 83 EC 20"
//     - 공백 구분
//     - "?" 또는 "??" : 와일드카드 (해당 바이트 무시)
//     - 그 외 두 자리 hex
//
// ⚠ 빈 문자열 = "아직 분석 안 함" → 후킹이 감지하고 install skip.
//   값을 채우는 즉시 다음 실행에서 자동으로 활성화됩니다.
//
// 출처: 대부분 Reference/mdfiles/2026-05-07_AOB_Patterns_CT_Analysis.md
// (Palworld-Win64-Shipping.CT, krebs/Byakuran 작성) 의 검증된 패턴.
// 빌드별로 깨질 수 있으므로 게임 패치 직후엔 IDA 로 재검증 필요.
// ────────────────────────────────────────────────────────────────────

namespace SDK::Signatures
{
    // ── 함수 진입 후킹용 (MinHook) ─────────────────────────────────
    inline constexpr const char* TakeDamage = "";

    // ── Mid-function 인플레이스 패치용 ─────────────────────────────

    // [Track 2] Always-Normal-Temperature mid-function patch.
    // 매치되는 14바이트:
    //   48 8B 9C 24 B0 00 00 00   mov rbx,[rsp+0xB0]
    //   39 87 38 01 00 00          cmp [rdi+0x138],eax
    // 사용처: Cheats/HookCheats/TemperatureHook.cpp
    inline constexpr const char* Temperature =
        "48 8B 9C 24 B0 00 00 00 39 87 38 01 00 00";

    // ── P0: NOP-style 패치 (NopHook 헬퍼로 처리) ─────────────────
    // 모두 "특정 명령어 1~2개를 NOP 화" 하는 단순 패치. 토글 ON 시
    // 첫 명령어가 실행되지 않아 차감/쓰기가 일어나지 않음.

    // [P0] Infinite Stamina — subss xmm0,xmm2 무력화
    //   F3 0F 5C C2   subss xmm0,xmm2          ← NOP 대상 (4바이트)
    //   F3 48 0F 2C C0   cvttss2si rax,xmm0   ← tail, 그대로 실행
    //   48 89 01           mov [rcx],rax
    //   48 8B C1           mov rax,rcx
    inline constexpr const char* StaminaSub =
        "F3 0F 5C C2 F3 48 0F 2C C0 48 89 01 48 8B C1";

    // [P0] Never Get Hungry (main branch) — subss xmm2,xmm7 무력화
    //   F3 0F 5C D7        subss xmm2,xmm7    ← NOP 대상 (4바이트)
    //   0F 28 7C 24 40     movaps xmm7,[rsp+0x40]
    //   0F 2F D1           comiss xmm2,xmm1
    // (마지막 `72 07` jb 는 relative 라 patchLen 에서 제외 — Hook 모듈에서 12 로 자름)
    inline constexpr const char* DoesFoodHungerSub =
        "F3 0F 5C D7 0F 28 7C 24 40 0F 2F D1 72 07";

    // [P0] Freeze Food Counter (조리/숙성 타이머 정지)
    //   F3 0F 11 83 58 01 00 00   movss [rbx+0x158], xmm0   ← NOP 대상 (전체 8바이트)
    inline constexpr const char* FoodTimerWrite =
        "F3 0F 11 83 58 01 00 00";

    // [P0] Items Don't Decrease — mov [rbx+0x154], r14d 무력화
    //   44 89 B3 54 01 00 00      mov [rbx+0x154], r14d     ← NOP 대상 (7바이트)
    //   45                         (다음 명령어의 REX 시작 — patchLen 7 에서 끊음)
    inline constexpr const char* WritesToItems =
        "44 89 B3 54 01 00 00 45";

    // [P0] Infinite Durability — movss [rcx+0x10], xmm0 무력화
    //   F3 0F 11 41 10            movss [rcx+0x10], xmm0    ← NOP 대상 (5바이트)
    //   F3 0F 11 51 ?             movss [rcx+?], xmm2       (다음 instr 시작 — patchLen 5)
    inline constexpr const char* DurabilityWrite =
        "F3 0F 11 41 10 F3 0F 11 51";

    // [P0] Infinite Ammo — lea/mov 쌍을 통째로 NOP
    //   8D 42 FF                  lea eax,[rdx-1]           ← (3바이트)
    //   89 81 84 00 00 00         mov [rcx+0x84], eax       ← (6바이트)
    // 총 9바이트 NOP 화 → 탄약 차감이 일어나지 않음.
    inline constexpr const char* AmmoDecrease =
        "8D 42 FF 89 81 84 00 00 00";

    // ── P1: 포인터 캡처 (SaveSpaceHook 헬퍼로 처리) ─────────────
    // GWorld(0x090D3030) 가 게임 패치마다 깨지는 의존성을 우회하는 백업 경로.
    // 토글 없음 — 항상 활성. 함수 호출 시 첫 명령어 직전에 register 를 슬롯에 저장.

    // [P1] Player this 포인터 캡처 (rcx). saveSpace+0x128 = APalCharacter*.
    // 매치 위치는 함수 진입점이라 rcx 가 신뢰성 있게 캡처됨.
    //   48 8B 81 28 01 00 00      mov rax, [rcx+0x128]      ← 첫 명령어 (7바이트)
    //   ... (이후 패턴은 uniqueness 용)
    inline constexpr const char* PlayerPtrCapture =
        "48 8B 81 28 01 00 00 48 8B DA 0F 29 74 24 20 F3 0F 10 B1 D0 01 00 00 48 85 C0 75";

    // [P1] Inventory open 시 rcx 캡처 (인벤토리 객체 root).
    //   4A 20 83 B9 54 01 00 00 00   and [r11+0x015401B9], al   (9바이트, AOB 길이 그대로)
    //   ↑ patchLen 9, rcx 캡처
    inline constexpr const char* ItemSlotCapture =
        "4A 20 83 B9 54 01 00 00 00";

    // [P1] Technology cost subtract 시점에서 rdi 캡처 (= TechnologyData*).
    // saveSpace+0x150 / +0x154 가 TechnologyPoint / BossTechnologyPoint.
    //   2B C1                     sub eax, ecx              ← (2바이트, NOP 대상은 아님)
    //   89 87 50 01 00 00         mov [rdi+0x150], eax      ← (6바이트)
    // 총 8바이트 패치, rdi 캡처.
    inline constexpr const char* TechPointCapture =
        "2B C1 89 87 50 01 00 00";

    // [P1] Inventory open 시 현재 EXP 포인터 캡처 (rcx).
    //   CC                        int3                       ← 함수 직전 패딩 (skip)
    //   48 8B 81 38 03 00 00      mov rax, [rcx+0x338]       ← 함수 첫 명령어
    //   C3                        ret
    // 패치 위치는 mov 명령어 (CC 다음 1바이트부터). 8바이트 패치, rcx 캡처.
    inline constexpr const char* ExpPtrCapture =
        "CC 48 8B 81 38 03 00 00 C3";

    // ── P2: 조건분기 강제 (MidHook bodyEnabled 로 분기 결과 변조) ──
    // 함수 본체에서 cmp/test 결과를 사용자가 원하는 방향으로 강제.
    // bodyDisabled = 원본 그대로, bodyEnabled = 분기 조작 + 원본 재실행.

    // [P2] Can Build Anything — buildableTest 통과 강제.
    // CT 원문 sig: `40 38 B1 E4 00 00 00` (cmp byte ptr [rcx+0xE4], sil, 7바이트).
    // bodyEnabled: 같은 위치에 sil 자체를 미리 write 해 cmp 결과를 ZF=1 로 강제.
    //   `40 88 B1 E4 00 00 00`   mov [rcx+0xE4], sil   (7바이트)
    //   `40 38 B1 E4 00 00 00`   cmp [rcx+0xE4], sil   (7바이트, 원본 재실행)
    // bodyDisabled: 원본 cmp 7바이트만.
    inline constexpr const char* BuildableTest =
        "40 38 B1 E4 00 00 00";

    // [P2] Can Catch Boss Pal — `[rax+0x598]` 보스 차단 플래그를 0 강제 후 원본 cmp.
    // CT 원문 sig: `48 8B 83 30 06 00 00 80 B8 98 05 00 00 00` (14바이트).
    //   `48 8B 83 30 06 00 00`   mov rax, [rbx+0x630]              (7바이트)
    //   `80 B8 98 05 00 00 00`   cmp byte ptr [rax+0x598], 0       (7바이트, imm8=0)
    // bodyEnabled (mov rax 후 byte 강제 0 삽입 후 원본 cmp):
    //   `48 8B 83 30 06 00 00`     mov rax, [rbx+0x630]            (원본)
    //   `C6 80 98 05 00 00 00 00`  mov byte ptr [rax+0x598], 0     (8바이트, 강제)
    //   `80 B8 98 05 00 00 00`     cmp byte ptr [rax+0x598], 0     (원본 재실행)
    inline constexpr const char* CanCatchBossPal =
        "48 8B 83 30 06 00 00 80 B8 98 05 00 00 00";

    // ── P2: 레지스터 강제값 / 명령어 주입 ────────────────────────

    // [P2] Perfect Pal Stats on Creation — `mov edi, 0x64` 사전 주입.
    // CT 원문 sig: `41 BE 65 00 00 00 44 2B F7 45 85 F6 0F` (13바이트).
    //   `41 BE 65 00 00 00`   mov r14d, 0x65   ← 패치 사이트 (6바이트)
    //   `44 2B F7`            sub r14d, edi    (이후 sig 는 uniqueness 용)
    // bodyEnabled: 원본 mov 직전에 `BF 64 00 00 00` (mov edi, 0x64) 삽입 →
    //   sub r14d, edi 가 edi=0x64 로 평가 → 새 펄의 모든 IV(HP/근접/원거리/방어)
    //   가 100 으로 부풀려짐.
    inline constexpr const char* PalStatsCreation =
        "41 BE 65 00 00 00 44 2B F7 45 85 F6 0F";

    // [P2] Quick Research — `addss xmm6, [rdi+0x14]` 직후 `mulss xmm6, xmm6` 주입.
    // CT 원문 sig: `F3 0F 58 77 14` (5바이트 — 정확히 1 명령어).
    //   `F3 0F 58 77 14`   addss xmm6, [rdi+0x14]   ← 패치 사이트
    // bodyEnabled: 원본 addss 후 `F3 0F 59 F6` (mulss xmm6, xmm6, 4바이트) 추가 →
    //   매 tick 진행도가 제곱 누적되어 연구 완료까지 몇 호출이면 충분.
    inline constexpr const char* ResearchIncrease =
        "F3 0F 58 77 14";

    // TODO(Team C): 추가 분석 후 확장
    //   - mobsTakeDamgBossF (다중-비트 토글: GodMode+OHK+Immortal+AlmostKill)
    //   - getCaptureStrength, movCaptureChance (포획률/강도 부풀림)
    //   - arenaRPGainMult (Arena RP 배율 증가)
    //   - writeMachineHealth (One-Hit Machines)
}
