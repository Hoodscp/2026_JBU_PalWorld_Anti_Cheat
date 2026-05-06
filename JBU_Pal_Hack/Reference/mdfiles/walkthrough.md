# 팰월드 게임 핵 제작용 협업 스켈레톤 구축 완료

요청하신 `Project` 폴더 내부에 C++ 기반 Internal + 모듈형 구조 뼈대 코드들을 성공적으로 생성 완료했습니다.

## 📁 구성된 핵심 구조
```text
/Project
 ├── CMakeLists.txt      # Visual Studio 빌드 및 솔루션(.sln) 자동 생성용
 ├── setup_vendor.bat    # 서드파티 라이브러리(ImGui, MinHook 등) 한 번에 다운로드 받는 스크립트
 ├── Core/
 │   ├── dllmain.cpp     # DLL 진입점 및 스레드 할당
 │   └── HackMain.cpp/.h # 치트 루프 및 초기화/해제 담당
 ├── GUI/
 │   ├── Menu.cpp/.h     # ImGui 체크박스, 화면 렌더링 UI 제작
 │   └── Overlay.cpp/.h  # DirectX 훅 연동 및 ImGui 프레임 적용
 ├── Memory/
 │   ├── Scanner.cpp/.h  # 메모리 패턴스캔 로직 기본 뼈대
 │   └── HookManager.h   # 대상 함수 바이패스/후킹
 └── SDK/
     └── Engine_structs.h / Pal_structs.h  # 언리얼 분석 구조체 저장소
```

## 🚀 앞으로의 진행 방식 (팀원 공유용)

> [!TIP]
> **프로젝트 여는 방법 (Visual Studio 2022)**
> 1. `Project/setup_vendor.bat` 스크립트를 더블클릭해서 외부 라이브러리(ImGui, MinHook, kiero)를 다운로드 받습니다.
> 2. Visual Studio 2022를 실행하고 `파일 -> 열기 -> 폴더(Folder)`를 선택하여 `Project` 폴더를 엽니다.
> 3. VS2022가 백그라운드에서 `CMakeLists.txt`를 읽고 자동으로 C++ 환경을 구성합니다. (상단 재생 버튼에서 대상을 선택 후 빌드하면 됨)

### 1단계: 빌드 환경 통일
먼저 `setup_vendor.bat`을 실행해 보세요. 에러 없이 `Vendor` 폴더 안에 서드파티 깃허브 레포지토리가 받아져야 합니다.

### 2단계: 핵심 후킹
DirectX 프레젠트 함수(Present)에 접근해 `ImGui`를 게임 위에 그림으로 덮어 씌우는 것을 1차 목표로 잡으세요. 코드는 `GUI/Overlay.cpp`에 뼈대가 있으며 kiero를 활용하면 매우 쉽게 적용 가능합니다.

### 3단계: 역할 분담 진행
A는 UI 꾸미기에 집중하고, B는 체력이나 스태미너 주소를 찾고, C는 `Memory/Scanner.cpp`에 가져온 주소나 AOB 값을 활용하여 변조하는 구조로 확장해 나가시면 됩니다. 

모든 기초 세팅은 다 갖추어졌습니다. 어느 부분의 세부 기능이나 코드가 필요하다면 언제든 다시 질문 남겨주세요!
