# Palworld CT 파일 — AOB 패턴 / 코드 인젝션 종합 분석

> **분석 대상**:
> - `JBU_Pal_Hack/Reference/CT Files/Palworld_Hack.CT` (소형, 5KB) — 단순 포인터 체인 + Lua 스크립트 2종
> - `JBU_Pal_Hack/Reference/CT Files/Palworld-Win64-Shipping.CT` (대형, 1MB, 16,849 lines, krebs/Byakuran 작성) — **AssemblerScript 기반 AOB 인젝션 80+종**
>
> **분석일**: 2026-05-07
> **목적**: 외부 CT의 모든 AOB 시그니처와 인젝션 효과를 본 프로젝트 코드베이스(Track 2 후킹 / 안티치트 탐지)에 흡수 가능한 형태로 정리.

---

## 0. 두 CT의 성격 비교

| 항목 | `Palworld_Hack.CT` | `Palworld-Win64-Shipping.CT` |
|---|---|---|
| 타겟 모듈 | `PalServer-Win64-Shipping-Cmd.exe` (전용 서버) | `Palworld-Win64-Shipping.exe` (**클라이언트** ★) |
| 변조 방식 | 정적 포인터 체인 + Lua | **`aobscanmodule` + AutoAssembler 인젝션** + Lua |
| 항목 수 | 3 CheatEntry + 2 Lua | 80+ AssemblerScript (각각 1개 이상의 AOB) |
| 가치 | 서버 측 멤버 오프셋(검증용) | **본 프로젝트의 `SDK::Signatures`/Track 2 직접 활용 가능** ★★★ |

> ⚠ **중요**: `Palworld-Win64-Shipping.CT` 의 모든 AOB 패턴은 **클라이언트 바이너리 기준** 이라, 본 프로젝트 `SDK::Signatures` 에 그대로 옮겨 `Scanner::FindPattern` + MinHook으로 후킹 가능.

---

## 1. AssemblerScript 인젝션의 표준 구조

CT 내부 모든 인젝션은 일관된 5단계 구조를 따른다. 본 프로젝트의 후킹 인프라로 변환 시 이 구조를 이해하는 것이 핵심.

```text
┌─────────────────────────────────────────────────────────────┐
│ [ENABLE] 블록                                                │
│                                                             │
│ ① {$lua} setupPattern() — AOB가 기존에 등록되었으면 무효화 │
│                                                             │
│ ② aobscanmodule(symName, ModuleName.exe, AOB...)             │
│        └─ 게임 모듈 메모리에서 패턴 검색 → symName 등록      │
│                                                             │
│ ③ alloc(newmem, $1000, symName)                              │
│        └─ 트램폴린 코드 영역 할당 (게임 모듈 ±2GB 내부)      │
│                                                             │
│ ④ newmem:                                                    │
│        ; ── 치트 로직 ──                                     │
│        ; 레지스터 변조, 메모리 write, 포인터 캡처 등         │
│                                                             │
│    code:                                                     │
│        ; ── 원본 명령어 (그대로 또는 NOP) ──                 │
│        jmp return                                            │
│                                                             │
│    symName:                                                  │
│        jmp newmem                                            │
│        nop N    ; (jmp가 5바이트 → 원본 명령어 길이 패딩)    │
│    return:                                                   │
│                                                             │
│ ⑤ registersymbol(symName)                                    │
│                                                             │
│ [DISABLE] 블록 — symName 위치에 원본 바이트 복원, dealloc    │
└─────────────────────────────────────────────────────────────┘
```

### 1.1 본 프로젝트로의 변환 매핑

| CT 메커니즘 | 본 프로젝트 대응 |
|---|---|
| `aobscanmodule(...)` | `Scanner::FindPattern(module, sig)` (`Memory/Scanner.h`) |
| `alloc(newmem, $1000)` + `jmp newmem` | MinHook trampoline (`HookManager::CreateHook`) |
| `[DISABLE]` 블록의 db 원본 복원 | MinHook의 `MH_DisableHook` / `MH_RemoveHook` |
| `registersymbol(saveSpace) + dq 0` | C++ 측 `static void* g_savedPtr = nullptr;` |
| `mov [saveSpace], rcx` | detour 함수의 첫 줄에서 `g_savedPtr = (void*)thisArg;` |

---

## 2. AOB 패턴 — 카테고리별 종합 표

