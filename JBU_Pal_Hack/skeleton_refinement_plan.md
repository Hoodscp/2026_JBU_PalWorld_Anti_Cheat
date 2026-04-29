# 🔧 스켈레톤 다듬기 계획서 (Skeleton Refinement Plan)

본 문서는 현재 `Project/` 디렉토리의 스켈레톤을 **여러 개발자에게 분배하기 직전 단계**에서 발견된 문제점을 정리하고, 머지 충돌·런타임 크래시·디버깅 시간 낭비를 줄이기 위해 **선반영해 두는 게 좋은 수정 사항**을 우선순위별로 제안합니다.

> 모든 항목은 코드 검증 후 작성되었으며, 각 항목 끝에 `파일:라인`으로 위치를 명시해두었습니다.

---

## 🚨 P0 — 머지 직후 크래시/오작동을 만들 수 있는 항목 (반드시 수정)

### 1. `HackMain::Shutdown()` 이중 호출 위험
**위치**: `Core/HackMain.cpp:45-62`, `Core/dllmain.cpp:17-31`

`MainLoop()`가 `END` 키를 감지하면 `break` 후 자체적으로 `Shutdown()`을 호출하는데, 이후 DLL이 언로드될 때 `DLL_PROCESS_DETACH`에서도 `Shutdown()`이 다시 호출됩니다. 결과적으로 `Overlay::Shutdown()` → `kiero::shutdown()` → `ImGui::DestroyContext()`가 **두 번** 실행되어 ImGui/Kiero 내부에서 NULL 역참조 또는 이중 해제 크래시가 발생할 수 있습니다.

**수정 방향**:
- `Shutdown()` 진입부에 `static std::atomic<bool> alreadyShutdown{false}` 가드 추가
- `MainLoop` 종료 시에는 `Shutdown()`을 직접 호출하지 말고 `FreeLibraryAndExitThread`로 자연스러운 디테치 흐름을 타도록 변경

### 2. `Overlay::Shutdown()` 콘솔 종료 순서 오류
**위치**: `Core/HackMain.cpp:38-42`

```cpp
HWND consoleWnd = GetConsoleWindow();
FreeConsole();          // ← 콘솔 핸들이 무효화된 뒤
PostMessage(consoleWnd, WM_CLOSE, 0, 0); // ← invalid HWND로 메시지 보냄
```
순서를 `PostMessage` → `FreeConsole`로 바꾸거나 `PostMessage` 자체를 제거해야 합니다.

### 3. `mainRenderTargetView`, `pDevice`, `pContext` 메모리 누수
**위치**: `Overlay.cpp:60-77`, `Overlay.cpp:123-146`

후킹 초기화 시 `GetDevice`, `GetImmediateContext`, `CreateRenderTargetView`가 모두 내부적으로 `AddRef`를 호출하지만 `Shutdown()`에서 `Release()`를 호출하지 않아 D3D 객체가 새고 있습니다. 디테치 후 재주입 시 누적되는 형태라 게임이 점점 무거워집니다.

**수정**: `Shutdown()`에서 `mainRenderTargetView->Release()`, `pContext->Release()`, `pDevice->Release()` 추가하고 포인터를 `nullptr`로 설정.

### 4. `WndProc` 가로채기 + Kiero hkPresent 사이 레이스 컨디션
**위치**: `Overlay.cpp:51-94`, `Overlay.cpp:123-146`

`Shutdown()`에서 `kiero::shutdown()` 직후 `Sleep(200)`으로 렌더링 중인 프레임을 기다리지만 시간 기반은 보장이 약합니다. 만약 그 사이 `hkPresent`가 다시 진입하면 이미 해제된 ImGui 컨텍스트를 사용해 즉시 크래시.

**수정**:
- `init` 플래그를 `std::atomic<bool>`로 변경
- `hkPresent` 진입부에서 shutdown 플래그 체크 후 즉시 `oPresent` 패스스루
- `kiero::unbind(8)` → 짧은 대기 → ImGui shutdown 순으로 명시적 분리

### 5. `Menu::Render()`에 치트 적용 로직이 섞여 있음
**위치**: `GUI/Menu.cpp:43-48`

`bGodMode` 체크 시 `SDK::SetLocalPlayerHealth(1000000000)`를 메뉴 렌더 함수 내부에서 호출합니다. **메뉴를 닫으면(`bShowMenu = false`) 함수 자체가 조기 리턴(line 11-12)되어 GodMode가 꺼집니다.** 사용자 입장에서는 “메뉴 닫으면 신모드 풀림” 버그로 보임.

**수정**: 치트 적용 로직은 `HackMain::MainLoop()` 또는 별도 `Cheats::Tick()` 함수로 분리. GUI는 상태 표시·체크박스만 담당하도록 책임을 명확히 분리.

---

