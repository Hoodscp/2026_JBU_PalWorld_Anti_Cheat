# Palworld-Win64-Shipping.CT 분석 보고서

> **대상 파일**: `JBU_Pal_Hack/Reference/CT_Files/Palworld-Win64-Shipping.CT`
> **포맷**: Cheat Engine Table (XML) — `CheatEngineTableVersion="45"`
> **규모**: 약 16,849 lines / 990 KB / `CheatEntry` ≈ **340개**
> **타겟 모듈**: 주로 `Palworld-Win64-Shipping.exe`, 일부 `EOSSDK-Win64-Shipping.dll`

본 문서는 CT 파일의 모든 치트 항목을 **(1) 단순 메모리 변조 (Simple Memory Manipulation)** 와 **(2) AOB 패턴 기반 코드 인젝션 (Auto Assembler Script + AOB)** 두 카테고리로 나누어 정리한다. 이는 안티치트(Anti-Cheat) 측면에서 어떤 종류의 탐지/방어가 필요한지 파악하기 위한 자료다.

---

## 0. 카테고리 정의

| 분류 | 판별 기준 | 안티치트 관점 |
|------|----------|---------------|
| **A. 단순 메모리 변조** | `VariableType`이 `4 Bytes / Float / Double / Byte / String / Pointer` 이고, `<Offsets>`(포인터 체인)을 따라 값만 읽고 쓰는 항목. AssemblerScript 없음. | 주기적인 값 검증(서버 권위 / 미러 변수 / 체크섬)으로 막을 수 있다. 정적인 변조라 탐지 비용이 낮다. |
| **B. AOB + Auto Assembler 인젝션** | `VariableType = Auto Assembler Script`. `aobscanmodule(...)` 로 명령어 시그니처를 찾고 `alloc(newmem, ...)` → `jmp newmem` 으로 코드 케이브를 만들어 실행 흐름을 변조하는 항목. | 명령어 영역의 무결성 검사(.text 체크섬, AOB 자기점검), VEH/페이지 권한 감시, 핫패치 탐지 등이 필요. 탐지가 어려운 고급 변조. |

---

## 1. 카테고리 A — 단순 메모리 변조 (≈ 150+ 항목)

대부분 `getPlayerPtr` 등 한 번의 AOB 스캔으로 얻은 **베이스 포인터를 저장**해 두고, 이후 모든 자식 항목은 그 포인터에 **오프셋만 더해 값에 직접 접근**하는 구조다. AOB 자체는 1회성이며, **런타임 동작은 순수한 R/W**이다.

### 1.1 플레이어 본인 스탯 (Player Pointer 기반)

베이스: `getPlayerPtrSaveSpace`  (시그니처: `48 8B 81 28 01 00 00`)

| Description | Type | 오프셋 경로 |
|-------------|------|------------|
| Level | Byte | `+398 → +128` |
| Exp | 4 Bytes | `+3A0 → +128` |
| Health | 4 Bytes | `+3F0 → +128` |
| Workspeed | 4 Bytes | `+464 → +128` |
| Shield | 4 Bytes | `+478 → +128` |
| Max Shield | 4 Bytes | `+480 → +128` |
| Max Stamina | 4 Bytes | `+490 → +128` |
| Current Hunger | Float | `+3FC → +128` |
| Max Hunger | Float | `+4E4 → +128` |
| Stat Points | 4 Bytes | `+4E4 → +128` |
| Arena RP | 4 Bytes | `+660 → +128` |

### 1.2 팀 멤버 (Pal 1~5) 스탯

베이스: `getFoodOnHover`  (시그니처: `F3 0F 10 81 0C 04 00 00`)
구조: **5명 × 13항목 ≈ 65 entry**

| Description (per Pal) | Type |
|----------------------|------|
| Health | 4 Bytes |
| Workspeed | 4 Bytes |
| Sanity | Float |
| Pal Level | Byte |
| Pal Exp | 4 Bytes |
| Pal Rank (0~4) | Byte |
| Max Stamina | 4 Bytes |
| Food / Max Food | Float |
| Trust | 4 Bytes |
| Potential Health / Attack / Defense | Byte |

### 1.3 인벤토리 슬롯 (40 슬롯)

베이스: `getItemsInventoryOpen`  (시그니처: `4A 20 83 B9 54 01 00 00 00`)

- `Item 1` ~ `Item 40` (각 4 Bytes) → 슬롯별 수량 직접 변조
- `Clear` (리셋 버튼)

### 1.4 건설/제작 재료 슬롯 (50 슬롯)

베이스: `getCraftingItems`  (시그니처: `8B 8F 54 01 00 00 01`)

