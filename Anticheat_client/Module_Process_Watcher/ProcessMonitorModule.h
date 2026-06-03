// ProcessMonitorModule.h
// 외부 프로세스 감시 모듈 (블랙리스트: 이미지명 + SHA-256 해시).
//
// 역할: 인젝션 탐지 모듈이 "게임 프로세스 내부에 로드된 DLL"을 본다면,
//       본 모듈은 "시스템에서 실행 중인 외부 프로세스"를 본다.
//       Cheat Engine / x64dbg 같은 external R/W·디버깅 도구를 포착하는 보완재.
//
// 한계 (의도적으로 명시):
//   - 이름 매칭은 실행 파일 리네임으로 즉시 우회됨.
//   - 해시 매칭은 재컴파일/단일 바이트 패치로 우회됨.
//   => 단독 방어선이 아니라, 서버 측 교차 검증으로 보내는 "약한 신호" 중 하나.
//      (10주차: 다중 검사 + 검사 지점 분산 + 서버 교차 검증 원칙)
#pragma once

#include "IDetectionModule.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ac {

// 블랙리스트 항목. imageName 또는 sha256 중 채워진 항목으로 매칭.
struct BlacklistEntry {
    std::string label;                 // 사람이 읽는 이름 (예: "Cheat Engine")
    std::string imageName;             // 소문자 실행 파일명 (예: "x64dbg.exe"). 비어있으면 이름 매칭 안 함
    std::string sha256;                // 대문자 16진 SHA-256. 비어있으면 해시 매칭 안 함
    Severity    severity = Severity::High;
};

class ProcessMonitorModule : public IDetectionModule {
public:
    ProcessMonitorModule();

    bool Initialize(DetectionEventQueue* queue) override;
    void Tick() override;
    void Shutdown() override;

    const char* Name() const override { return "ProcessMonitor"; }
    ModuleKind  Kind() const override { return ModuleKind::Polling; }
    uint32_t    PollIntervalMs() const override { return intervalMs_; }

    // 설정 API.
    void SetPollInterval(uint32_t ms)        { intervalMs_ = ms; }
    void AddBlacklistEntry(BlacklistEntry e) { blacklist_.push_back(std::move(e)); }
    void EnableHashCheck(bool on)            { hashCheck_ = on; }

private:
    struct ProcInfo {
        uint32_t     pid = 0;
        std::wstring fullPath;       // 해시 계산용 전체 경로 (없을 수 있음)
        std::string  imageNameLower; // 소문자 base 이름 (utf-8)
    };

    bool EnumerateProcesses(std::vector<ProcInfo>& out);
    bool ComputeFileSha256(const std::wstring& path, std::string& outHex);
    void Report(const BlacklistEntry& e, const ProcInfo& p, const char* matchType);

    DetectionEventQueue* queue_ = nullptr;
    std::vector<BlacklistEntry> blacklist_;

    // "pid:label" 단위 중복 보고 방지. (PID 재사용 가능성은 PoC 한계)
    std::unordered_set<std::string> reported_;
    // 동일 바이너리 재해싱 방지: fullPath -> sha256 hex.
    std::unordered_map<std::wstring, std::string> hashCache_;

    uint32_t intervalMs_ = 2000;
    bool     hashCheck_  = true;
};

} // namespace ac