> 패턴은 모두 `Palworld-Win64-Shipping.exe` 기준. **★** = 활성(uncomment), **☆** = 주석 처리(과거 빌드용) — 동일/유사 기능에 대해 더 새로운 패턴이 함께 존재함.
>
> 활성/주석 표기는 CT 내 `aobscanmodule` 라인 자체의 주석 여부 기준이며, 실제 스캔은 대부분 `setupPattern()`(Lua) 으로 수행됨에 유의.

### 2.1 ☣ 데미지 / 체력 / 실드

| 심볼 | 활성 | AOB 패턴 | 효과 / 인젝션 동작 |
|---|---|---|---|
| `mobsTakeDamgBossF` | ★ | `48 8B 80 88 03 00 00 48 89 45 58 45 8B C4` | 메인 데미지 분기. saveSpace 비트 플래그(0:MaxHP, 1:TakeNoDmg, 2:OHK, 3:ImmortalEnemy, 4:AlmostKill)로 다수 치트 동시 토글 |
| `shieldSubtract` (구) | ☆ | `48 8B 81 88 04 00 00` | `[rcx+0x488]` 로드(ShieldHP) 직전에 `xor edx,edx` 삽입 → 차감량 0 |
| `shieldSubtract` (신) | ★ | `48 8B 81 10 04 00 00 8B FA` | 0.6.x 빌드 — 같은 효과, 오프셋 0x410으로 이동 |
| `readsShield` | ★ | `48 8B 81 00 04 00 00 48 89` | 실드 read를 가로채 `mov rax,[rcx+0x400]` 로 변경 → MaxHP 미러 (Insta Max Shield) |
| `dmgCalc` | ☆ | `48 8B 80 88 03 00 00 48 89 44 24 28 45` | 체력 로드 분기 (구버전) |
| `loadsHealthAndDmg` | ★ | `48 8B 81 88 03 00 00 4C` | 체력/데미지 로드 — saveSpace로 player/팀/적 포인터 분기 캡처 |
| `healthWhenAttacked` | ☆ | `EF FE FF 48 8B 93 78 03 00 00` | 피격 시 호출되는 함수 진입점 — saveSpace에 attacker/victim 포인터 저장 |
| `writeMachineHealth` | ★ | `88 42 00 48 8B 7C 24 78 F3 0F 10 83 D0 00 00 00` | 빌딩/머신 체력 write 시 NOP → One Hit Machines |

### 2.2 🔋 스태미나

| 심볼 | 활성 | AOB 패턴 | 효과 |
|---|---|---|---|
| `staminaSub` | ★ | `F3 0F 5C C2 F3 48 0F 2C C0 48 89 01 48 8B C1` | `subss xmm0,xmm2` 를 `nop nop nop nop` 으로 → 스태미나 감소 자체 차단 |
| `infStamina` | ★ | `48 8B 87 18 04 00 00 48 89 44 24 60 48` | 다른 분기 — `[rdi+0x418]` 를 `[r12+0x300]` 에도 미러링 (보조 분기) |
| `staminaRead` | ★ | `48 8B 81 00 03 00 00 48 89 02 48 8B C2 C3` | Read 함수 — Stamina High 표시용 (직접 변조 X) |
| `doesStaminaStuff` | ☆ | `F3 0F 58 C8 F3 48 0F 2C C1` | 구버전 (0.6.x 이전) |
| `loadStamina` | ☆ | `48 8B 81 20 03 00 00` | 또 다른 read 분기 |

### 2.3 🔫 탄약 / 발사체

| 심볼 | 활성 | AOB 패턴 | 효과 |
|---|---|---|---|
| `ammoDecrease` | ★/☆ | `8D 42 FF 89 81 84 00 00 00` | `lea eax,[rdx-1]` + `mov [rcx+0x84],eax` → 차감 자체 무력화 (NOP) |
| `ammoTestZero` | ☆ | `83 B8 84 00 00 00 00` | `cmp [rax+0x84],0` 통과 강제 (탄약 0 검사 우회) |
| `ammoDec` | ★ | `8D 42 FF 89 41 7C` | 신버전 변종 (`mov [rcx+0x7C],eax`) |
| `jetDragonAmmoDecrease` | ☆ | `8B 02 29 01 48 8B C1` | 펄 어빌리티(예: 제트드래곤 로켓) 차감 무력화 |
| `comparesSpheresZero` | ☆ | `83 BE 54 01 00 00 00` | 페일 스피어(포획구) 0 비교 우회 — Inf Sphere |

### 2.4 🎯 펄 포획