- Items `1–10 / 11–20 / 21–30 / 31–40 / 41–50` (각 4 Bytes)

### 1.5 기타

- `Player Pointer` (8 Bytes) — 베이스 포인터 자체
- Dummy / Separator 엔트리들 (`---------------------------------`)

> **A 카테고리 합계**: 약 **150+ entry**. 모두 **포인터 + 오프셋 → 값 R/W** 형태로, 코드 영역은 일절 건드리지 않는다.

---

## 2. 카테고리 B — AOB + Auto Assembler 코드 인젝션 (≈ 60+ 항목)

다음 형태의 패턴을 가진 항목들이다:

```
aobscanmodule(<symbol>, Palworld-Win64-Shipping.exe, <hex pattern>)
alloc(newmem, $1000, <symbol>)
label(code)
label(return)

newmem:
   ; 원본 명령 + 치트 로직 (값 덮어쓰기 / nop / 비교 우회 등)
   jmp return

<symbol>:
   jmp newmem
return:
```

### 2.1 생존계 (Stamina / Hunger / Sanity / Temperature / Weight)

#### 2.1.1 Infinite Stamina
- **Symbol**: `staminaSub` (`F3 0F 5C C2 F3 48 0F 2C C0 48 89 01 48 8B C1`)
- **Hook 위치**: `subss xmm0,xmm2` @ `Palworld-Win64-Shipping.exe+2FFBAD0`
- **동작**: 감소 명령을 `90 90 90 90` (NOP)으로 패치
- **대상 기능**: 플레이어 스태미나 무한

#### 2.1.2 Never Get Hungry (메인)
- **Symbol**: `doesFoodHungerSub` (`F3 0F 5C D7 0F 28 7C 24 40 0F 2F D1 72 07`)
- **동작**: `subss`로 배고픔이 감소하는 라인을 무력화

#### 2.1.3 Never Get Hungry (대체)
- **Symbol**: `foodDoesNotDecrease` (`F3 0F 11 87 94 03 00 00 48`)
- **Hook**: `[rdi+00000394]` 쓰기 명령 → NOP

#### 2.1.4 Freeze Food Counter (조리/숙성 타이머 정지)
- **Symbol**: `foodTimerWrite` (`F3 0F 11 83 58 01 00 00`)
- **Hook**: `[rbx+00000158]` 쓰기 → NOP

#### 2.1.5 Max Sanity
- **Symbol**: `changeSanity` (`F3 41 0F 58 C0`)
- **동작**: `addss` 결과를 최대값으로 고정 또는 항상 가산

#### 2.1.6 Always Normal Temperature
- **Symbol**: `temperature` (`48 8B 9C 24 B0 00 00 00 39 87 38 01 00 00`)
- **Hook**: `[rdi+00000138]` 비교를 항상 통과/정상값으로 강제

#### 2.1.7 Zero Weight
- **Symbol**: `loadsWeight` (`F3 0F 10 8B 80 01 00 00 …`)
- **Hook**: `[rbx+00000180]` 로드 결과를 0으로 변조 → 무게 제한 무력화

---

### 2.2 전투/체력/방패 (Damage / Health / Shield)

#### 2.2.1 Shield Does Not Decrease
- **Symbol**: `shieldSubtract` (`48 8B 81 88 04 00 00`)
- **동작**: `[rcx+0x488]` 읽기 후 `xor edx,edx` → 감산값을 0으로

#### 2.2.2 Damage/Health 종합 패치 (Player vs Enemy 분기)
- **Symbol**: `mobsTakeDamgBossF` (`48 8B 80 88 03 00 00 48 89 45 58 45 8B C4`)
- **Hook**: `[rax+0x400]`(엔티티), `[rax+60]`(아군/적군 플래그)
- **비트별 옵션**:
  - bit0 — **Max Out Health** (`[rax+0x5F8]` 저장값 사용)
  - bit1 — **Take No Damage** (`xor r15d,r15d`)
  - bit2 — **OHK** (`xor rax,rax` → 적 HP=0)
  - bit3 — **Immortal Enemy** (`xor r15d,r15d`)
  - bit4 — **Almost Kill** (`r15d /= 1000`)

#### 2.2.3 One-Hit Machines
- **Symbol**: `writeMachineHealth`
- **동작**: 기계 구조물 체력 쓰기 시 1로 축소

---

### 2.3 탄약 / Pal 능력 (Ammo)

#### 2.3.1 Infinite Ammo
- **Symbols**:
  - `ammoDecrease` (`8D 42 FF 89 81 84 00 00 00`)
  - `ammoTestZero` (`83 B8 84 00 00 00 00`)
