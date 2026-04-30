# MemoryCheats — Track 1: 오프셋 기반 메모리 변조

> **포인터 체인을 따라가서 값을 직접 read/write 하는 단순/빠른 접근.**

## 🎯 이 트랙을 언제 쓰나

**적합한 경우**
- HP, 스태미나, 탄약, 경험치, 골드처럼 평문 값으로 저장된 데이터
- 한 번 분석한 오프셋이 비교적 안정적인 경우
- 빠르게 동작 확인이 필요한 프로토타입

**부적합한 경우 → Track 2(`HookCheats`) 권장**
- 데미지 *계산식* 변경 (이건 함수 후킹이 정답)
- 자원 *소모* 무력화 (`ConsumeAmmo` 후킹)
- 게임이 매 프레임 우리 write를 덮어쓰는 race 발생 시

---

## ⚙️ 동작 원리

```
GModule (게임 EXE 베이스 주소)
    │  + Offsets::Module::GWorld          ← 첫 분기점
    ▼
GWorld
    │  + Offsets::World::GameState
    ▼
GameState
    │  + Offsets::GameState::PlayerArray  (TArray)
    ▼  + 0  (TArray의 Data 포인터 = 첫 엘리먼트)
PlayerState
    │  + Offsets::PlayerState::PawnPrivate
    ▼
Pawn
    │  + Offsets::Pawn::CharacterParameterComponent
    ▼
CharParamComp
    │  + Offsets::CharParamComp::IndividualParameter
    ▼
IndividualParameter
    │  + Offsets::IndividualParam::HP
    ▼
*(int64_t*)addr = 새 값       ← 여기서 메모리 덮어쓰기
```

`SDK::Get*()` 헬퍼들이 각 단계를 한 함수씩 분리해서 노출하므로, **본인 치트가 필요한 분기점부터 재사용** 가능. 예를 들어 “시간 치트”는 `GetGameState()` 에서 분기, “이동 속도”는 `GetLocalPawn()` 에서 분기.

---

## 📝 새 치트 추가 — 단계별 가이드

예시: **"Infinite Stamina"** 만들기.

### Step 1. 오프셋 분석
**Cheat Engine** 같은 도구로 스태미나 값의 포인터 체인을 추적. 결과:
```
... → IndividualParameter + 0x420 = Stamina (int64_t)
```

### Step 2. `SDK/Offsets.h` 등록
```cpp
namespace IndividualParam {
    constexpr uintptr_t HP      = 0x400;
    constexpr uintptr_t Stamina = 0x420;   // ← 추가
}
```

### Step 3. `SDK/Pal.h` 선언
```cpp
namespace SDK {
    // 기존 함수들...
    int64_t GetLocalPlayerStamina();
    bool    SetLocalPlayerStamina(int64_t v);
}
```

### Step 4. `SDK/Pal.cpp` 구현
```cpp
int64_t SDK::GetLocalPlayerStamina() {
    auto p = GetIndividualParameter();      // 체인 헬퍼 재사용 — 새로 안 짜도 됨
    if (!p) return -1;
    return Scanner::ReadMemory<int64_t>(p + Offsets::IndividualParam::Stamina);
}

bool SDK::SetLocalPlayerStamina(int64_t v) {
    auto p = GetIndividualParameter();
    if (!p) return false;
    uintptr_t addr = p + Offsets::IndividualParam::Stamina;
    if (IsBadWritePtr((void*)addr, sizeof(int64_t))) return false;
    *(int64_t*)addr = v;
    return true;
}
```

### Step 5. `GUI/Menu.h` 플래그 추가
```cpp
struct Settings {
    bool bGodMode = false;
    bool bInfiniteStamina = false;   // ← 이미 존재할 수 있음
    // ...
};
```

### Step 6. `GUI/Menu.cpp` 체크박스 (기존에 이미 있다면 스킵)
```cpp
ImGui::Checkbox("Infinite Stamina", &Config.bInfiniteStamina);
```

### Step 7. 이 폴더에 신규 파일 — `InfiniteStamina.h` / `.cpp`

`InfiniteStamina.h`
```cpp
#pragma once
namespace InfiniteStamina { void Tick(); }
```

`InfiniteStamina.cpp`
```cpp
#include "InfiniteStamina.h"
#include "../../GUI/Menu.h"
#include "../../SDK/Pal.h"

namespace InfiniteStamina
{
    static constexpr int64_t kForcedStamina = 1000;

    void Tick()
    {
        if (Menu::Config.bInfiniteStamina) {
            SDK::SetLocalPlayerStamina(kForcedStamina);
        }
    }
}
```

### Step 8. `Core/HackMain.cpp` 와이어링
```cpp
// 상단 인클루드
#include "../Cheats/MemoryCheats/InfiniteStamina.h"

// MainLoop() 안:
GodMode::Tick();
InfiniteStamina::Tick();   // ← 추가
```

### Step 9. `Project/CMakeLists.txt` 등록
```cmake
# Track 1: 오프셋 기반 메모리 치트
Cheats/MemoryCheats/GodMode.cpp
Cheats/MemoryCheats/InfiniteStamina.cpp   # ← 추가
```

### Step 10. 빌드 + 테스트
1. VS 빌드 → `Pal_Injector.exe`로 인젝션
2. 콘솔 로그에 에러 없는지 확인
3. `INSERT`로 메뉴 열고 "Infinite Stamina" 토글
4. 인게임에서 달리기/공격으로 스태미나 닳는지 확인 (안 닳아야 정상)

---

## ⚠️ 자주 하는 실수

| 실수 | 증상 | 해결 |
|---|---|---|
| 체인 중간 null 체크 누락 | 게임 즉시 크래시 | `if (!ptr) return;` 한 줄씩 |
| 잘못된 타입 read (float vs int64) | 표시값이 garbage | Cheat Engine에서 타입 재확인 |
| 매 프레임 write 충돌 | 게임이 우리 값 덮어씀 | Track 2(함수 후킹)로 전환 |
| `bShowMenu` 검사로 묶음 | 메뉴 닫으면 풀림 | `Menu::Render` 가 아니라 `Cheats/...::Tick`에서만 적용 |
| 와이어링 빠뜨림 | 컴파일 OK인데 동작 X | HackMain `MainLoop` 에 `Tick()` 호출 추가했는지 확인 |

---

## 🧪 디버깅 팁

- `SDK::GetGWorld()` 부터 단계별로 호출해 0이 나오는 단계가 어디인지 확인 (체인의 어느 오프셋이 깨졌는지 빠르게 찾기)
- 콘솔에 `printf("HP addr = 0x%llx\n", addr)` 찍어서 매번 같은 영역인지 검증
- 게임 메인 화면 → 인게임 진입 후에야 GWorld가 초기화됨. 로딩 중 콘솔 에러는 정상

---

## 📦 현재 구현된 치트
- **`GodMode.cpp`** — 매 프레임 HP를 1,000,000,000으로 강제. `Menu::Config.bGodMode` 토글로 제어.
