# 🎮 Palworld Internal Cheat & Anti-Cheat Research Project

환영합니다! 이 프로젝트는 언리얼 엔진 5(Unreal Engine 5) 기반 게임인 **팰월드(Palworld)** 를 타겟으로 한 **Internal Memory Hack & GUI Overlay** 실습 및 보안 우회(Anti-Cheat) 연구를 위해 제작된 스켈레톤/프레임워크입니다.

우리는 게임 보안 기술을 연구하고 이해하기 위해 현업 수준의 메모리 변조 방식과 후킹 기법을 다룹니다.

---

## 🏛️ 아키텍처 개요

이 프로젝트는 빠르고 강력한 네이티브 접근을 위해 **C++** 을 핵심 언어로 사용하며, 세련된 게임 내 UI 송출을 위해 **Dear ImGui** 라이브러리를 DirectX 후킹과 결합하여 사용합니다. (C++ Internal + GUI Hybrid)

- **Internal 방식**: 게임 프로세스 내부에 우리의 코드를 담은 `.dll` 파일을 주입(Injection)하여 게임과 동일한 메모리 권한 및 포인터를 획득합니다.
- **Dear ImGui + Kiero**: 게임의 DirectX 11 `Present` 호출을 가로채 최상단에 반투명 메뉴 창을 그립니다.
- **MinHook**: 게임 내부 함수까지도 가로채(detour) 동작을 변조할 수 있습니다.

---

## 🛠️ 개발 환경 구축 가이드

새로운 팀원은 코드를 작성하기 전 다음 환경을 통일해야 합니다.

1. **필수 설치 사항**
   - **Visual Studio 2022 이상** (워크로드: `C++을 사용한 데스크톱 개발`)
   - **Git** (서드파티 모듈 다운로드용)

2. **초기 세팅 방법 (중요)**
   - 코드를 내려받은 직후, `Project` 폴더 안의 **`setup_vendor.bat`** 을 실행 → `Vendor/` 에 ImGui · MinHook · Kiero가 자동 다운로드됨.
   - **Kiero 패치 적용 (수동)**: `fixed_lib/kiero.cpp` 와 `kiero.h` 를 `Vendor/kiero/` 로 복사하여 덮어쓰기. (CMake가 우선 `fixed_lib` 쪽을 사용하지만, 인클루드 경로 안전성을 위해 동기화 권장)
   - Visual Studio에서 **"로컬 폴더 열기 (Open a local folder)"** 로 `Project` 폴더를 그대로 열면 CMake가 자동으로 빌드 환경을 잡습니다.

---

## 📂 디렉토리 (모듈) 역할 분담 구조

```
Project/
├── Core/                   ← DLL 진입점 + 메인 루프
│   ├── dllmain.cpp
│   └── HackMain.cpp/h
├── GUI/                    ← ImGui 오버레이 + 메뉴 디자인
│   ├── Overlay.cpp/h
│   └── Menu.cpp/h
├── Memory/                 ← 공용 인프라 (모든 치트가 사용)
│   ├── Scanner.cpp/h           AOB 패턴 스캐너
│   ├── HookManager.cpp/h       MinHook 래퍼 (CreateHook 한 줄로 후킹)
│   └── CursorHooks.cpp/h       시스템 후킹 (마우스 잠금 우회 등)
├── SDK/                    ← 게임 데이터 정의
│   ├── Offsets.h               포인터 체인용 오프셋 상수
│   ├── Signatures.h            함수 후킹용 AOB 시그니처
│   └── Pal.cpp/h               고수준 게임 객체 접근 API
├── Cheats/                 ← ⭐ 실제 치트 기능 (개발자가 작업하는 곳)
│   ├── MemoryCheats/           Track 1: 오프셋 기반 메모리 변조
│   │   ├── GodMode.cpp/h
│   │   └── README.md           ← Track 1 개발 가이드
│   └── HookCheats/             Track 2: AOB + 함수 후킹
│       ├── ExampleHook.cpp/h
│       └── README.md           ← Track 2 개발 가이드
├── fixed_lib/              ← Kiero 패치 (Vendor 덮어쓰기용)
└── Vendor/                 ← 서드파티 (setup_vendor.bat이 채움)
```