| 심볼 | 활성 | AOB 패턴 | 효과 |
|---|---|---|---|
| `getCaptureStrength` | ★ | `8B 48 08 33 C0 85 C9 0F 4F C1 C3 CC CC CC CC 48 8B C2` | 포획 강도 read를 `(strength+0x10)*0x100` 로 부풀림 → High Capture |
| `movCaptureChance` | ★ | `F3 0F 10 44 24 38 ... 48 8B 5C 24 ** 48 83 C4 ** 5F C3 CC` | 포획률 float을 `cvtsi2ss xmm0, 1` 로 강제 → 100% 확정 |
| `canCatchBossPal` | ★ | `48 8B 83 30 06 00 00 80 B8 98 05 00 00 00` | `mov byte ptr [rax+0x598],0` 사전 삽입 → 보스/머천트 잡기 가능 플래그 |
| `palStatsCreation` | ★ | `41 BE 65 00 00 00 44 2B F7 45 85 F6 0F` | `mov edi,0x64` 를 추가 → 새 펄 생성 시 모든 스탯 100 (퍼펙트) |
| `guaranteeCatch` | ★ | `48 8B DA 48 85 C0 75 14 8D 50 01 48 8B CB E8 D0` | `[rax+0x60] != 0` 일 때 `xor rax,rax` → 포획 강제 통과(부작용: 적이 즉사) |
| `shiny` | ☆ | `0F 2F F0 0F 28 B4 24 30 04 00 00` (구) / `... D0 03 00 00` (신) | Shiny 확률 비교 우회 → New Spawns are Lucky |

### 2.5 🧭 플레이어 / 카메라 / 위치

| 심볼 | 활성 | AOB 패턴 | 효과 |
|---|---|---|---|
| `getPlayerPtr` | ★ | `48 8B 81 28 01 00 00 48 8B DA 0F 29 74 24 20 F3 0F 10 B1 D0 01 00 00 48 85 C0 75` | **포인터 캡처 핵심**. `mov [saveSpace], rcx` 만 추가, 원본은 그대로. 이후 모든 플레이어 스탯 치트의 root pointer 제공. ★★★ |
| `getPlayerPos` | ☆ | `0F 10 80 60 02 00 00` | 위치 read 가로채기 (포지션 세이버용) |
| `getCameraPointer` | ☆ | `0F 10 AF 60 02 00 00` | 카메라 객체 포인터 캡처 |

> `getPlayerPtr` saveSpace를 통해 노출되는 플레이어 멤버 (CT의 `Get Player Pointers`/sub-entry 들):
>
> | 멤버 오프셋 (`+saveSpace[+0]→player+ ?`) | 의미 |
> |---|---|
> | Level, Exp, Health, Workspeed, Shield, MaxShield, MaxStamina, CurrentHunger, MaxHunger, StatPoints, ArenaRP | UI 표시 → 직접 변조 가능한 모든 스탯 |

### 2.6 🐾 트러스트 / 펄 워크 / 콘덴세이션

| 심볼 | 활성 | AOB 패턴 | 효과 |
|---|---|---|---|
| `trustTickerUnsummoned` | ☆ | `8B 87 50 06 00 00` (구) / `8B 87 48 06 00 00` (신) | 호출당 트러스트 증가량 부풀림 — Quickly Increase Trust |
| `palsWorkspeedAcc` | ☆ | `48 8B 42 28 48 83 C2 30` | 펄 작업속도 가속 |
| `palCondensation` | ☆ | `74 12 8B 00 48 8B 5C 24 38` (in `EOSSDK-Win64-Shipping.dll`) | EOS SDK 모듈 — 콘덴세이션 카운트 검사 우회 |
| `palCondensationRemove` | ★ | `29 7B 08 44 8B C5 8B D6 48 8B CB E8 BC` | Pal Condensation 제거 시 카운트 차감 NOP |

### 2.7 🍗 식사 / 정신 / 무게

| 심볼 | 활성 | AOB 패턴 | 효과 |
|---|---|---|---|
| `getFoodOnHover` | ☆ | `F3 0F 10 81 0C 04 00 00` | 호버한 음식 아이템 포인터 캡처 |
| `doesFoodHungerSub` | ★ | `F3 0F 5C D7 0F 28 7C 24 40 0F 2F D1 72 07` | `subss xmm7,xmm2` (식사량 차감) 무력화 |
| `foodTimerWrite` | ★ | `F3 0F 11 83 58 01 00 00 E8` 또는 `... 58 01 00 00` | 음식 카운터 timer write 시 NOP → Freeze Food Counters |
| `foodDoesNotDecrease` | ☆ | `F3 0F 11 87 94 03 00 00 48` | 별도 분기 (구) |
| `changeSanity` | ★ | `F3 41 0F 58 C0` | `addss xmm0,xmm8` 정신력 가산 분기 → Max Sanity |
| `enableInsanityMode` | ★ | `F3 0F 59 91 8C 02 00 00` | 정신력 곱셈 분기 |
| `loadsWeight` | ★/☆ | `F3 0F 10 8B 80 01 00 00 0F 28 F0 F3 0F 10 83 84 01 00 00 0F 2F C8` (메인) / `... 60 01 00 00 ...` (구) | 무게 비교 직전 0으로 → Zero Weight |

