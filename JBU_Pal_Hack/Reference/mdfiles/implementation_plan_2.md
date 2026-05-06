# Kiero & ImGui 기반 DirectX 11 후킹 구현 계획서

현재 "텅 빈 화면"이 나오는 원인은, 지난 번 스켈레톤 구축 시 `Vendor` 폴더 내 서드파티 라이브러리(ImGui, MinHook 등)가 다운로드되지 않은 상태로 치트 DLL이 빌드되어 안전하게 껍데기만 주입되었기 때문입니다. 이 계획서는 실제 후킹 코드를 채워넣고 컴파일을 보장하여 게임 상에 완벽한 투명 GUI 레이어를 띄우는 과정입니다.

## User Review Required

> [!IMPORTANT]
> 아래의 구현 과정을 통해 언리얼 엔진 5(팰월드)의 DirectX 11 체인을 낚아채는(Hook) 핵심 코어 작업이 진행됩니다. 
> 이 작업은 코드가 조금 복잡하며, 타겟 프로세스의 입력(마우스, 키보드) 신호도 가로채야 합니다. 승인해 주시면 즉시 코드를 작성하겠습니다!

## Proposed Changes

### 1. `Vendor` 서드파티 다운로드 및 동기화 조치
사전에 스크립트로 제공했으나 누락되었던 **ImGui, MinHook, Kiero**를 제가 직접 콘솔 명령어를 통해 타겟 폴더(`Vendor/`) 내에 다운로드합니다. 이 라이브러리들은 DirectX 후킹에 무조건 필요한 산업 표준 에셋입니다.

### 2. CMakeLists.txt 재구성
 다운로드 된 외부 라이브러리 소스 코드(특히 `MinHook.c`, `kiero.cpp`, `imgui.cpp` 등)들이 한 덩어리로 안전하게 컴파일(Static 빌드)되어 `JBU_Pal_Hack.dll` 내부에 묶여 들어가도록 연동 브릿지를 수정합니다.

#### [MODIFY] `CMakeLists.txt`
- `MinHook` 라이브러리의 C 소스 코드 포함 로직 추가
- 명시적 D3D11 의존성 추가 링크

### 3. DirectX 11 기반 후킹 및 WndProc 입력 가로채기 적용
스켈레톤(껍데기)이던 C++ 클래스들에 생명을 불어넣어 게임 화면 픽셀의 맨 위에 우리만의 그림을 그리도록 세팅합니다.

#### [MODIFY] `GUI/Overlay.h` / `Overlay.cpp`
- **hkPresent**: Kiero를 통해 `DXGISwapChain::Present` 함수(화면이 모니터로 송출되기 직전의 프레임 함수)를 가로채어 ImGui를 렌더링하도록 작성
- **hkWndProc**: 게임 내 마우스 움직임과 클릭이 게임 시점을 돌리는 대신 "치트 메뉴"를 클릭할 수 있게끔, 윈도우 인터럽트 메시지를 가로챕니다.
- ImGui-DX11 및 ImGui-Win32 백엔드 초기화 로직 구현

#### [MODIFY] `GUI/Menu.cpp`
- 실제 크롬/게임 해킹 에셋에서 주로 사용하는 멋진 스타일의 반투명(Glassmorphism 스타일 혹은 다크 모드) ImGui 창 디자인 코드 활성화

## Open Questions

> [!NOTE]
> 1. 이번 업데이트를 진행할 때, 치트 메뉴를 켜고 끄기 위한 전용 단축키(예: `INSERT` 키)를 세팅해 드릴 텐데 괜찮으신가요?
> 2. 마음에 드신다면 **"계속 진행해 줘"** 형식으로 승인해주세요!

## Verification Plan
1. 제가 `Vendor` 다운로드 및 C++ 리팩토링을 완료한 후 빌드하여 주입해 보시라고 가이드해 드립니다.
2. `INSERT` 키를 누르면 화면에 "JBU Palworld Trainer" 창이 아름답게 나타나는지 테스트합니다.