## ⚠️ P1 — 협업 시 큰 마찰을 만들 수 있는 항목

### 6. `Scanner::FindPattern`이 빈 깡통 (`return 0`)
**위치**: `Memory/Scanner.cpp:6-11`

README에는 “AOB 스캐닝으로 패치에도 깨지지 않는다”라고 적혀 있지만 실제 구현은 비어 있고, `SDK::GetIndividualParameter()`는 **하드코딩 오프셋 `0x090D3030`** 으로 작동합니다. 게임이 한 번만 패치되어도 곧바로 깨집니다.

**수정**:
- 표준적인 IDA-style AOB 스캐너 코드 채워넣기 (`?` 와일드카드 지원)
- 또는 “Team C가 채울 영역”임을 주석으로 명시하고 `#error "Implement AOB scanner"` 같은 가드를 임시로 두어 누구도 모르게 패치 깨진 채로 진행되지 않게 방지

### 7. `HookManager.cpp`가 존재하지 않음
**위치**: `Memory/HookManager.h` 단독 존재, `CMakeLists.txt:12`에 주석 처리됨

`Initialize / Shutdown / CreateHook` 선언만 있고 정의는 없습니다. CMake에도 `# Memory/HookManager.cpp` 로 주석. 다른 개발자가 후킹 매니저를 쓰려고 하면 즉시 링크 에러를 만나게 됩니다.

**수정**:
- `HookManager.cpp`에 MinHook 래퍼 최소 골격 작성 (`MH_Initialize`, `MH_CreateHook`, `MH_EnableHook`)
- `CMakeLists.txt`의 주석 해제

### 8. Visual Studio 버전 표기 불일치
**위치**: `README.md:23` (VS 2026) vs `implementation_plan.md:18` (VS 2022)

실제 팀이 어느 버전으로 통일할지 알 수 없습니다. 신규 입사자가 잘못된 SDK를 깔게 됩니다.

**수정**: 한 곳을 truth로 정하고 다른 문서를 동기화. (CMake 3.15는 두 버전 모두 지원하므로 어느 쪽이든 OK)

### 9. `Pal_Injector.exe` 가 저장소에 없음
**위치**: `README.md:69-72`

“`Pal_Injector.exe`를 관리자 권한으로 실행”이라고 안내하지만, 저장소에는 `Injector/` 폴더 자체가 없습니다.

**수정**: 두 가지 중 선택
- (A) 간단한 LoadLibrary 인젝터 소스를 `Injector/` 폴더로 추가 (CMake target 1개 더 추가)
- (B) “자체 인젝터는 별도 레포에서 관리” 명시 후 README 링크 정리

### 10. `setup_vendor.bat`이 fixed_lib 자동 적용을 안 함
**위치**: `Project/setup_vendor.bat`, `README.md:30-31`

README 단계 3에서 “kiero.cpp/h를 fixed_lib 파일로 직접 교체”하라고 수동 절차를 둡니다. 새 팀원이 이 단계를 빠뜨리는 순간 빌드는 되지만 후킹이 동작하지 않는 미묘한 버그가 발생합니다. 또한 `CMakeLists.txt:46-50`의 `if(EXISTS fixed_lib/kiero.cpp)` 분기는 우선 컴파일은 fixed_lib 쪽으로 가도록 잡혀 있지만, **헤더 include 경로가 `Vendor/kiero`도 포함**되어 있어 어느 쪽 헤더가 잡힐지 빌드 환경에 따라 모호합니다.

**수정**:
- `setup_vendor.bat` 마지막에 `copy /Y ..\fixed_lib\kiero.cpp ..\Vendor\kiero\kiero.cpp` 류의 자동 덮어쓰기 추가
- 또는 CMake에서 `Vendor/kiero` 인클루드를 빼고 `fixed_lib`만 노출하여 모호성 제거

### 11. `.gitignore`의 Vendor 화이트리스트가 가짜 안전망
**위치**: `Project/.gitignore:1-9`

`Vendor/*` 무시 + `Vendor/kiero/` 화이트리스트 구조인데, `setup_vendor.bat`이 git clone으로 받기 때문에 `Vendor/kiero/.git` 폴더가 들어와 **상위 git이 “submodule로 보이는 디렉토리”라고 경고**합니다. 결과적으로 푸시 누락 사고 가능성.

**수정**: `setup_vendor.bat`에 `rmdir /S /Q Vendor\kiero\.git` 등 받은 직후 내부 .git 삭제 절차 추가. 또는 정식 git submodule 방식으로 전환.

---

## 🧹 P2 — 코드 위생/일관성 (시간 되면 함께 정리)

### 12. ImGui ConfigFlags 덮어쓰기
**위치**: `Overlay.cpp:70`

```cpp
io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;  // ← 기존 플래그 전부 날림
```
`|=`로 변경.

