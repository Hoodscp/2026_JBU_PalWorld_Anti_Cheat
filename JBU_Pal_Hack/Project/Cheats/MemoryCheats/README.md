# MemoryCheats — 오프셋 기반 치트 (Track 1)

게임 메모리에 직접 read/write로 동작하는 치트들이 모이는 곳.
포인터 체인을 따라가서 값을 덮어쓰는 단순/안정 방식.

## 새 치트 추가 절차

예시: **"Infinite Stamina"** 추가

### 1. 오프셋 등록 — `SDK/Offsets.h`
```cpp
namespace IndividualParam {
    constexpr uintptr_t HP = 0x400;
    constexpr uintptr_t Stamina = 0x???;  // ← 추가
}
```

### 2. SDK getter/setter 추가 — `SDK/Pal.h` & `SDK/Pal.cpp`
```cpp
// Pal.h
int64_t GetLocalPlayerStamina();
bool    SetLocalPlayerStamina(int64_t v);

// Pal.cpp — GetIndividualParameter()를 재사용
int64_t SDK::GetLocalPlayerStamina() {
    auto p = GetIndividualParameter();
    return p ? Scanner::ReadMemory<int64_t>(p + Offsets::IndividualParam::Stamina) : -1;
}
```

### 3. 메뉴 플래그 + 체크박스 — `GUI/Menu.h` & `GUI/Menu.cpp`
```cpp
// Menu.h Settings에:
bool bInfiniteStamina = false;

// Menu.cpp:
ImGui::Checkbox("Infinite Stamina", &Config.bInfiniteStamina);
```

### 4. 이 폴더에 신규 파일 — `InfiniteStamina.h` / `.cpp`
```cpp
// InfiniteStamina.h
#pragma once
namespace InfiniteStamina { void Tick(); }

// InfiniteStamina.cpp
#include "InfiniteStamina.h"
#include "../../GUI/Menu.h"
#include "../../SDK/Pal.h"

namespace InfiniteStamina {
    void Tick() {
        if (Menu::Config.bInfiniteStamina) {
            SDK::SetLocalPlayerStamina(1000);
        }
    }
}
```

### 5. 와이어링 — `Core/HackMain.cpp`
```cpp
#include "../Cheats/MemoryCheats/InfiniteStamina.h"   // 상단

// MainLoop():
GodMode::Tick();
InfiniteStamina::Tick();   // ← 추가
```

### 6. 빌드 등록 — `Project/CMakeLists.txt`
```cmake
Cheats/MemoryCheats/GodMode.cpp
Cheats/MemoryCheats/InfiniteStamina.cpp   # ← 추가
```

---

## 현재 구현된 치트
- `GodMode.cpp` — 플레이어 HP 상시 1,000,000,000으로 덮어쓰기
