# Module Anti Debugging

PID로 지정한 프로세스가 디버깅 중인지 3초마다 감시하는 Windows 실습용 C++ 프로그램입니다.

## Build

```powershell
cmake -S . -B build
cmake --build build --config Release
```

## Run

```powershell
.\build\Release\anti_debug_monitor.exe <PID>
```

예시:

```powershell
.\build\Release\anti_debug_monitor.exe 1234
```

관리자 권한이 없거나 보호 프로세스/권한이 높은 프로세스를 대상으로 하면 일부 검사가 실패할 수 있습니다. 이 경우 프로그램은 실패한 검사명을 출력하고 가능한 검사 결과만 계속 표시합니다.

## Detection Signals

- `CheckRemoteDebuggerPresent`
- `NtQueryInformationProcess(ProcessDebugPort)`
- `NtQueryInformationProcess(ProcessDebugFlags)`
- `NtQueryInformationProcess(ProcessDebugObjectHandle)`
- 원격 PEB의 `BeingDebugged` 플래그 읽기
- 원격 PEB의 `NtGlobalFlag` 디버그 힙 플래그 읽기
- Time-Based: 대상 프로세스 CPU 시간이 연속으로 정체되는지 확인
- 중단점 탐지: 실행 가능 메모리의 `INT3(0xCC)` 바이트 스캔
- 중단점 탐지: 대상 스레드의 하드웨어 디버그 레지스터 `DR0..DR3/DR7` 확인
- 예외 활용: `DBG_CONTROL_C`, `DBG_PRINTEXCEPTION_C` 기반 감시 프로그램 self-probe

## Notes

- `INT3(0xCC)`는 컴파일러 패딩에도 사용될 수 있어 오탐 가능성이 있습니다.
- Time-Based 검사는 대상 프로세스가 정상적으로 idle 상태여도 의심 신호가 나올 수 있습니다.
- 외부 PID 감시 방식에서는 대상 프로세스 내부 예외 흐름을 직접 검사하기 어렵습니다. 정확한 예외 기반 안티 디버깅은 이후 DLL/인프로세스 모듈 형태로 확장하는 것이 좋습니다.