### 2.8 🌡 환경 (온도)

| 심볼 | 활성 | AOB 패턴 | 효과 |
|---|---|---|---|
| `temperature` | ★ | `48 8B 9C 24 B0 00 00 00 39 87 38 01 00 00` | 새 지역 진입 시 체온 비교 분기 통과 강제 → Always Normal |

### 2.9 🐟 낚시

| 심볼 | 활성 | AOB 패턴 | 효과 |
|---|---|---|---|
| `getFishing` (×4 변종) | ☆ | `F3 0F 10 81 DC 00 00 00` | 낚시 진행 변수 포인터 캡처 — Easy/Inf/Instant Fishing |
| `getFishFishingPos` | ☆ | `F3 0F 10 81 DC 00 00 00` | 같은 패턴, 다른 매치 인덱스 |
| `getPlayerFishingPos` | ☆ | `F3 0F 10 81 DC 00 00 00` | (낚시 시스템 내 플레이어) |

> ⚠ 같은 AOB가 여러 매치 인덱스로 분기되는 케이스. `setupPattern(pattern, name, **targetIndex**, ...)` 의 인덱스 파라미터로 순서를 가린다.

### 2.10 🏗 빌딩 / 크래프팅

| 심볼 | 활성 | AOB 패턴 | 효과 |
|---|---|---|---|
| `playerBuildingLoadWorkForce` | ☆ | `66 0F 6E 8B FC 03 00 00` | 작업속도 가속 |
| `buildableTest` | ★ | `40 38 B1 E4 00 00 00` | `cmp byte ptr [rcx+0xE4], sil` 통과 강제 → Can Build Anything |
| `itemsNeededForBuilding` | ☆ | `8B 42 3C 89 41 3C ... 8B 42 6C 89 41 6C` (44바이트 길이) | 건축 자재 카운트 복사 분기 NOP — Unlock all Buildings |
| `getItemRecipyUnlocke` | ☆ | `48 89 5C 24 18` | 아이템 레시피 잠금 검사 우회 |
| `craftingItemsStugg` | ☆ | `4C 8B 41 24 48 8B DA` | 크래프팅 자재 처리 분기 |
| `getCraftRecipyUnlocke` | ☆ | `48 89 5C 24 18` | (별도 매치) Craft 레시피 |
| `subStillToCreateItems` | ★ | `83 E8 01 89 87 84 02 00 00` | `sub eax,1` 삭제 → 크래프팅 자재 차감 무력화 |

### 2.11 🔬 테크놀로지 / 연구

| 심볼 | 활성 | AOB 패턴 | 효과 |
|---|---|---|---|
| `researchIncrease` | ★ | `F3 0F 58 77 14` | `addss xmm6,[rdi+14]` 직후 `mulss xmm6,xmm6` 추가 → 연구 진행도 제곱 증가 (Quick Research) |
| `technologyCostSubtract` | ★ | `2B C1 89 87 50 01 00 00` | `xor ecx,ecx` 사전 삽입 + saveSpace에 `[rdi+0x150]` 포인터 저장 → No Technology Cost + 직접 편집 가능한 포인터 노출 |
| `noAncientTechCost` | ☆ | `2B C1 89 87 54 01 00 00` | 동일 메커니즘, 오프셋 0x154 (Ancient) |
| `getTechPointsBasedOnAncient` | ☆ | `8B 81 54 01 00 00` | Ancient/Tech 분기점 캡처 |
| `testEnoughAncientTechPoints` | ☆ | `39 86 54 01 00 00` | Ancient Tech Point 충분성 검사 우회 |
| `technologyCheckIfCanUnlock` | ☆ | `39 86 50 01 00 00 EB` | Tech Point 충분성 검사 우회 |
| `getCraftingItems` | ☆ | `8B 8F 54 01 00 00 01` | 크래프트 가능 아이템 카운트 |
| `arenaRPGainMult` | ★ | `41 8D 0C 1F 0F B6 E8` | `lea ecx,[r15+rbx]` 결과를 곱셈 → Arena RP 부풀리기 |

