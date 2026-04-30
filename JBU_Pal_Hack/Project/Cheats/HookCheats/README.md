# HookCheats — AOB 시그니처 + 함수 후킹 치트 (Track 2)

게임 함수 자체를 가로채서 동작을 바꾸는 치트들.
메모리 폴링보다 안정적이고 안티치트에도 덜 노출됨.

## 워크플로우 한눈에

```
SDK/Signatures.h (AOB)
        ↓
Scanner::FindPattern   ←  "48 89 5C 24 ? 57 48 83 EC 20 ..."
        ↓ (게임 모듈에서 일치 위치 검색)
   함수 주소
        ↓
HookManager::CreateHook  →  detour 함수가 원본 자리 차지
```

## 새 후킹 치트 추가 절차

예시: **"DamageHook"** (TakeDamage 후킹으로 GodMode 강화)

### 1. 시그니처 등록 — `SDK/Signatures.h`
```cpp
namespace SDK::Signatures {
    inline constexpr const char* TakeDamage = "48 89 5C 24 ? 57 48 83 EC 20 ...";
}
```

### 2. 이 폴더에 신규 파일 — `DamageHook.h` / `.cpp`
`ExampleHook.cpp`를 복붙해서 시작 → detour 시그니처를 IDA 분석 결과에 맞춰 조정.

### 3. 와이어링 — `Core/HackMain.cpp`
```cpp
#include "../Cheats/HookCheats/DamageHook.h"   // 상단

// Initialize() 안, HookManager::Initialize() 다음:
DamageHook::Install();
```

### 4. 빌드 등록 — `Project/CMakeLists.txt`
```cmake
Cheats/HookCheats/ExampleHook.cpp
Cheats/HookCheats/DamageHook.cpp   # ← 추가
```

---

## 시그니처 추출 팁
- IDA / Ghidra / x64dbg에서 게임 함수 첫 10~20바이트 추출
- 변하기 쉬운 immediate operand(상수, 주소 오프셋)는 `?` 와일드카드 처리
- 패턴이 너무 짧으면 false positive 발생 → 보통 16바이트 이상 권장

## detour 작성 시 주의
- **호출 규약 일치**: x64는 보통 `__fastcall`, 멤버 함수면 첫 인자가 `this`
- **원본 호출 결정**: 완전 차단(return)이냐, 인자 수정 후 원본 호출이냐
- **재귀 방지**: 후킹 안에서 `oOriginal` 호출 (직접 함수 호출 X)

## 현재 구현된 후킹
- `ExampleHook.cpp` — TakeDamage 후킹 skeleton. `SDK::Signatures::TakeDamage`가 비어 있어 install 시 자동 skip. 시그니처 채우면 자동 활성화.