- **Hook**: `[rcx+0x84]` (탄창 카운트)
- **동작**: 감소 명령을 `xor`로 무력화, 0 비교 시 1로 재설정

#### 2.3.2 Inf Pal Ability Ammo (예: Jet Dragon)
- **Symbol**: `jetDragonAmmoDecrease` (`8B 02 29 01 48 8B C1`)
- **동작**: Pal 특수기술 탄약 감소 `sub [rcx], eax`를 회피

#### 2.3.3 Infinite Sphere (포획구)
- **Symbol**: `comparesSpheresZero` (`83 BE 54 01 00 00 00`)
- **동작**: 비교 직전에 `inc dword ptr [rax+0x154]` → 0 도달 방지

---

### 2.4 포획 (Capture)

#### 2.4.1 High Capture Rate
- **Symbol**: `getCaptureStrength` (`8B 48 08 33 C0 85 C9 0F 4F C1 C3 CC CC CC CC 48 8B C2`)
- **동작**: `mov ecx,[rax+8]; add ecx,10; imul ecx,#100` → 포획 강도 약 10배 증폭

#### 2.4.2 Guarantee Capture
- **Symbol**: `movCaptureChance` (`F3 0F 10 44 24 38`)
- **동작**: 포획 확률 float를 100% / 최대값으로 강제 로드

#### 2.4.3 Catch Bosses / Merchants
- **Symbol**: `canCatchBossPal` (`48 8B 83 30 06 00 00`)
- **동작**: `[rbx+0x630]` 보스/상인 포획 차단 플래그를 우회

---

### 2.5 Pal 육성 (Breeding / Stats / Shiny / Trust)

#### 2.5.1 New Spawns Have Perfect Stats
- **Symbol**: `palStatsCreation` (`41 BE 65 00 00 00`)
- **동작**: 새로 생성되는 팰의 IV/Potential을 최대로 강제

#### 2.5.2 New Spawns are Lucky (Shiny)
- **Symbol**: `shiny` (`0F 2F F0 0F 28 B4 24 30 04 00 00`)
- **동작**: shiny 확률 비교 결과를 항상 통과

#### 2.5.3 Quick Breeding
- **Symbol**: `quickBreeding` (`F3 0F 58 8F 48 02 00 00 …`)
- **Hook**: `[rdi+0x248]` 번식 진행도 → 가산값 증폭/조건 우회

#### 2.5.4 Trust 증가 가속
- **Symbols**: `trustTickerUnsummoned` (`8B 87 50 06 00 00`), `trustTickerSummoned` (`8B 87 48 06 00 00`)
- **Hook**: `[rdi+0x650] / [rdi+0x648]` 신뢰도 카운터

#### 2.5.5 Pal Condensation Remove (WIP)
- **Symbol**: `palCondensationRemove` (`29 7B 08 44 8B C5 8B D6 48 8B CB E8 BC`)
- **동작**: 응축 시 팰 차감 라인을 우회

---

### 2.6 자원/제작/내구도

#### 2.6.1 Infinite Durability
- **Symbol**: `durabilityWrite` (`F3 0F 11 41 10 F3 0F 11 51`)
- **동작**: 내구도 `[rcx+0x10]` 쓰기 NOP

#### 2.6.2 Crafting/Building Items Don't Decrease
- **Symbols**: `craftingItemsStugg`, `createStackWise`, `takeStack`, `writesToItems`, `stackBytes` 등
- **패턴 예**: `4C 8B 41 24 48 8B DA`, `39 83 54 01 00 00 0F`, `8B 81 54 01 00 00 0F 10`
- **동작**: 제작/건설/스택 차감 로직 우회 → 자원 무한

#### 2.6.3 Technology Cost = 0
- **Symbol**: `technologyCostSubtract` (`2B C1 89 87 50 01 00 00`)
- **Hook**: `[rdi+0x150]` 기술 포인트 sub 무력화

#### 2.6.4 No Ancient Tech Cost
- **Symbol**: `noAncientTechCost` (`2B C1 89 87 54 01 00 00`)
- **Hook**: `[rdi+0x154]` 고대 기술 포인트 sub 무력화

#### 2.6.5 Tech / Item Recipe Unlock All
- **Symbols**: `getCraftRecipyUnlocke`, `getItemRecipyUnlocke`, `technologyCheckIfCanUnlock`
- **동작**: 해금 조건 비교를 항상 통과

#### 2.6.6 Research Speed
- **Symbol**: `researchIncrease` (`F3 0F 58 77 14`)
- **Hook**: `[rdi+0x14]` 연구 진행도 가산값 증폭

---