### 2.12 ⏱ 경험치 / 레벨

| 심볼 | 활성 | AOB 패턴 | 효과 |
|---|---|---|---|
| `getExpRdx` | ★ | `4C 8B 91 38 03 00 00 44` | EXP 분기 — `mov r10,[rcx+0x338]` 가로채 saveSpace에 포인터/배율 저장 → Exp Multiplier / Flat Gain |
| `loadCurrentExpPointerOnInvOpen` | ★ | `CC 48 8B 81 38 03 00 00 C3` | 인벤토리 오픈 시 현재 EXP 포인터 캡처 (관전용) |

### 2.13 🛠 내구도

| 심볼 | 활성 | AOB 패턴 | 효과 |
|---|---|---|---|
| `durabilityWrite` | ★ | `F3 0F 11 41 10 F3 0F 11 51` | `movss [rcx+0x10],xmm0` 무력화 → Inf Durability |

### 2.14 🐣 번식

| 심볼 | 활성 | AOB 패턴 | 효과 |
|---|---|---|---|
| `quickBreeding` | ★ | `F3 0F 58 8F 48 02 00 00 0F 2F C8 72 07 0F 28 C2` | `addss xmm1,[rdi+0x248]` 직후 강제 큰 값 → Quick Breeding |

### 2.15 🎒 인벤토리 / 아이템

| 심볼 | 활성 | AOB 패턴 | 효과 |
|---|---|---|---|
| `getItemsInventoryOpen` | ★ | `4A 20 83 B9 54 01 00 00 00` | 인벤토리 오픈 시 모든 아이템 슬롯 포인터를 saveSpace 배열에 캡처 → "Get Items On Inventory Open" Item 1~42 |
| `getItemOnHover` | ★ | `8B 81 54 01 00 00` | 호버한 슬롯 포인터 캡처 → "Get Item On Hover" Item 1~10 |
| `takeStack` | ★ | `8B 81 54 01 00 00 0F 10` | 스택 차감 시 사용자 지정 카운트로 강제 → Take Stack |
| `takeStackTwo` | ★ | `45 8B 88 54 01 00 00` | `mov r9d, 6989` 강제 (Take Stack 2 변종) |
| `takeStackThree` | ★ | `8B 80 54 01 00 00 89` | (또 다른 분기) |
| `createStackWise` | ★ | `39 83 54 01 00 00 0F` | 스택 생성 시 사용자 지정 카운트 → Create Stack-wise |
| `testForInvFull` | ★ | `3C 01 0F 87 2B 01 00 00` | 인벤 풀 검사 무력화 (`Take Stack 2 Enabler`) |
| `writesToItems` | ★ | `44 89 B3 54 01 00 00 45` | 아이템 카운트 write 자체 NOP — "Items Don't Decrease" |
| `stackBytes` | ☆ | `8B 95 F8 03 00 00` | 스택 바이트 read |
| `foodTimerWrite` | ★ | `F3 0F 11 83 58 01 00 00` | (앞서 식사에서 동일) |

### 2.16 📊 UI / 진행도

| 심볼 | 활성 | AOB 패턴 | 효과 |
|---|---|---|---|
| `getProgressBar` | ☆ | `F3 0F 10 44 24 50` | 진행도 바 read 캡처 |
| `INJECT` (variant 1) | ☆ | `F3 0F 11 44 24 50` | (Visual Progress bars용 write) |
| `INJECT` (variant 2) | ☆ | `48 89 5C 24 10 57 48 83 EC 40 0F 57 C0 48 8B DA F3 0F 11 44 24 50 48 8B F9` | 더 긴 패턴으로 함수 진입점 직접 매칭 |

---

## 3. 공통 인젝션 디자인 패턴 — 분류

CT의 80+ 인젝션을 메커니즘 단위로 분류하면 5가지로 압축된다.

### 패턴 A — **명령어 NOP 화** (가장 단순)

```asm
; 원본:  subss xmm0,xmm2     (스태미나 차감)
staminaSub:
  db 90 90 90 90              ; 그냥 NOP 4바이트
```
적용: `staminaSub`, `writesToItems`, `writeMachineHealth`, `subStillToCreateItems`, `foodTimerWrite`, `durabilityWrite`, `doesFoodHungerSub`, `ammoDecrease` …

> **본 프로젝트 변환**: MinHook 후킹 후 detour에서 즉시 return — 또는 더 가벼운 `VirtualProtect + memcpy(NOP)` 패치로도 가능.

