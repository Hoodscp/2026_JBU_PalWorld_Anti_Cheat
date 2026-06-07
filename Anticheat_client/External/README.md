# External — 외부 감시(Out-of-Process) 탐지 모듈 모음

게임 프로세스 **밖**에서 PID 로 팰월드 프로세스를 열어(`OpenProcess` +
`ReadProcessMemory`) 메모리/모듈을 들여다보는 탐지 모듈들입니다.
모든 모듈은 공통 인터페이스 [`IDetectionModule`](../Module_Process_Watcher/IDetectionModule.h)
를 구현하고, 공용 헬퍼 [`AcCommon.h`](../Common/AcCommon.h) 를 공유하며,
결과를 단일 `DetectionEventQueue` 로 보냅니다.

> 학술/연구용 PoC. 외부 감시는 인젝터가 먼저 동작하면 일부를 놓칠 수 있어
> "단독 방어선"이 아니라 서버 교차 검증으로 보내는 신호 중 하나로 설계되었습니다.

---

## 모듈 구성

| 모듈 | 디렉터리 | 종류 | 무엇을 보나 | 노리는 치트 |
|------|----------|------|-------------|-------------|
| **ProcessMonitor** | `../Module_Process_Watcher` | Polling | 시스템에서 실행 중인 블랙리스트 외부 도구(CE/x64dbg/IDA 등) — 이름·SHA-256 | external R/W·디버깅 도구 |
| **InjectionScanner** | `Module_Injection_Scanner` | Polling | 게임 프로세스에 로드된 비신뢰/미서명/신규 DLL, 수동 매핑(reflective) 영역 | DLL 인젝션·수동 매핑 |
| **HookScanner** | `Module_Hook_Scanner` | Polling | 시스템 API 익스포트 인라인 후크 + 메인 모듈 IAT 후크 | MinHook/Detours 인라인·IAT 후크 |
| **IntegrityScanner** | `Module_Integrity_Scanner` | Polling | 라이브 `.text`(코드) vs 디스크 원본 PE 비교(ASLR 보정) | Track 2 NOP 패치·MidHook 트램펄린 |
| **AntiDebug** | `Module_Anti_Debug` | Polling | 대상 프로세스 디버거 탐지(PEB/NtQuery/하드웨어 BP/타이밍) | 디버거 부착·동적 분석 |

ProcessMonitor·InjectionScanner 는 기존 모듈, HookScanner·IntegrityScanner·
AntiDebug 는 신규 추가 모듈입니다. 다섯 모듈은 [`Runner/main.cpp`](Runner/main.cpp)
의 단일 스케줄러/큐로 **통합** 실행되며, 결과는 **콘솔 로그**와 **웹 대시보드**
양쪽으로 동시에 출력됩니다.

---

## 웹 대시보드 (Web UI)

실행 시 루프백 전용 임베디드 HTTP 서버가 함께 기동합니다(외부 의존성 없음, Winsock).

```
http://127.0.0.1:8787
```

- 모듈별 **탭**으로 따로 볼 수 있습니다: `전체 / 안티디버깅 / 인젝션 / 후킹 감지 /
  프로세스 감시 / 무결성` (이벤트가 발생한 모듈만 탭 자동 생성).
- 1초 주기 증분 폴링(`GET /api/events?since=N`)으로 새 이벤트만 받아 append.
- 심각도 색상 배지, type/evidence 검색, 화면 비우기, 자동 최신 스크롤 지원.
- **콘솔 로그는 그대로 유지**됩니다. 대시보드는 같은 `DetectionEvent` 를 별도
  링버퍼(최근 5,000건)로 보관해 보여주는 추가 출력 경로입니다.
- 서버 기동에 실패해도(포트 사용 중 등) 콘솔 로그는 정상 동작합니다.

엔드포인트:

| 경로 | 응답 |
|------|------|
| `GET /` | 단일 페이지 대시보드(HTML) |
| `GET /api/events?since=N` | `{"lastId":N,"events":[...]}` (id>N 인 이벤트만) |

포트 변경은 `Runner/main.cpp` 의 `kDashboardPort` 에서. 인증이 없으므로
루프백(127.0.0.1) 바인드를 유지하세요.

---

## HookScanner — 후킹 탐지 상세

1. **익스포트 인라인 후크** — `ntdll`/`kernelbase`/`kernel32`/`user32` 의 자주
   후킹되는 함수(워치리스트) 프롤로그 16바이트를 디스크 원본과 비교합니다.
   - 라이브 첫 명령이 점프 스텁(`E9/EB/FF25/push-ret/mov-jmp`)으로 분류되면
     → `Hook.InlineExport` (Critical). 가능하면 점프 목적지 모듈까지 해석하며,
     목적지가 **로드된 모듈 밖(사적 메모리)** 이면 트램펄린의 강한 증거입니다.
   - 점프는 아니지만 (재배치 아닌) 바이트가 다르면 → `Hook.ProloguePatch` (High).
   - ASLR 베이스 재배치가 적용되는 바이트는 정상 차이로 보고 건너뜁니다.
2. **IAT 후크** — 메인 모듈의 Import Address Table 엔트리가 **어느 로드 모듈에도
   속하지 않는 메모리**를 가리키면 → `Hook.IAT` (Critical). forward 익스포트/ApiSet
   으로 다른 시스템 DLL 을 가리키는 정상 케이스는 오탐하지 않습니다.