### 2.7 경험치 / RP

#### 2.7.1 Exp Multiplier / Flat Exp Gain
- **Symbol**: `getExpRdx` (`4C 8B 91 38 03 00 00 44`)
- **Hook**: `[rcx+0x338]` 경험치 값 로드 후 `imul` / `add`

#### 2.7.2 Arena RP Gain Multiplier
- **Symbol**: `arenaRPGainMult` (`41 8D 0C 1F 0F B6 E8`)
- **동작**: `lea`로 RP 계산식을 변조

---

### 2.8 위치 / 카메라 (Teleport)

#### 2.8.1 Teleport (Dismount 상태 한정)
- **Symbol**: `getPlayerPos` (`0F 10 80 60 02 00 00`)
- **Hook**: `[rax+0x260]` 플레이어 좌표를 newmem에서 사용자가 설정한 좌표로 덮어씀

#### 2.8.2 Get Camera Pointer
- **Symbol**: `getCameraPointer`
- **용도**: 텔레포트 목적지 또는 시야 조작용 카메라 베이스 확보

---

### 2.9 낚시 / 건설 / 작업속도

#### 2.9.1 Instant Fishing
- **Symbols**: `getFishing`, `getFishFishingPos`, `getPlayerFishingPos`, `getFishingFishPost`
- **패턴**: `F3 0F 10 81 DC 00 00 00`
- **Hook**: `[rcx+0xDC]` 낚시 진행도 → 즉시 100%

#### 2.9.2 Work Speed Accelerator
- **Symbol**: `playerBuildingLoadWorkForce` (`66 0F 6E 8B FC 03 00 00`)
- **Hook**: `[rbx+0x3FC]` 작업속도 → 증폭

#### 2.9.3 Pal Workspeed Multiplier
- **Symbol**: `palsWorkspeedAcc`
- **동작**: 팰의 작업속도 값에 배수 적용

#### 2.9.4 Build Everywhere
- **Symbol**: `buildableTest` (`40 38 B1 E4 00 00 00`)
- **Hook**: `[rcx+0xE4]` 건설 가능 여부 → 항상 true

---

### 2.10 기타 / 부수 효과

| Symbol | 기능 |
|--------|------|
| `enableInsanityMode` (`F3 0F 59 91 8C 02 00 00`) | 정신 악화 가속 (WIP) |
| `getProgressBar` | 진행 바 값 강제 |
| `loadsHealthAndDmg` | 체력/피해 동시 로드 후크 |
| `loadCurrentExpPointerOnInvOpen` | 인벤토리 오픈 시 exp 포인터 확보 |
| `testEnoughAncientTechPoints` | 고대 기술 포인트 부족 검사 우회 |
| `palCondensation` (`EOSSDK-Win64-Shipping.dll`) | EOS DLL 내 팰 응축 후크 (이 항목만 별도 모듈 대상) |
| `staminaRead` / `loadStamina` / `doesStaminaStuff` (old) | 구버전 스태미나 우회 (0.6.1 이하) |
| `healthWhenAttacked`, `readsShield`, `ammoDec`, `garanteeCatch`, `infStamina`, `takeStackTwo/Three` (old) | 구버전 호환용 후크 |

---

## 3. 총괄 통계

| 분류 | 개수 | 비고 |
|------|------|------|
| **A. 단순 메모리 변조** | ≈ 150+ | 5인 팀 × 13 스탯 + 인벤 40 + 제작 50 + 본인 스탯 다수 |
| **B. AOB + Auto Assembler 인젝션** | ≈ 60+ | 이 중 한 AOB 심볼이 다수 자식 entry의 베이스로 재사용됨 |
| **C. 구분자/세팅/플레이스홀더** | ≈ 20+ | UI용 separator, 헤더, 토글 등 |
| **합계 CheatEntry** | **≈ 340** | |

---

## 4. 안티치트 관점 시사점 (요약)

### 4.1 카테고리 A 방어 우선순위
1. **권위 있는 변수의 서버 미러링** — Health, Stamina, Exp, Capture Rate 등은 클라이언트 RAM만 신뢰하지 말 것
2. **주기적 체크섬** — 인벤토리/스탯/팰 능력치 구조체에 XOR/CRC mirror
3. **이상 변화율 탐지** — 1 frame 내 비현실적 증가(예: Exp +∞, 무게 0) 플래깅