### 패턴 B — **레지스터 강제 값**

```asm
newmem:
  mov edi,64                 ; 새 펄 스탯을 100으로 강제
code:
  mov r14d,00000065          ; 원본
  jmp return
```
적용: `palStatsCreation` (스탯 100), `takeStackTwo` (r9d=6989), `getCaptureStrength` (포획 강도 부풀림), `movCaptureChance` (xmm0=1.0), `enableInsanityMode`, `arenaRPGainMult`, `researchIncrease` …

> **본 프로젝트 변환**: detour에서 인자/반환값을 가공해 원본 호출. 단, 이 패턴은 함수 중간 명령어를 가로채는 것이라 *함수 단위 후킹*보다 *trampoline inline patch*가 더 정확. MinHook의 Mid-function 후킹은 미지원이므로 직접 inline assembly + naked detour가 필요할 수 있음.

### 패턴 C — **분기 강제** (Conditional → Unconditional)

```asm
newmem:
  mov rax,[rbx+00000630]
  mov byte ptr [rax+00000598],00   ; 검사 플래그 강제 0
code:
  jmp return
```
적용: `canCatchBossPal`, `buildableTest`, `temperature`, `comparesSpheresZero`, `testForInvFull`, `testEnoughAncientTechPoints`, `technologyCheckIfCanUnlock` …

### 패턴 D — **포인터 캡처 (Save Space)**

```asm
label(getPlayerPtrSaveSpace)
registersymbol(getPlayerPtrSaveSpace)

newmem0:
  mov [getPlayerPtrSaveSpace],rcx    ; ★ 핵심 — 함수 진입 시 this 포인터 저장

code0:
  mov rax,[rcx+00000128]              ; 원본
  jmp return0

getPlayerPtrSaveSpace:
dq 0
```
적용: `getPlayerPtr` (★★★ Get Player Pointers의 root), `mobsTakeDamgBossF` (saveSpace+1: player ptr, +9: health holder), `getItemsInventoryOpen`, `getItemOnHover`, `getCameraPointer`, `getFishing` 류, `loadCurrentExpPointerOnInvOpen`, `technologyCostSubtract`(saveSpace+0: tech 포인터) …

> **본 프로젝트 변환**: 핵심 함수에 후킹 → detour 진입 시 `static void* g_playerPtr = (void*)thisArg;`. Track 1(MemoryCheats)의 모든 포인터 체인이 패치 후 깨질 때, 이 후킹들이 백업 경로가 됨.

### 패턴 E — **플래그 비트맵 + 다기능 토글**

```asm
mobsTakeDamgBossFSaveSpace:
db 0          ; bit0=MaxHP, bit1=TakeNoDmg, bit2=OHK, bit3=ImmortalEnemy, bit4=AlmostKill
dq 0          ; pointer to player
dq 0          ; healthHolder
```
하위 CheatEntry들은 단순 Lua로 `setBit(1, offset, bit, baseAdress)` 호출만 함. 하나의 AOB 인젝션이 5개의 토글을 동시 제공.

> **본 프로젝트 변환**: detour 안에서 `Menu::Config.bGodMode / bOHK / bImmortalEnemy / bAlmostKill` 비트마스크 분기 — 이미 본 프로젝트의 `Menu::Settings` 패턴과 정확히 같음.

---

## 4. 특수 파일·모듈 의존성

| 외부 의존 | 등장 위치 | 비고 |
|---|---|---|
| `EOSSDK-Win64-Shipping.dll` | `palCondensation` AOB의 모듈 인자 | 본 프로젝트 Scanner도 모듈명 인자 받으므로 동일 호출로 커버 가능 |
| Cheat Engine `setupPattern()` Lua 헬퍼 | 거의 모든 인젝션 ENABLE 블록 첫 줄 | 같은 심볼 재등록 방지 — 본 프로젝트는 MinHook이 그 책임을 짐 |
| Cheat Engine `setBit(...)` Lua 헬퍼 | 비트맵 토글 sub-entries | 본 프로젝트는 `Menu::Config` bool 토글로 1:1 대체 |

---

## 5. 외부 CT가 노출한 멤버 오프셋 — 본 프로젝트 SDK에 합류시킬 후보

> 인젝션 패턴 안의 `[reg + N]` 형태 메모리 접근에서 추출. 모두 클라이언트 바이너리 기준이며, 본 프로젝트 `SDK/Offsets.h` 에 후속 작업으로 등록할 가치가 높음.

### 5.1 IndividualCharacterParameter 부근

