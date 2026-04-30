# HookCheats — Track 2: AOB 시그니처 + 함수 후킹

> **게임 함수 자체를 가로채서 동작을 바꾸는 트랙. 안정적이고 안티치트에 덜 노출.**

## 🎯 이 트랙을 언제 쓰나

**적합한 경우**
- 데미지 *계산 로직* 변경 (`TakeDamage` 후킹 → return 0)
- 자원 *소모* 무력화 (`ConsumeAmmo`, `StaminaDecrement` 후킹 → no-op)
- 행동 가로채기 (`APalCharacter::Tick` 후킹 → 위치 정보 수집해서 ESP)
- 안티치트 우려가 있는 핵심 기능 (메모리 폴링은 탐지되기 쉬움)

**부적합한 경우 → Track 1(`MemoryCheats`) 권장**
- 단순히 메모리에 박힌 값 한두 개만 바꾸면 되는 경우
- 함수 분석 시간을 들일 가치가 없는 보조 기능

---

## ⚙️ 동작 원리

```
SDK/Signatures.h          "48 89 5C 24 ? 57 48 83 EC 20 ..."
        │
        ▼  Scanner::FindPattern  (게임 모듈 메모리 선형 스캔)
   함수 주소 (uintptr_t)
        │
        ▼  HookManager::CreateHook (MinHook이 함수 첫 바이트를 jmp로 덮어씀)
   ┌────────────────────┐
   │ 게임이 함수 호출    │
   └─────────┬──────────┘
             ▼
   ┌────────────────────┐
   │ 우리 detour 실행    │ ← 여기서 동작 변조
   │  (조건부로 oOriginal│
   │   호출 or skip)    │
   └────────────────────┘
```

---

## 🔍 AOB 시그니처 추출 워크플로우

### IDA Pro / IDA Free
1. 게임 EXE (`Palworld-Win64-Shipping.exe`) 로드
2. 함수 찾기:
   - 문자열 검색 (Shift+F12) — 함수 안 디버그 문자열
   - cross-reference (Ctrl+X) — 호출 관계 추적
   - 시그니처 검색 (UE4SS dump의 알려진 함수와 매칭)
3. 함수 첫 명령어들 선택 (보통 16~24바이트)
4. **SigMakerEx** 또는 **MakeSig** 플러그인으로 IDA-style 문자열 추출
5. 변하기 쉬운 부분(immediate 상수, 주소 displacement)은 `?`로 마킹

### Ghidra
- File → Export → Search Memory 로 패턴 탐색 검증
- "Make Pattern" 같은 스크립트 활용

### x64dbg
1. 게임 attach
2. 함수 시작 주소로 이동 (`Ctrl+G`)
3. hex 모드로 첫 바이트들 복사
4. SigMaker 플러그인 사용

### 검증
- 추출한 패턴을 게임 모듈 전체에서 재검색해 **고유**한지 확인 (1개만 매칭)
- 게임 재시작 / 다른 맵에서도 같은 함수를 가리키는지 확인
- 다른 빌드(예: 작은 패치 후) 에도 살아남는지 가능하면 확인

---

## 📝 새 후킹 치트 추가 — 단계별 가이드

예시: **"DamageHook"** — `APalCharacter::TakeDamage` 후킹으로 GodMode를 강화.

### Step 1. 함수 분석
- **함수**: `void APalCharacter::TakeDamage(this, FDamageEvent*, float dmg)`
- **호출 규약**: x64 fastcall
- **인자 위치**:
  - `RCX` = `this` (APalCharacter*)
  - `RDX` = `FDamageEvent*`
  - `XMM2` = `float damage` (float은 XMM 레지스터로)

### Step 2. `SDK/Signatures.h` 등록
```cpp
namespace SDK::Signatures
{
    inline constexpr const char* TakeDamage =
        "48 89 5C 24 ? 57 48 83 EC 20 48 8B F9 ? ? ? ?";
}
```
> 예시일 뿐 실제 시그니처는 분석 결과로 교체.

### Step 3. 이 폴더에 신규 파일 — `DamageHook.h` / `.cpp`

`DamageHook.h`
```cpp
#pragma once
namespace DamageHook { bool Install(); }
```

`DamageHook.cpp` — `ExampleHook.cpp`를 복붙해서 시작
```cpp
#include "DamageHook.h"
#include "../../Memory/HookManager.h"
#include "../../Memory/Scanner.h"
#include "../../SDK/Signatures.h"
#include "../../GUI/Menu.h"
#include <iostream>

namespace DamageHook
{
    using TakeDamageFn = void(__fastcall*)(void* self, void* dmgEvent, float dmg);
    static TakeDamageFn oTakeDamage = nullptr;

    static void __fastcall hkTakeDamage(void* self, void* dmgEvent, float dmg)
    {
        if (Menu::Config.bGodMode) return;     // 데미지 함수 자체 skip
        oTakeDamage(self, dmgEvent, dmg);
    }

    bool Install()
    {
        const char* sig = SDK::Signatures::TakeDamage;
        if (!sig || !*sig) {
            std::cout << "[*] DamageHook: signature empty, skip.\n";
            return true;
        }
        uintptr_t addr = Scanner::FindPattern("Palworld-Win64-Shipping.exe", sig);
        if (!addr) {
            std::cout << "[-] DamageHook: pattern not found.\n";
            return false;
        }
        std::cout << "[+] DamageHook: TakeDamage @ 0x" << std::hex << addr << std::dec << "\n";
        if (!HookManager::CreateHook((void*)addr, &hkTakeDamage, (void**)&oTakeDamage)) {
            std::cout << "[-] DamageHook: install failed.\n";
            return false;
        }
        std::cout << "[+] DamageHook installed.\n";
        return true;
    }
}
```

