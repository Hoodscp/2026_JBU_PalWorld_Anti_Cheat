# 🎮 Palworld Internal Cheat & Anti-Cheat Research Project

환영합니다! 이 프로젝트는 언리얼 엔진 5(Unreal Engine 5) 기반 게임인 **팰월드(Palworld)** 를 타겟으로 한 **Internal Memory Hack & GUI Overlay** 실습 및 보안 우회(Anti-Cheat) 연구를 위해 제작된 스켈레톤/프레임워크입니다.

우리는 게임 보안 기술을 연구하고 이해하기 위해 현업 수준의 메모리 변조 방식과 후킹 기법을 다룹니다.

---

## 🏛️ 아키텍처 개요

이 프로젝트는 빠르고 강력한 네이티브 접근을 위해 **C++** 을 핵심 언어로 사용하며, 세련된 게임 내 UI 송출을 위해 **Dear ImGui** 라이브러리를 DirectX 후킹과 결합하여 사용합니다. (C++ Internal + GUI Hybrid)

* **Internal 방식**: `ReadProcessMemory`/`WriteProcessMemory`에 의존하는 외부(External) 트레이너와 달리, 게임 프로세스 내부에 우리의 코드를 담은 `.dll` 파일을 주입(Injection)하여 게임과 동일한 메모리 권한 및 포인터를 획득합니다.
* **Dear ImGui**: 게임 화면을 가로채어 최상단에 투명도와 멋진 애니메이션을 지원하는 메뉴 창을 그립니다.

---

## 🛠️ 개발 환경 구축 가이드

새로운 팀원은 코드를 작성하기 전 다음 환경을 통일해야 합니다.

1. **필수 설치 사항**
   * **Visual Studio 2022** (필수 항목: `C++을 사용한 데스크톱 개발` 워크로드 설치)
   * **Git** (서드파티 모듈 다운로드용)

2. **초기 세팅 방법 (중요)**
   * 코드를 내려받은 직후, 가장 먼저 `Project` 폴더 안에 있는 **`setup_vendor.bat`** 스크립트를 더블클릭하여 실행합니다. 
   * 위 과정을 거치면 `Vendor` 폴더 내에 `ImGui`, `MinHook` 등 필수 외부 라이브러리 소스들이 자동으로 다운로드됩니다.
   * 그 후 Visual Studio 2022를 켜고 **"로컬 폴더 열기(Open a local folder)"** 를 통해 `Project` 폴더 자체를 엽니다. (솔루션 파일이 필요 없이 CMakeLists 스크립트에 의해 자동으로 빌드 환경이 세팅됩니다.)

---

## 📂 디렉토리 (모듈) 역할 분담 구조

코드 충돌(Merge Conflict)을 방지하고 각자의 장점을 살리기 위해, `Project` 내부 폴더를 다음과 같이 분할하였습니다.

### 1. `Core/` (메인 루프 및 초기화)
* `dllmain.cpp` : 인젝터에 의해 DLL이 최초 주입되었을 때 실행되는 시작점입니다.
* `HackMain.cpp` : 콘솔창 할당, 스레드 초기화, 핵 종료 등의 전반적인 사이클을 관리합니다.

### 2. `GUI/` (디자인/렌더링 담당 영역 🎨)
* `Overlay.cpp` : DirectX 함수(Present)를 찾아내어 가로채고(Hook), ImGui 캔버스를 준비합니다.
* `Menu.cpp` : 실제 화면에 그려질 핵 메뉴(체크박스, 컬러, 버튼) 등을 디자인하고 배치합니다.

### 3. `Memory/` (해킹 및 코어 로직 담당 영역 🧠)
* `Scanner.cpp` : AOB (Array of Bytes) 패턴 스캐닝을 이용해 게임이 업데이트되어도 깨지지 않는 오프셋 주소를 찾아냅니다. 
* `HookManager.h` : 타겟 시스템 또는 엔진 내부 함수를 `MinHook` 등을 활용해 낚아채서 우리가 원하는 동작으로 바꿉니다.

### 4. `SDK/` (언리얼 언어 매핑 담당 영역 🗺️)
* `Engine_structs.h` / `Pal_structs.h` : 언리얼 엔진 Dumper(예: UE4SS)를 이용해 뽑아낸 C++ 기반 게임 내 구조체(클래스) 설계도를 담는 공간입니다. 코어 로직 담당자가 이 헤더 포인터를 사용하여 게임 포지션이나 체력에 직관적으로 접근합니다.

### 5. `Vendor/` (서드파티 의존성)
* `ImGui`, `kiero`, `MinHook`과 같이 외부에서 가져온 검증된 해킹/렌더링 라이브러리들입니다. (`setup_vendor.bat`으로 설치 후 건드릴 필요 없음)

---

## 💉 빌드된 DLL 파일 주입(Injection) 및 실행 방법

프로젝트를 빌드하여 `JBU_Pal_Hack.dll` 파일이 성공적으로 생성되었다면, 이를 팰월드 게임 내에 삽입(Inject)해야 작동합니다.

### 1. 인젝터(Injector) 준비
우리의 코드는 Internal 방식이므로, 게임 프로세스 메모리 공간에 `.dll`을 강제로 밀어넣어 줄 "인젝터(Injector)" 프로그램이 필요합니다. 
* **학습 목적 (직접 개발)**: `OpenProcess` -> `VirtualAllocEx` -> `WriteProcessMemory` -> `CreateRemoteThread` (보편적인 LoadLibrary 인젝션 기법) API를 이용해 C# 또는 C++로 콘솔 기반의 간단한 인젝터를 직접 만들어보는 것을 적극 권장합니다.
* **테스트 목적 (상용 툴 사용)**: `Extreme Injector`, `Xenos Injector`, `Cheat Engine`, `Process Hacker` 등의 툴을 사용하여 곧바로 주입 테스트를 진행할 수도 있습니다.

### 2. 실행 순서
1. **팰월드(Palworld) 게임 실행**: 게임을 로비 화면이나 인게임까지 진입시킵니다.
2. **DLL 주입(Injection)**: 인젝터를 관리자 권한으로 실행한 뒤, 타겟 프로세스를 `Palworld-Win64-Shipping.exe`로 잡고, 우리가 빌드한 `JBU_Pal_Hack.dll`을 선택하여 주입(Inject)합니다.
3. **확인**: 코드 내에 포함된 `AllocConsole()`로 인해 검은색 명령 프롬프트(디버그 콘솔) 창이 함께 뜨면서 `[+] JBU Palworld Internal Hack Loaded`라는 문구가 나타난다면 주입에 완벽히 성공한 것입니다!
4. **종료**: 게임을 끄거나, 스켈레톤 코드에 지정된 종료 단축키(`END` 키)를 누르면 후킹이 풀리며 안전하게 핵이 종료됩니다.

---

## 🎯 우리의 첫 번째 목표
1. `ImGui` 및 DirectX 훅 연동을 성공하여 게임 내에 텅 빈 UI 메뉴 창 띄우기
2. `Memory/Scanner.cpp`를 활용하여 팰월드의 로컬 플레이어 엔티티(Entity)나 언리얼 `uWorld` 오브젝트 주소 알아내기
3. `SDK` 구조체를 통해 플레이어의 스태미너 주소를 따서 깎이지 않게 락(Lock) 걸어두기!
4. 콘솔 기반의 초소형 `자체 제작 DLL 인젝터` 파생 프로젝트 만들기!

즐거운 파괴(?) 지향 보안 연구 되시길 바랍니다! 🚀
