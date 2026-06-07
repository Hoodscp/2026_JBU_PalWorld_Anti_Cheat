// AntiDebugModule.h  (External / 외부 감시 버전)
// 안티디버깅 탐지 모듈.
//
// 무엇을 보나(대상 = 팰월드 프로세스, 외부에서 PID 로 검사):
//   - CheckRemoteDebuggerPresent
//   - NtQueryInformationProcess: ProcessDebugPort / ProcessDebugFlags /
//     ProcessDebugObjectHandle
//   - PEB.BeingDebugged / PEB.NtGlobalFlag(디버그 힙 비트)
//   - 스레드 디버그 레지스터(DR0~3/DR7) 하드웨어 브레이크포인트
//   - CPU 시간 정체(브레이크/일시정지 의심) — 상태 기반
//
// 기존 Module_Anti_Debugging(독립 실행형, PID 인자)의 검사 기법을 동일한
// IDetectionModule 규약으로 래핑해, 통합 러너 + 웹 대시보드에서 "안티디버깅"
// 탭으로 따로 볼 수 있게 한다.
//
// 한계(명시):
//   - 외부 검사라 자기 자신(감시기)에 붙은 디버거가 아니라 "대상 프로세스"의
//     디버깅 신호를 본다. 예외 기반 self-probe 는 인프로세스 전용이라 제외.
//   - INT3(0xCC) 소프트웨어 BP 스캔은 컴파일러 패딩 오탐이 많아 기본 비활성.
//     (코드 바이트 변조는 IntegrityScanner 가 디스크 대비로 더 정확히 잡음.)
#pragma once

#include "AcCommon.h"
#include "IDetectionModule.h"

#include <string>
#include <unordered_set>

namespace ac {

class AntiDebugModule : public IDetectionModule {
public:
    AntiDebugModule() = default;

    bool Initialize(DetectionEventQueue* queue) override;
    void Tick() override;
    void Shutdown() override;

    const char* Name() const override { return "AntiDebug"; }
    ModuleKind  Kind() const override { return ModuleKind::Polling; }
    uint32_t    PollIntervalMs() const override { return intervalMs_; }

    // 설정 API.
    void SetTarget(const TargetSpec& t)               { target_ = t; }
    void SetTargetImageName(const std::string& lower)  { target_.imageNameLower = lower; }
    void SetPollInterval(uint32_t ms)                  { intervalMs_ = ms; }
    void EnableHardwareBreakpointScan(bool on)         { hwbpScan_ = on; }
    void EnableTimingCheck(bool on)                    { timingCheck_ = on; }

private:
    void ResetSession(DWORD pid);
    void ReportOnce(const std::string& key, const std::string& type, Severity sev,
                    const std::string& desc, const std::string& evidence);

    void CheckNtQueries(HANDLE proc);
    void CheckPeb(HANDLE proc);
    void CheckHardwareBreakpoints(DWORD pid);
    void CheckTiming(HANDLE proc);

    DetectionEventQueue* queue_ = nullptr;
    TargetSpec           target_;

    DWORD currentPid_ = 0;
    std::unordered_set<std::string> reported_;  // 세션당 1회 보고(상태 신호 도배 방지)

    // 시간 기반 검사 상태.
    bool     hasPrevTiming_ = false;
    uint64_t prevProcMs_    = 0;
    uint64_t prevWallMs_    = 0;
    int      zeroCpuStreak_ = 0;

    uint32_t intervalMs_  = 3000;
    bool     hwbpScan_    = true;
    bool     timingCheck_ = true;
};

}  // namespace ac