### Step 4. `Core/HackMain.cpp` 와이어링
```cpp
// 상단 인클루드
#include "../Cheats/HookCheats/DamageHook.h"

// Initialize() 안, HookManager::Initialize() 다음에:
DamageHook::Install();
```

### Step 5. `Project/CMakeLists.txt` 등록
```cmake
# Track 2: AOB + 함수 후킹 치트
Cheats/HookCheats/ExampleHook.cpp
Cheats/HookCheats/DamageHook.cpp     # ← 추가
```

### Step 6. 빌드 + 테스트
1. 인젝션 → 콘솔 로그에 `[+] DamageHook: TakeDamage @ 0x...` 확인
2. 메뉴에서 GodMode 토글 후 인게임에서 데미지 받아보기
3. `END` 디테치 시 후킹 정상 해제되는지 확인 (재진입 시 메뉴 다시 작동)

---

## 🛠️ Detour 작성 패턴

### 패턴 A — 완전 차단 (원본 호출 skip)
```cpp
static void __fastcall hkFn(void* self) {
    if (Menu::Config.bSomething) return;   // 함수가 아예 실행 안 됨
    oFn(self);
}
```

### 패턴 B — 인자 수정 후 원본 호출
```cpp
static void __fastcall hkFn(void* self, float dmg) {
    if (Menu::Config.bGodMode) dmg = 0.0f;
    oFn(self, dmg);
}
```

### 패턴 C — 원본 호출 후 결과 가공
```cpp
static int __fastcall hkFn(void* self) {
    int result = oFn(self);
    if (Menu::Config.bMaxAmmo) result = 999;
    return result;
}
```

### 패턴 D — 정보 수집 (ESP, 로깅)
```cpp
static void __fastcall hkTick(void* self, float dt) {
    GameState::cachedActors.push_back(self);   // 매 프레임 actor 위치 수집
    oTick(self, dt);
}
```

---

## 📐 호출 규약 / 인자 cheatsheet (x64 Windows)

| 함수 종류 | this | 인자 1 | 인자 2 | 인자 3 | 인자 4 | 그 이후 |
|---|---|---|---|---|---|---|
| **static / 일반** | -- | RCX | RDX | R8 | R9 | stack |
| **멤버 함수** | RCX | RDX | R8 | R9 | stack | stack |
| **float / double 인자** | -- | XMM0 | XMM1 | XMM2 | XMM3 | stack |

C++로 표현:
```cpp
// static 함수
using StaticFn = ReturnT(__fastcall*)(Arg1, Arg2);

// 멤버 함수 (this 포함)
using MemberFn = ReturnT(__fastcall*)(ThisType* self, Arg1, Arg2);

// 가상 함수 — vtable에서 찾아서 후킹 (별도 분석 필요)
```

---

## ⚠️ 자주 하는 실수

| 실수 | 증상 | 해결 |
|---|---|---|
| 호출 규약 불일치 | 인자가 엉망, 즉시 크래시 | `__fastcall` + 정확한 인자 순서 확인 |
| 패턴이 너무 짧음 | 다른 함수에 매칭 → 엉뚱한 곳 후킹 | 16바이트 이상, 검증 단계 거치기 |
| `oOriginal` 안 쓰고 함수명 직접 호출 | 무한 재귀, 스택 오버플로우 | 반드시 `oOriginal(...)` 호출 |
| detour 안에서 예외 발생 | 게임 전체 크래시 | 위험 코드는 `__try / __except`로 감싸기 |
| `bGodMode` 같은 토글 검사 누락 | 항상 활성화되어 게임 부자연스러움 | 모든 detour는 토글 분기 권장 |
| Install 호출 누락 | 컴파일 OK인데 후킹 안 됨 | HackMain의 `Initialize()` 에 호출 추가 확인 |

---

## 🧪 디버깅 팁

- `Scanner::FindPattern` 결과를 콘솔에 찍기. 0이 나오면 패턴 자체가 안 맞는 것
- 후킹 직후 `oOriginal` 만 호출하는 통과형 detour로 시작 → 콘솔 로그로 호출 빈도 확인 후 로직 추가
- 게임 시작 직후엔 일부 함수가 아직 메모리에 안 올라온 경우 있음 → 인게임 진입 후 재인젝션
- 재진입 시 hook 잔존 여부는 `HookManager::Shutdown()`이 처리. 디테치 후 게임 즉시 종료되면 detour 잔재 의심

---

## 📦 현재 구현된 후킹
- **`ExampleHook.cpp`** — TakeDamage 후킹 skeleton. `SDK::Signatures::TakeDamage` 가 빈 문자열이라 install 시 자동 skip. 실제 시그니처를 채우는 즉시 다음 실행에서 활성화됨.
