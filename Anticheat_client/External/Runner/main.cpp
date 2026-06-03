// main.cpp  (External 통합 러너)
// 외부 감시(External) 탐지 모듈들을 하나의 이벤트 큐 + 미니 스케줄러로 통합 실행한다.
//
// 통합 대상(IDetectionModule):
//   - ProcessMonitor   : 시스템의 블랙리스트 외부 프로세스(CE/x64dbg 등) 감시  [기존]
//   - InjectionScanner : 게임 프로세스 내 DLL 인젝션/수동 매핑 탐지            [기존]
//   - HookScanner      : 시스템 API 익스포트 인라인 후크 + 메인 모듈 IAT 후크   [신규]
//   - IntegrityScanner : 게임 .text(코드) vs 디스크 원본 무결성 비교            [신규]
//
// 모든 모듈은 동일한 DetectionEventQueue 로 결과를 보내고, 소비 루프가 콘솔에
// 출력한다(실 운영에선 서버 REST 송신 레이어로 교체).
//
// 설계 근거(IDetectionModule.h): 폴링 모듈은 스케줄러가 PollIntervalMs 주기로
// Tick() 한다. 본 러너의 4개 모듈은 모두 Polling 종류다.
#include "IDetectionModule.h"
#include "ProcessMonitorModule.h"
#include "InjectionScannerModule.h"
#include "HookScannerModule.h"
#include "IntegrityScannerModule.h"

#include <windows.h>

#include <cstdio>
#include <memory>
#include <vector>

using namespace ac;

namespace {

// 팰월드 UE5 Shipping 프로세스명(소문자). 환경에 맞게 변경 가능.
const char* kPalworldImage = "palworld-win64-shipping.exe";

volatile bool g_running = true;

BOOL WINAPI ConsoleHandler(DWORD type) {
    if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT || type == CTRL_CLOSE_EVENT) {
        g_running = false;
        return TRUE;
    }
    return FALSE;
}

const char* SevName(Severity s) {
    switch (s) {
        case Severity::Info:     return "INFO";
        case Severity::Low:      return "LOW";
        case Severity::Medium:   return "MEDIUM";
        case Severity::High:     return "HIGH";
        case Severity::Critical: return "CRITICAL";
    }
    return "?";
}

}  // namespace

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    DetectionEventQueue queue;

    // ── 1) 프로세스 감시 (기존) ──────────────────────────────────────────────
    auto procMon = std::make_unique<ProcessMonitorModule>();
    procMon->SetPollInterval(2000);
    procMon->EnableHashCheck(true);
    procMon->AddBlacklistEntry({ "Cheat Engine", "cheatengine-x86_64.exe", "", Severity::Critical });
    procMon->AddBlacklistEntry({ "Cheat Engine", "cheatengine-i386.exe",   "", Severity::Critical });
    procMon->AddBlacklistEntry({ "x64dbg",       "x64dbg.exe",             "", Severity::High });
    procMon->AddBlacklistEntry({ "x32dbg",       "x32dbg.exe",             "", Severity::High });
    procMon->AddBlacklistEntry({ "OllyDbg",      "ollydbg.exe",            "", Severity::High });
    procMon->AddBlacklistEntry({ "IDA (64)",     "ida64.exe",              "", Severity::Medium });
    procMon->AddBlacklistEntry({ "Process Hacker","processhacker.exe",     "", Severity::Medium });
    procMon->AddBlacklistEntry({ "ReClass.NET",  "reclass.net.exe",        "", Severity::Medium });

    // ── 2) 인젝션 탐지 (기존) ────────────────────────────────────────────────
    auto injMon = std::make_unique<InjectionScannerModule>();
    injMon->SetTargetImageName(kPalworldImage);
    injMon->SetPollInterval(4000);
    injMon->EnableSignatureCheck(true);
    injMon->EnableManualMapScan(true);

    // ── 3) 후킹 탐지 (신규) ──────────────────────────────────────────────────
    auto hookMon = std::make_unique<HookScannerModule>();
    hookMon->SetTargetImageName(kPalworldImage);
    hookMon->SetPollInterval(5000);
    hookMon->EnableExportHookScan(true);
    hookMon->EnableIatHookScan(true);
    // 전체 익스포트 스캔은 비용↑/소음↑ — 기본 핫 워치리스트만 사용.
    hookMon->EnableScanAllExports(false);

    // ── 4) 무결성 검사 (신규) ────────────────────────────────────────────────
    auto integrityMon = std::make_unique<IntegrityScannerModule>();
    integrityMon->SetTargetImageName(kPalworldImage);
    integrityMon->SetPollInterval(8000);
    integrityMon->SetMaxRegionsPerModule(32);
    // 게임 exe 외에 검증하고 싶은 모듈이 있으면 추가(예: 엔진 DLL).
    // integrityMon->AddModule("someengine.dll");

    std::vector<IDetectionModule*> modules{
        procMon.get(), injMon.get(), hookMon.get(), integrityMon.get(),
    };

    for (auto* m : modules) {
        if (!m->Initialize(&queue)) {
            std::fprintf(stderr, "[init fail] %s\n", m->Name());
            return 1;
        }
    }

    std::printf("Anti-cheat External Suite running.\n");
    std::printf("  target : %s\n", kPalworldImage);
    std::printf("  modules: ProcessMonitor / InjectionScanner / HookScanner / IntegrityScanner\n");
    std::printf("  (Ctrl+C to stop)\n\n");

    // 미니 스케줄러: 폴링 모듈별 다음 실행 시각 추적.
    struct Scheduled { IDetectionModule* mod; uint64_t nextDueMs; };
    std::vector<Scheduled> sched;
    for (auto* m : modules) {
        if (m->Kind() == ModuleKind::Polling) sched.push_back({ m, 0 });
    }

    while (g_running) {
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
            std::printf("[%-8s] %-16s | %-26s | %s\n",
                        SevName(ev.severity), ev.moduleName.c_str(),
                        ev.detectionType.c_str(), ev.evidence.c_str());
        }

        ::Sleep(100);  // 스케줄러 틱 해상도
    }

    for (auto* m : modules) m->Shutdown();
    std::printf("\nStopped.\n");
    return 0;
}