| 오프셋 | 의미 | 출처 |
|---|---|---|
| `+0x400` | HP (이미 등록됨) | `mobsTakeDamgBossF` saveSpace 분기 |
| `+0x418` | Stamina related (Inf Stamina 변종) | `infStamina` |
| `+0x488` (구) / `+0x410` (신) | ShieldHP write 분기 | `shieldSubtract` |
| `+0x5F8` | Max Health (read 시) | `mobsTakeDamgBossF` "Max Out Health" 비트 |
| `+0x600`±  | OHK 분기에서 0 강제 | `mobsTakeDamgBossF` |

### 5.2 펄 / Capture 관련 (UPalIndividualCharacterParameter 또는 GameSetting)

| 오프셋 | 의미 | 출처 |
|---|---|---|
| `+0x60`  | ally(1)/enemy(2) 분류자 | `mobsTakeDamgBossF`, `guaranteeCatch` |
| `+0x630` | 보스/일반 캐릭터 데이터 ptr | `canCatchBossPal` |
| `+0x598` (sub `+0x548`) | 보스 잡기 가능 플래그 | `canCatchBossPal` |
| `+0x6C`  | 새 펄 스탯 base | `palStatsCreation` |

### 5.3 PlayerCharacter / Movement / Camera

| 오프셋 | 의미 | 출처 |
|---|---|---|
| `+0x128` | 플레이어 객체 (this+0x128) — saveSpace의 root | `getPlayerPtr` |
| `+0x1D0` | (xmm6 read) 추정 회전/속도 | `getPlayerPtr` 같은 함수 후속 |
| `+0x260` | xmm6 부근 — 카메라/캐릭터 위치 | `getCameraPointer` |
| `+0x278`±| 부속 component | Inf Stamina 분기 |

### 5.4 Inventory / ItemSlot

| 오프셋 | 의미 | 출처 |
|---|---|---|
| `+0x154` | StackCount (이미 등록) | takeStack 군 — 일치 ★ |
| `+0x12B` (cmp) | 슬롯 가용 플래그 | `testForInvFull` |
| `+0x40C` | Food on hover (음식 카운터) | `getFoodOnHover`, `doesFoodHungerSub` |
| `+0x158` | foodTimer | `foodTimerWrite` |
| `+0x394` | 또 다른 음식 분기 | `foodDoesNotDecrease` |

### 5.5 Tech / Research

| 오프셋 | 의미 | 출처 |
|---|---|---|
| `+0x150` | TechnologyPoint (이미 등록 ★) | `technologyCostSubtract` — 일치 |
| `+0x154` | AncientTechnologyPoint (이미 등록 ★) | `noAncientTechCost` — 일치 |
| `+0x14`  | 연구 진행도 float | `researchIncrease` |
| `+0x338` | Exp 분기 root | `getExpRdx`, `loadCurrentExpPointerOnInvOpen` |

### 5.6 환경 / 무게

| 오프셋 | 의미 | 출처 |
|---|---|---|
| `+0x180` | weight current | `loadsWeight` |
| `+0x184` | weight max | `loadsWeight` |
| `+0x138` (구) `+0x160` (신) | 무게 분기 위치 변동 | `loadsWeight` 변종 |
| `+0x138` | 온도 분기 | `temperature` |

> ★ 표시는 본 프로젝트 `SDK/Offsets.h` 에 이미 동일 값으로 존재해 일치 확인된 항목.

---

## 6. 본 프로젝트 흡수 액션 플랜 (우선순위)

### 6.1 P0 — 즉시 적용 가능 (단일 NOP 패치형)

이미 분석된 AOB + 단순 NOP 으로 끝나는 인젝션. `SDK::Signatures` 에 시그니처 등록 + 후킹 detour에서 원본 호출 skip 만으로 동등한 효과.

| 신규 후킹 모듈 | 시그니처 | detour 동작 |
|---|---|---|
| `StaminaSubHook` | `F3 0F 5C C2 F3 48 0F 2C C0 48 89 01 48 8B C1` | 토글 ON 이면 NOP / 원본 skip |
| `WriteItemsHook` | `44 89 B3 54 01 00 00 45` | 아이템 write 차단 |
| `DurabilityWriteHook` | `F3 0F 11 41 10 F3 0F 11 51` | 내구도 write 차단 |
| `FoodTimerHook` | `F3 0F 11 83 58 01 00 00` | 음식 timer write 차단 |

### 6.2 P1 — 포인터 캡처용 후킹 (Track 1 백업 경로)