### 4.2 카테고리 B 방어 우선순위
1. **`.text` 무결성 검사** — Palworld-Win64-Shipping.exe 코드 페이지를 주기적으로 CRC/SHA 비교 (특히 위 AOB 시그니처 영역)
2. **알려진 AOB 패턴 자기-스캔** — 본 문서의 패턴 리스트를 안티치트 측에서 직접 스캔하여 매칭되는 코드 영역 주변에 `JMP`가 박혀 있는지 검사
3. **페이지 권한 모니터링** — RWX 페이지 할당 / `VirtualProtect` 변경 후킹
4. **EOSSDK-Win64-Shipping.dll** 모듈도 후킹 대상이므로 동일하게 검증
5. **핫키 입력 + 외부 프로세스(`CheatEngine.exe`, `dbk*`) 핸들 감시**

### 4.3 추가 관찰
- 거의 모든 AOB가 **상대적으로 짧고 일반적인 SSE float 명령**(`F3 0F 11`, `F3 0F 5C` 등)을 기준점으로 한다. 즉 게임 빌드가 업데이트되면 시그니처가 자주 깨지므로, **빌드별 .text 베이스라인 해시**를 두면 변조 탐지율이 매우 높다.
- `getPlayerPtr`, `getFoodOnHover`, `getItemsInventoryOpen`, `getCraftingItems` 4개의 베이스 후크가 **카테고리 A 항목 대부분을 떠받친다** → 이 4개 명령 영역만 보호해도 효과가 크다.

---

## 부록 A — AOB 시그니처 인덱스 (방어용 자체 스캔 리스트)

```text
getPlayerPtr                : 48 8B 81 28 01 00 00
getFoodOnHover              : F3 0F 10 81 0C 04 00 00
getItemsInventoryOpen       : 4A 20 83 B9 54 01 00 00 00
getCraftingItems            : 8B 8F 54 01 00 00 01
shieldSubtract              : 48 8B 81 88 04 00 00
mobsTakeDamgBossF           : 48 8B 80 88 03 00 00 48 89 45 58 45 8B C4
staminaSub                  : F3 0F 5C C2 F3 48 0F 2C C0 48 89 01 48 8B C1
ammoDecrease                : 8D 42 FF 89 81 84 00 00 00
ammoTestZero                : 83 B8 84 00 00 00 00
jetDragonAmmoDecrease       : 8B 02 29 01 48 8B C1
comparesSpheresZero         : 83 BE 54 01 00 00 00
getCaptureStrength          : 8B 48 08 33 C0 85 C9 0F 4F C1 C3 CC CC CC CC 48 8B C2
movCaptureChance            : F3 0F 10 44 24 38
canCatchBossPal             : 48 8B 83 30 06 00 00
palStatsCreation            : 41 BE 65 00 00 00
shiny                       : 0F 2F F0 0F 28 B4 24 30 04 00 00
quickBreeding               : F3 0F 58 8F 48 02 00 00 0F 2F C8 72 07 0F 28 C2
trustTickerUnsummoned       : 8B 87 50 06 00 00
trustTickerSummoned         : 8B 87 48 06 00 00
doesFoodHungerSub           : F3 0F 5C D7 0F 28 7C 24 40 0F 2F D1 72 07
foodDoesNotDecrease         : F3 0F 11 87 94 03 00 00 48
foodTimerWrite              : F3 0F 11 83 58 01 00 00
changeSanity                : F3 41 0F 58 C0
temperature                 : 48 8B 9C 24 B0 00 00 00 39 87 38 01 00 00
loadsWeight                 : F3 0F 10 8B 80 01 00 00 0F 28 F0 F3 0F 10 83 84 01 00 00 0F 2F C8
durabilityWrite             : F3 0F 11 41 10 F3 0F 11 51
technologyCostSubtract      : 2B C1 89 87 50 01 00 00
noAncientTechCost           : 2B C1 89 87 54 01 00 00
researchIncrease            : F3 0F 58 77 14
getExpRdx                   : 4C 8B 91 38 03 00 00 44
arenaRPGainMult             : 41 8D 0C 1F 0F B6 E8
getPlayerPos                : 0F 10 80 60 02 00 00
getFishing(*)               : F3 0F 10 81 DC 00 00 00
playerBuildingLoadWorkForce : 66 0F 6E 8B FC 03 00 00
buildableTest               : 40 38 B1 E4 00 00 00
palCondensationRemove       : 29 7B 08 44 8B C5 8B D6 48 8B CB E8 BC
enableInsanityMode          : F3 0F 59 91 8C 02 00 00
```

이 시그니처들은 안티치트의 **자기-스캔 베이스라인**으로 활용 가능하다. 각 패턴의 위치에 `JMP`/`CALL`이 박힌 흔적, 또는 인접 메모리에 RWX `newmem` 페이지가 할당된 흔적이 있다면 변조로 판정할 수 있다.