**한계**: DX11 Present(Kiero) 같은 **vtable 후크**는 `.rdata` 포인터를 바꾸므로
익스포트/`.text` 비교로는 잡히지 않습니다(인프로세스 모듈 필요).

설정: `EnableExportHookScan` / `EnableIatHookScan` / `EnableScanAllExports`
(기본은 핫 워치리스트만 — 성능), `AddWatchedModule(name, {funcs...})`.

---

## IntegrityScanner — 무결성 검사 상세

대상 모듈(기본: 게임 메인 exe)의 **실행 가능 섹션**(`.text` 등)을 디스크 원본 PE 와
바이트 단위로 비교합니다.

- ASLR 델타(`live_base - preferred_base`)가 0이 아니면 베이스 재배치 테이블을
  읽어, 재배치가 적용되는 바이트는 정상 차이로 건너뜁니다.
- 차이 바이트는 인접 구간으로 병합 후 분류해 `Integrity.CodePatch` 로 신고:
  - 전부 `0x90` → **NOP 패치(명령어 비활성화)** — Track 2 의 전형
  - 점프 스텁 → **인라인 후크/트램펄린** (Critical) — MidHook 등
  - 그 외 → **코드 변조** (High)
- 섹션 대부분이 다르면(`Integrity.MassMismatch`) 패킹/언팩 또는 손상 가능으로 요약.

설정: `AddModule(name)` 로 시스템 DLL 추가 검증, `SetMaxRegionsPerModule(n)`,
`SetPollInterval(ms)` (기본 8s — `.text` 전체 RPM 비용 때문에 넉넉히).

**한계**: vtable(.rdata 포인터) 후크는 재배치로 가려져 잡히지 않습니다. 대상
모듈이 크면 매 틱 비용이 큽니다(주기로 완화).

---

## AntiDebug — 안티디버깅 상세

기존 독립 실행형 `Module_Anti_Debugging` 의 검사 기법을 `IDetectionModule` 규약으로
래핑해, 통합 러너/대시보드에서 "안티디버깅" 탭으로 따로 볼 수 있게 한 모듈입니다.
대상 = 팰월드 프로세스(외부에서 PID 로 검사).

- `CheckRemoteDebuggerPresent` → `AntiDebug.RemoteDebugger` (Critical)
- `NtQueryInformationProcess`:
  - `ProcessDebugPort` ≠ 0 → `AntiDebug.DebugPort` (Critical)
  - `ProcessDebugObjectHandle` 존재 → `AntiDebug.DebugObject` (Critical)
  - `ProcessDebugFlags` = 0 → `AntiDebug.DebugFlags` (High)
- `PEB.BeingDebugged` → `AntiDebug.PebBeingDebugged` (Critical)
- `PEB.NtGlobalFlag` 디버그 힙 비트 → `AntiDebug.NtGlobalFlag` (High)
- 스레드 디버그 레지스터(DR0~3/DR7) → `AntiDebug.HardwareBreakpoint` (High)
- CPU 시간 정체(연속 2틱) → `AntiDebug.ExecutionStalled` (Medium, idle 오탐 가능)

신호는 세션당 1회만 보고(상태 도배 방지), 게임 재시작 시 초기화됩니다.

설정: `SetPollInterval`(기본 3s) / `EnableHardwareBreakpointScan` /
`EnableTimingCheck`.

**한계**: 외부 검사라 "대상 프로세스"의 디버깅 신호를 봅니다. 예외 기반
self-probe 는 인프로세스 전용이라 제외했고, INT3(0xCC) 소프트웨어 BP 스캔은
컴파일러 패딩 오탐이 많아 IntegrityScanner(디스크 대비)로 대체했습니다.

---

## 빌드

```powershell
cmake -S . -B out/build/x64-Debug -G "Visual Studio 17 2022" -A x64
cmake --build out/build/x64-Debug --config Debug
```

산출물: `out/build/x64-Debug/Debug/ac_external_monitor.exe`

- **플랫폼**: Windows 10/11 x64 (대상 = UE5 x64 Shipping)
- **컴파일러**: MSVC (VS 2022), C++17, `/W4 /utf-8 /permissive-`
- **링크**: `wintrust`(서명 검증), `bcrypt`(SHA-256), `ws2_32`(웹 대시보드)

## 실행

```powershell
.\out\build\x64-Debug\Debug\ac_external_monitor.exe
```

실행하면:
- **콘솔**에 `[SEVERITY] Module | DetectionType | evidence` 형식으로 출력(기존 유지).
- **웹 대시보드**가 `http://127.0.0.1:8787` 에서 함께 뜸 → 브라우저로 모듈별 탭 확인.

대상 프로세스명은 `Runner/main.cpp` 의 `kPalworldImage`
(`palworld-win64-shipping.exe`), 대시보드 포트는 `kDashboardPort`(8787)에서
변경합니다. 실 운영에서는 큐 소비 루프를 서버 REST 송신 레이어로 교체합니다.

> 충분한 권한 필요: 보호된 게임 프로세스를 열려면 관리자 권한으로 실행하세요.