---

## 🚀 개발 워크플로우 — 어디서부터 작업하면 되나?

새 치트를 만들 때는 **두 가지 트랙 중 하나**를 고릅니다. 트랙은 *기술적 접근 방식*의 차이일 뿐 둘 다 동일한 메뉴 / SDK 인프라를 공유합니다.

### Track 1 — 오프셋 기반 메모리 변조 (`Cheats/MemoryCheats/`)
**언제?** HP, 스태미나, 탄약, 골드처럼 평문 값으로 메모리에 저장된 데이터를 직접 read/write 하고 싶을 때. **단순하고 빠르게 시작 가능**.

**워크플로우 한눈에**:
```
SDK/Offsets.h 에 오프셋 등록
  → SDK/Pal.cpp 에 getter/setter 추가
  → GUI/Menu.h 에 체크박스 플래그 추가
  → Cheats/MemoryCheats/<MyCheat>.cpp 작성 (Tick 함수)
  → Core/HackMain.cpp::MainLoop 에 한 줄 추가
  → CMakeLists.txt 에 한 줄 추가
```
👉 **상세 가이드**: [`Project/Cheats/MemoryCheats/README.md`](Project/Cheats/MemoryCheats/README.md)

### Track 2 — AOB 시그니처 + 함수 후킹 (`Cheats/HookCheats/`)
**언제?** 데미지 계산, 자원 소모 같은 **로직 자체**를 변경하고 싶을 때. 메모리 폴링보다 안정적이고 안티치트에 덜 노출.

**워크플로우 한눈에**:
```
IDA/Ghidra로 게임 함수 분석 → AOB 패턴 추출
  → SDK/Signatures.h 에 시그니처 등록
  → Cheats/HookCheats/<MyHook>.cpp 작성 (detour + Install)
  → Core/HackMain.cpp::Initialize 에 Install() 호출 한 줄 추가
  → CMakeLists.txt 에 한 줄 추가
```
👉 **상세 가이드**: [`Project/Cheats/HookCheats/README.md`](Project/Cheats/HookCheats/README.md)

### 두 트랙은 자유롭게 섞어 써도 됨
하나의 치트 토글(`bGodMode`)이 **메모리 폴링(Track 1)** 과 **함수 후킹(Track 2)** 양쪽에서 동시에 작동해도 무방. 실제로 `GodMode`는 현재 Track 1로 동작 중이고, `ExampleHook`은 같은 토글을 Track 2로 강화하는 skeleton.

---

## 💉 빌드 후 DLL 주입 및 실행

1. **팰월드 실행**: 스팀 시작 옵션에 `-d3d11` 추가 후 게임 시작 (DirectX 11 모드 강제).
2. **DLL 주입**: `Injector/` 폴더에서 빌드한 `Pal_Injector.exe`를 관리자 권한으로 실행하면 `JBU_Pal_Hack.dll` 을 자동 매핑.
3. **인게임 메뉴**: 콘솔 로그에 `[+] Kiero Initialized!` 가 뜨면 `INSERT` 키로 메뉴 토글.
4. **종료**: `END` 키로 안전 디테치. 재주입 가능.

---

## 🎯 개발 진행 현황

- ✅ ImGui + DirectX 11 후킹 (Kiero 기반 universal hook)
- ✅ C++ LoadLibrary 인젝터 (`Injector/`)
- ✅ 메모리/SDK/GUI 모듈 분리 + 실시간 HP 포인터 추적
- ✅ AOB 스캐너 실제 구현 (IDA-style 패턴)
- ✅ MinHook 래퍼 (`HookManager`) + 시스템 후킹 (커서/raw input)
- ✅ Cheats 디렉토리 분리 (Track 1 / Track 2 구조)
- 🚀 (진행중) Phase 2: 실제 게임 함수 시그니처 분석 + DamageHook · AmmoHook 등 채우기

즐거운 보안 연구 되시길! 🚀