본 프로젝트 Track 1은 GWorld(0x090D3030) 시작 포인터 체인에 의존 → 패치 시 깨짐. 아래 후킹들은 saveSpace 식 포인터 캡처를 제공해 GWorld 변경에도 즉시 복원 가능.

| 신규 후킹 | 시그니처 | 캡처 대상 |
|---|---|---|
| `PlayerPtrCaptureHook` | `48 8B 81 28 01 00 00 48 8B DA 0F 29 74 24 20 F3 0F 10 B1 D0 01 00 00 48 85 C0 75` | 플레이어 객체 (`rcx`) |
| `ItemSlotCaptureHook` | `4A 20 83 B9 54 01 00 00 00` | 인벤토리 슬롯 배열 |
| `TechPointCaptureHook` | `2B C1 89 87 50 01 00 00` | TechnologyPoint 포인터 (`[rdi+0x150]`) |
| `ExpPtrCaptureHook` | `CC 48 8B 81 38 03 00 00 C3` | 현재 EXP 포인터 (`[rcx+0x338]`) |

### 6.3 P2 — 조건부 분기 강제 (`Track 2: HookCheats` 적합)

| 후킹 | 시그니처 | 효과 |
|---|---|---|
| `BuildAnywhereHook` | `40 38 B1 E4 00 00 00` | Can Build Anything |
| `CanCatchBossHook` | `48 8B 83 30 06 00 00 80 B8 98 05 00 00 00` | 보스 포획 가능 |
| `TempNormalizeHook` | `48 8B 9C 24 B0 00 00 00 39 87 38 01 00 00` | 새 지역 체온 정상화 |
| `CaptureChanceHook` | `F3 0F 10 44 24 38 ... 48 8B 5C 24 ** 48 83 C4 ** 5F C3 CC` | 포획률 100% (xmm0=1.0) |

### 6.4 P3 — 안티치트 탐지 룰 후보

본 프로젝트의 다른 트랙(방어/탐지) 에서는 다음 검사가 실용적:

| 룰 | 검사 방법 |
|---|---|
| 핵심 함수 첫 5바이트 무결성 | `staminaSub`, `writesToItems`, `durabilityWrite` 시작 주소 + 5바이트가 `E9 ?? ?? ?? ??` (jmp) 인지 |
| EOSSDK 외부 인젝션 탐지 | `EOSSDK-Win64-Shipping.dll` + 0x?? 부근에 외부 trampoline 할당 흔적 |
| GameSetting 무결성 (CT #1) | `IgnoreFirstCaptureFailedHPRate ≈ 0.3` 주기 검증 |
| 배율형 변조 탐지 | EXP, ArenaRP, ResearchIncrease 등의 단발 증가량이 정상 범위(`<= upperBound`) 인지 (`getExpRdx`, `arenaRPGainMult`, `researchIncrease`) |
| 인벤토리 비정상 카운트 | `StackCount > MaxStack(보통 9999)` 인 슬롯이 있으면 fragged |

---

## 7. 요약 — 한눈 보기

| | `Palworld_Hack.CT` (CT #1) | `Palworld-Win64-Shipping.CT` (CT #2) |
|---|---|---|
| AOB 스캔 수 | 1 (`9A 99 99 3E`) | **80+ 고유 시그니처** |
| 코드 인젝션 수 | 0 (Lua 메모리 변조만) | **~75 AssemblerScript** |
| 본 프로젝트 활용도 | 멤버 오프셋 검증용 | **Track 2 직접 이식 가능** ★ |
| 안티치트 탐지에 미치는 영향 | 단일 GameSetting 무결성 룰 | 함수 첫 바이트 무결성 + 메모리 alloc 패턴 + 변조량 임계 등 다층 룰 필요 |

### 다음 추천 작업

1. **P0 4종**(Stamina/Items/Durability/Food) 시그니처를 `SDK/Signatures.h` 에 등록 후 `HookCheats/` 에 1개씩 모듈 추가 (`ExampleHook.cpp` 복붙으로 시작).
2. **`PlayerPtrCaptureHook`** 부터 우선 구현하면 본 프로젝트의 GWorld(0x090D3030) 패치 의존성을 단번에 제거할 수 있어 ROI가 가장 높음.
3. 분석 후 본 문서를 `SDK/Signatures.h` 에 인라인 주석으로 분산해, 시그니처 옆에 출처(이 문서의 §2.X) 와 매치 위치 RVA(예: `Palworld-Win64-Shipping.exe.text+2D1388F`) 를 함께 적어두면 패치마다의 검증이 빠름.
