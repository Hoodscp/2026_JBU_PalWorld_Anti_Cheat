// InjectionScannerModule.h  (External / 외부 감시 버전)
// DLL 인젝션 탐지 모듈.
//
// 무엇을 보나:
//   1) 모듈 목록 이상 — 게임/시스템 디렉터리 밖에서 로드된 DLL, 서명되지 않은 DLL,
//      기준선(baseline) 이후 새로 나타난 DLL.
//   2) 수동 매핑(Manual Map / Reflective) 인젝션 — 로더 목록에 없는데 실행 가능한
//      MEM_PRIVATE 메모리 영역, 그 안의 'MZ' PE 헤더.
//
// 어떤 치트를 노리나:
//   JBU_Pal_Hack 은 DLL 인젝션 방식(README 기준)이다. LoadLibrary 인젝션은 (1)에서,
//   탐지 회피용 수동 매핑은 (2)에서 잡는 것이 목표.
//
// 한계(의도적 명시):
//   - 외부 RPM 방식은 인젝터가 PsSetLoadImageNotify 보다 먼저 동작하면 타이밍상
//     놓칠 수 있다. 또 게임 자체 JIT/스크립트가 private exec 메모리를 만들면 오탐.
//   => 서버 교차 검증으로 보내는 신호 중 하나. (인프로세스 버전이 더 견고)
#pragma once

#include "AcCommon.h"
#include "IDetectionModule.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace ac {

class InjectionScannerModule : public IDetectionModule {
public:
    InjectionScannerModule() = default;

    bool Initialize(DetectionEventQueue* queue) override;
    void Tick() override;
    void Shutdown() override;

    const char* Name() const override { return "InjectionScanner"; }
    ModuleKind  Kind() const override { return ModuleKind::Polling; }
    uint32_t    PollIntervalMs() const override { return intervalMs_; }

    // 설정 API.
    void SetTarget(const TargetSpec& t)        { target_ = t; }
    void SetTargetImageName(const std::string& lower) { target_.imageNameLower = lower; }
    void SetPollInterval(uint32_t ms)          { intervalMs_ = ms; }
    void EnableSignatureCheck(bool on)         { signatureCheck_ = on; }
    void EnableManualMapScan(bool on)          { manualMapScan_ = on; }
    // 추가 신뢰 디렉터리(소문자 경로 접두사). 안티치트 자신/런타임 등.
    void AddTrustedDir(const std::wstring& dirLower) { trustedDirs_.push_back(dirLower); }

private:
    void OnTargetChanged(DWORD pid, HANDLE proc, const std::vector<RemoteModule>& mods);
    void ScanModules(HANDLE proc, const std::vector<RemoteModule>& mods);
    void ScanManualMap(HANDLE proc, const std::vector<RemoteModule>& mods);

    bool IsTrustedPath(const std::wstring& pathLower) const;
    void Report(const std::string& type, Severity sev,
                const std::string& desc, const std::string& evidence);

    DetectionEventQueue* queue_ = nullptr;
    TargetSpec target_;

    DWORD currentPid_ = 0;
    std::wstring gameDirLower_;                 // 메인 exe 가 있는 디렉터리
    std::vector<std::wstring> trustedDirs_;     // 추가 신뢰 접두사
    std::unordered_set<uintptr_t> baselineBases_;  // 기준선 모듈 베이스 집합
    std::unordered_set<std::string> reported_;     // 중복 보고 방지 키

    uint32_t intervalMs_   = 4000;
    bool signatureCheck_   = true;
    bool manualMapScan_    = true;
};

}  // namespace ac
