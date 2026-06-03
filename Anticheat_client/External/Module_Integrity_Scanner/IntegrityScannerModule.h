// IntegrityScannerModule.h  (External / 외부 감시 버전)
// 코드 무결성 검사 모듈.
//
// 무엇을 보나:
//   라이브(실행 중) 프로세스의 코드 섹션(.text 등 실행 가능 섹션)을
//   "디스크상의 원본 PE"와 바이트 단위로 비교한다. ASLR 베이스 재배치로
//   정상적으로 달라지는 위치는 재배치 테이블을 보고 건너뛴다.
//   차이 구간은 모아서(coalesce) NOP 패치 / 인라인 후크 / 일반 코드 변조로
//   분류해 신고한다.
//
// 어떤 치트를 노리나:
//   JBU_Pal_Hack 의 Track 2(AOB 스캐닝 + NOP 패치로 명령어 비활성화)와
//   MidHook(34바이트 트램펄린)은 게임 .text 바이트를 직접 고친다(README).
//   바로 그 변조를 디스크 원본 대비로 잡는 것이 목표.
//   기본 검사 대상은 게임 메인 exe. AddModule 로 시스템 DLL 도 추가 가능.
//
// 한계(의도적 명시):
//   - vtable(.rdata 포인터) 후크처럼 코드 바이트가 아닌 포인터를 바꾸는
//     변조는 .text 비교로 잡히지 않는다(재배치로 가려짐).
//   - 매 틱 .text 전체를 ReadProcessMemory 하므로 대상 모듈이 크면 비용이 큼
//     => 폴링 주기를 넉넉히(기본 8s) 둔다.
//   - 게임이 셀프 패치/언팩하면 오탐 가능(언팩커가 없는 Shipping 빌드는 안정적).
#pragma once

#include "AcCommon.h"
#include "IDetectionModule.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace ac {

class IntegrityScannerModule : public IDetectionModule {
public:
    IntegrityScannerModule() = default;

    bool Initialize(DetectionEventQueue* queue) override;
    void Tick() override;
    void Shutdown() override;

    const char* Name() const override { return "IntegrityScanner"; }
    ModuleKind  Kind() const override { return ModuleKind::Polling; }
    uint32_t    PollIntervalMs() const override { return intervalMs_; }

    // 설정 API.
    void SetTarget(const TargetSpec& t)               { target_ = t; }
    void SetTargetImageName(const std::string& lower)  { target_.imageNameLower = lower; }
    void SetPollInterval(uint32_t ms)                  { intervalMs_ = ms; }
    // 메인 exe 외에 추가로 무결성 검사할 모듈(소문자 이름).
    void AddModule(const std::string& moduleLower)     { extraModules_.push_back(moduleLower); }
    // 모듈당 신고할 변조 구간 최대 개수(나머지는 요약).
    void SetMaxRegionsPerModule(uint32_t n)            { maxRegions_ = n; }

private:
    void ResetSession(DWORD pid);
    bool WantModule(const std::string& nameLower) const;
    void ScanModule(HANDLE proc, const RemoteModule& mod);
    void Report(const std::string& type, Severity sev,
                const std::string& desc, const std::string& evidence);

    DetectionEventQueue* queue_ = nullptr;
    TargetSpec           target_;

    DWORD currentPid_ = 0;
    std::vector<std::string>        extraModules_;
    std::unordered_set<std::string> reported_;

    uint32_t intervalMs_ = 8000;
    uint32_t maxRegions_ = 32;
};

}  // namespace ac
