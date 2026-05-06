# 트레이너 구조 분석 (Trainer Structure Analysis)

## 개요
이 바이너리는 팰월드(Palworld)용 **FLiNG 트레이너**로 식별되는 64비트 Windows 실행 파일입니다. 이 파일은 실제 치트 로직이 실행될 환경을 준비하는 로더(Loader) 역할을 수행합니다.

## 주요 구성 요소

### 1. 네이티브 로더 (Native Loader)
메인 실행 파일은 C++로 작성되었으며 다음 작업을 처리합니다:
- **프로세스 부착**: `Palworld-Win64-Shipping.exe`를 감지하고 프로세스 핸들을 엽니다.
- **리소스 관리**: 트레이너의 핵심 로직과 GUI에 사용되는 임베디드 리소스를 포함합니다.
- **API 분석**: 단순한 정적 분석을 피하기 위해 실행 중에 `GetProcAddress`를 사용하여 Windows API(`WriteProcessMemory`, `OpenProcess` 등)를 동적으로 찾아냅니다.

### 2. .NET 런타임 호스팅
트레이너는 GUI 및 상위 레벨 로직을 실행하기 위해 .NET 런타임을 내부적으로 호스팅합니다:
- **인터페이스**: `mscoree.dll`의 `ICorRuntimeHost` 및 `ICLRMetaHost` 인터페이스를 사용합니다.
- **초기화**: `CLRCreateInstance` 및 `GetRuntime`(버전 `v4.0.30319`)을 호출합니다.
- **실행**: `ExecuteInDefaultAppDomain`을 사용하여 .NET 어셈블리를 시작합니다.

### 3. 리소스 추출
- **리소스 이름**: `REMOTE` (ID: 250).
- **대상 폴더**: 사용자 임시 디렉토리 내의 `FLiNGTrainerTemp`.
- **메커니즘**: `REMOTE` 리소스를 **무작위 파일명**(예: "TrSpeedHack"에서 유도됨)을 가진 DLL로 추출하고 자신의 프로세스 공간으로 로드합니다.
- **권한 설정**: 추출된 DLL이 정상적으로 로드될 수 있도록 파일 보안 권한(`S-1-15-2-1` / `ALL_APP_PACKAGES`)을 명시적으로 설정합니다.

### 4. 치트 엔진 스크립트 엔진
- 바이너리 내부에는 **치트 엔진(CE) 형식**의 수많은 스크립트가 포함되어 있습니다.
- 이 스크립트들은 `[ENABLE]` 및 `[DISABLE]` 섹션을 포함하며, 코드 위치를 찾기 위한 `aobscanmodule`과 코드 케이브 생성을 위한 `alloc` 명령을 사용합니다.
- 이는 트레이너 내부에 치트 엔진과 호환되는 스크립트 엔진 또는 라이브러리가 내장되어 있음을 시사합니다.
