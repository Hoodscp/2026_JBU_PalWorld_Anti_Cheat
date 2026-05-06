# 팰월드(Unreal Engine) Internal + ImGui 트레이너 스켈레톤 구축 계획

본 문서에서는 여러 개발자가 함께 협업하여 팰월드 언리얼 엔진 기반의 **C++ Internal DLL + Dear ImGui** 게임 핵/트레이너를 개발하기 위한 환경 구축 가이드 및 스켈레톤 아키텍처를 제안합니다.

## User Review Required

> [!IMPORTANT]
> 스켈레톤 및 구조물 자동 생성을 진행하기 전에, 아래 제안하는 디렉토리 구조와 사용하려는 서드파티 라이브러리(ImGui, MinHook 등)가 마음에 드시는지 확인해 주세요. 승인해 주시면 해당 구조로 `source` 폴더 내에 실제 C++ 초기 코드(스켈레톤)와 Visual Studio 프로젝트 구성을 시작하겠습니다.

---

## 1. 개발 환경 구축 가이드 (팀원 공통 설정)

팀 내 모든 개발자들은 다음 개발 환경 기준을 통일해야 충돌 없이 협업할 수 있습니다.

### 필수 설치 항목
- **IDE**: Visual Studio 2022 (Community 버전 이상)
  - 설치 시 **"C++을 사용한 데스크톱 개발"** 워크로드를 반드시 체크하여 최신 Windows 11 SDK 및 MSVC v143 빌드 도구를 포함합니다.
- **버전 관리**: Git

### 프로젝트 환경설정 (미리 구성할 내용)
- **빌드 타겟**: `x64` (팰월드 프로세스가 64비트이므로 무조건 64비트 빌드)
- **문자 집합 (Character Set)**: "멀티바이트 문자 집합 사용(Use Multi-Byte Character Set)"
- **C++ 표준**: ISO C++ 17 또는 20

---

## 2. 디렉토리 구조 (스켈레톤 설계)

협업 시 충돌(Merge Conflict)을 방지하고 각자의 역할을 모듈화하기 위해 아래와 같이 `source/` 내부 구조를 분할합니다.

```text
/source/
 ├── JBU_Pal_Hack.sln           # Visual Studio 솔루션 파일
 ├── JBU_Pal_Hack/              # 메인 프로젝트 폴더
 │   ├── Core/                  # DLL 시작 지점 및 메인 쓰레드 관리
 │   │   ├── dllmain.cpp        # 1. 프로세스 삽입 시 최초 진입점
 │   │   └── HackMain.cpp/.h    # 2. 핵심 루프 및 후킹 초기화
 │   │
 │   ├── GUI/                   # ImGui 그래픽 인터페이스 영역
 │   │   ├── Menu.cpp/.h        # 사용자에게 보여질 치트 활성화/비활성화 등
 │   │   └── Overlay.cpp/.h     # DirectX/ImGui 렌더링 초기화
 │   │
 │   ├── Memory/                # 메모리 변조, 후킹 및 AOB 탐색
 │   │   ├── Scanner.cpp/.h     # 패턴 스캐닝(AOB) 유틸리티 함수
 │   │   └── HookManager.h      # MinHook 등을 래핑한 후킹 모듈
 │   │
 │   ├── SDK/                   # 언리얼 엔진 구조체 (UE4SS 파싱 데이터 등)
 │   │   ├── Engine_structs.h   # 나중에 UWorld 등 엔진 관련 헤더를 넣을 자리
 │   │   └── Pal_structs.h
 │   │
 │   └── Vendor/                # 외부 라이브러리 소스
 │       ├── ImGui/             # Dear ImGui 소스 코드 (기본+DirectX 백엔드)
 │       ├── MinHook/           # 함수 후킹용 라이브러리
 │       └── kiero/             # (선택) DirectX 11/12 Universal Hooking
 │
 └── build/                     # 컴파일 시 .dll 파일이 떨어지는 결과물 폴더
```

---

## 3. 뼈대(Skeleton) 코드의 역할 분담

단일 파일에 모든 코드를 넣으면 팀원 간 수정 시 엉킬 확률이 큽니다. 구조별 역할은 다음과 같습니다.

* **팀원 A (UI/UX 담당)**: `GUI/Menu.cpp`를 주로 작업합니다. ImGui를 사용해 프리미엄 디자인, 색상, 애니메이션, 치트 On/Off 체크박스 등을 배치합니다.
* **팀원 B (메모리 부분 담당)**: 팰월드를 리버싱하여 `SDK/` 항목에 구조체를 채워 넣고, 치트 기능에 필요한 포인터 기반 데이터를 찾습니다.
* **팀원 C (핵심 기능/스캐너 담당)**: `Memory/` 및 `Core/` 단에서 무한체력, 데미지 조작, 아이템 생성 등을 처리하는 실제 훅(Hook) 함수나 바이트 덮어쓰기 로직을 작성하고 AOB 스캐너를 최적화합니다.

---

## 4. Open Questions

1. 대상 게임(팰월드)이 **DirectX 11**과 **DirectX 12** 기반 중 어떤 모드를 타겟으로 할지 결정되셨나요? (최근 팰월드는 DX12를 지원/권장합니다. Kiero 라이브러리를 추가하면 버전에 상관없이 쉽게 훅 작성이 가능합니다.)
2. 위의 제안된 디렉토리 구조와 환경이 마음에 드신다면, 실제 Visual Studio `sln` 솔루션 파일과 빈 스켈레톤 파일들, 그리고 `ImGui`, `kiero`, `MinHook` 라이브러리를 다운로드하여 `source/` 폴더 내에 배치하는 작업을 이어서 바로 실행해 드릴까요?

---

## 5. Verification Plan

1. **자동 생성**: CMake CLI 스크립트 도구 혹은 직접 `.sln`, `.vcxproj` 파일을 작성하여 Visual Studio 2022에서 바로 빌드할 수 있는 초기 뼈대를 구성합니다.
2. **검증**: 생성된 코드들이 정상적으로 VS2022에서 열리고 의존성이 깨진 곳 없이 빌드 준비 상태가 되었는지 확인합니다.
