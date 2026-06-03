// main.cpp
// ProcessMonitorModule PoC 드라이버.
// 폴링 모듈을 PollIntervalMs 주기로 돌리는 최소 스케줄러 + 큐 소비 루프.
#include "ProcessMonitorModule.h"

#include <windows.h>

#include <cstdio>
#include <memory>
#include <vector>

using namespace ac;

static const char* SevName(Severity s) {
    switch (s) {
        case Severity::Info:     return "INFO";
        case Severity::Low:      return "LOW";
        case Severity::Medium:   return "MEDIUM";
        case Severity::High:     return "HIGH";
        case Severity::Critical: return "CRITICAL";
    }
    return "?";
}

int main() {
    DetectionEventQueue queue;

    auto procMon = std::make_unique<ProcessMonitorModule>();
    procMon->SetPollInterval(2000);
    procMon->EnableHashCheck(true);

    // 샘플 블랙리스트 — 이름은 반드시 소문자로 등록.
    // 실제 운영에선 팀 테스트 환경에서 측정한 값으로 갱신/확장.
    procMon->AddBlacklistEntry({ "Cheat Engine",   "cheatengine-x86_64.exe", "", Severity::Critical });
    procMon->AddBlacklistEntry({ "Cheat Engine",   "cheatengine-i386.exe",   "", Severity::Critical });
    procMon->AddBlacklistEntry({ "x64dbg",         "x64dbg.exe",             "", Severity::High });
    procMon->AddBlacklistEntry({ "x32dbg",         "x32dbg.exe",             "", Severity::High });
    procMon->AddBlacklistEntry({ "OllyDbg",        "ollydbg.exe",            "", Severity::High });
    procMon->AddBlacklistEntry({ "IDA (64)",       "ida64.exe",              "", Severity::Medium });
    procMon->AddBlacklistEntry({ "Process Hacker", "processhacker.exe",      "", Severity::Medium });
    procMon->AddBlacklistEntry({ "ReClass.NET",    "reclass.net.exe",        "", Severity::Medium });
    // 해시 매칭 예시 (리네임 우회 대응). 실제 SHA-256(대문자 hex)으로 교체:
    // procMon->AddBlacklistEntry({ "Renamed CE", "", "E3B0C44298FC1C14...", Severity::Critical });

    std::vector<IDetectionModule*> modules{ procMon.get() };

    for (auto* m : modules) {
        if (!m->Initialize(&queue)) {
            std::fprintf(stderr, "[init fail] %s\n", m->Name());
            return 1;
        }
    }

    std::printf("Anti-cheat ProcessMonitor PoC running. (Ctrl+C to stop)\n");

    // 최소 스케줄러: 폴링 모듈별 다음 실행 시각을 추적.
    // 이벤트 트리거 모듈(디버거/인젝션)은 별도 후킹 경로로 처리하므로 여기선 제외.
    struct Scheduled { IDetectionModule* mod; uint64_t nextDueMs; };
    std::vector<Scheduled> sched;
    for (auto* m : modules) {
        if (m->Kind() == ModuleKind::Polling) sched.push_back({ m, 0 });
    }

    for (;;) {
        const uint64_t now = NowMs();
        for (auto& s : sched) {
            if (now >= s.nextDueMs) {
                s.mod->Tick();
                s.nextDueMs = now + s.mod->PollIntervalMs();
            }
        }

        // 큐 소비 — 실제로는 별도 스레드 / 서버 REST 송신 레이어로 분리.
        DetectionEvent ev;
        while (queue.Pop(ev)) {
            std::printf("[%-8s] %-14s | %-22s | %s\n",
                        SevName(ev.severity), ev.moduleName.c_str(),
                        ev.detectionType.c_str(), ev.evidence.c_str());
        }

        ::Sleep(100); // 스케줄러 틱 해상도
    }

    return 0; // 도달하지 않음 (PoC)
}