### 13. ImGui ini 파일이 게임 디렉토리에 생성됨 (forensic 흔적)
**위치**: `Overlay.cpp:69` 부근

`io.IniFilename = nullptr;` 한 줄로 흔적 안 남기는 것이 보안 연구 컨텍스트에 부합.

### 14. `IsBadReadPtr` / `IsBadWritePtr` 사용
**위치**: `Memory/Scanner.h:18`, `SDK/Pal.cpp:51`

MS 공식 문서에서 사용 금지로 분류된 API입니다. 가드 페이지를 건드리며 `__try`/`__except`로 대체하거나 `VirtualQuery`로 검증해야 합니다.

### 15. `WndProc` 반환값에 `bool` 사용
**위치**: `Overlay.cpp:32, 44`

`LRESULT`는 정수 타입이므로 `return true;` → `return 1;` 또는 `return 0;`로 통일하는 편이 가독성·이식성 모두 양호.

### 16. `MainThread`의 반환값 컨벤션
**위치**: `Core/dllmain.cpp:13`

`DWORD` 함수에서 `return TRUE`는 컴파일러 경고는 없지만 의미상 `0`(성공) 또는 종료 코드여야 함.

### 17. `extern LRESULT ImGui_ImplWin32_WndProcHandler`
**위치**: `Overlay.cpp:11`

수동 extern은 ImGui 백엔드 헤더(`imgui_impl_win32.h`)가 이미 선언을 제공하므로 제거 가능. 향후 ImGui 업그레이드 시 시그니처 변경에 자동 대응.

### 18. CMake에서 x64 강제하지 않음
**위치**: `CMakeLists.txt`

Palworld는 64비트입니다. 32비트로 generate되는 사고를 막기 위해:
```cmake
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "x64 빌드만 지원합니다.")
endif()
```

### 19. 콘솔 UTF-8 코드페이지 미설정
**위치**: `Core/HackMain.cpp:13-15`

`SetConsoleOutputCP(CP_UTF8)` 한 줄 추가로 향후 한글 디버그 메시지도 깨짐 없이 표시.

### 20. 매직넘버 `kiero::bind(8, ...)`
**위치**: `Overlay.cpp:109`

D3D11 SwapChain Present의 vtable index 8을 `constexpr int kPresentVTableIndex = 8;` 같은 명명 상수로 추출하면 향후 D3D12 분기 시 추적이 쉬움.

### 21. 빈 `Pal_structs.h` / `Engine_structs.h`
**위치**: `SDK/`

스켈레톤이긴 해도 `// TODO(team-B): UE4SS dump from build X.X.X` 같은 명시적 TODO 주석을 두면 담당자가 손댈 곳이 명확해짐.

### 22. `.editorconfig` 부재
**위치**: 저장소 루트

여러 개발자가 들어오면 들여쓰기/줄바꿈 차이로 머지 충돌이 빈번해집니다. 4-space, LF, UTF-8을 강제하는 `.editorconfig` 한 개 권장.

### 23. CMake `CMakePresets.json` 부재
**위치**: 저장소 루트

VS 2022/2026에서 폴더 열기 시 누구나 동일한 빌드 디렉토리/툴체인으로 빠르게 시작할 수 있도록 preset 추가 권장.

---

## 📋 권장 작업 순서 (작업 분배 시점에 충돌 최소화)

1. **P0 5건 일괄 패치** → 한 PR로 묶어서 머지 (스켈레톤 안정화)
2. `HookManager.cpp` 골격 추가 + `Scanner::FindPattern` 구현 (Team C 영역) → **별도 PR**
3. 인젝터(`Injector/`) 추가 또는 외부화 결정 → **README 동기화 PR**
4. `setup_vendor.bat` 자동화 + .gitignore 정리 → **빌드/CI PR**
5. P2 위생 항목들은 느슨하게 정리 PR로 모으기

---

## ✅ Verification Plan

각 P0 항목 수정 후 다음을 확인:
1. `cmake -S Project -B Project/out` → 경고/에러 없이 generate
2. 빌드된 DLL을 게임에 주입 → `INSERT`로 메뉴 토글, `END`로 안전 디테치 (콘솔에 “Shutdown completed” 한 번만 출력되는지)
3. 메뉴 닫은 상태에서 GodMode 체크박스가 계속 작동하는지 확인 (P0-5 검증)
4. 디테치 후 재주입 시 D3D 누수 없이 정상 동작하는지 확인 (P0-3 검증)

---

## 🙋 User Review Required

> [!IMPORTANT]
> 위 항목 중 **승인하시는 P0/P1 범위**만 알려주시면, 곧바로 코드 수정에 착수하겠습니다.
>
> 추천: P0 전체 + P1의 #6, #7 (스켈레톤이라면 빈 함수가 가장 위험합니다).
