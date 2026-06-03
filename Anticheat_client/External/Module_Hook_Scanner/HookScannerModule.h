// HookScannerModule.h  (External / 외부 감시 버전)
// API 후킹 탐지 모듈.
//
// 무엇을 보나:
//   1) 익스포트 인라인 후크 — ntdll/kernel32/user32 등 핵심 시스템 DLL 의 자주
//      후킹되는 함수들의 시작(프롤로그) 바이트를 "디스크 원본"과 비교한다.
//      MinHook / Detours 류는 함수 첫머리에 jmp(트램펄린)를 심으므로,
//      라이브 바이트가 점프 스텁(ClassifyJumpStub)으로 분류되면 강한 증거.
//   2) IAT(Import Address Table) 후크 — 게임 메인 모듈의 임포트 주소 테이블을
//      걸어, 어떤 임포트의 실제 주소가 "로드된 어떤 모듈에도 속하지 않는"
//      메모리(사적 할당 트램펄린 등)를 가리키면 IAT 리다이렉션으로 본다.
//
// 어떤 치트를 노리나:
//   JBU_Pal_Hack 은 MinHook 래퍼(HookManager)로 인라인 후크를 건다(README).
//   시스템 API 를 후킹하는 경우는 (1) 이, 게임이 부르는 임포트를 가로채는
//   경우는 (2) 가 잡는 것이 목표. (게임 함수 본문의 인라인 패치는
//   IntegrityScanner 가 .text 비교로 잡는다 — 역할 분담.)
//
// 한계(의도적 명시):
//   - DX11 Present 후킹(Kiero)처럼 vtable(.rdata 포인터)을 바꾸는 후크는
//     익스포트/ .text 비교로는 잡히지 않는다(인프로세스 모듈 필요).
//   - forward 익스포트/ApiSet 으로 다른 물리 DLL 로 해석되는 경우를 오탐하지
//     않도록, IAT 검사는 "어느 모듈에도 안 속하는 포인터"만 신고한다.
//   => 단독 방어선이 아니라 서버 교차 검증으로 보내는 신호 중 하나.
#pragma once

#include "AcCommon.h"
#include "IDetectionModule.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace ac {

class HookScannerModule : public IDetectionModule {
public:
    HookScannerModule() = default;

    bool Initialize(DetectionEventQueue* queue) override;
    void Tick() override;
    void Shutdown() override;

    const char* Name() const override { return "HookScanner"; }
    ModuleKind  Kind() const override { return ModuleKind::Polling; }
    uint32_t    PollIntervalMs() const override { return intervalMs_; }

    // 설정 API.
    void SetTarget(const TargetSpec& t)               { target_ = t; }
    void SetTargetImageName(const std::string& lower)  { target_.imageNameLower = lower; }
    void SetPollInterval(uint32_t ms)                  { intervalMs_ = ms; }
    void EnableExportHookScan(bool on)                 { exportScan_ = on; }
    void EnableIatHookScan(bool on)                    { iatScan_ = on; }
    // true 면 등록 모듈의 "모든" 익스포트를 검사. 기본은 핫 워치리스트만(성능).
    void EnableScanAllExports(bool on)                 { scanAllExports_ = on; }

    // 검사할 시스템 모듈(소문자 이름)과 핫 함수 목록을 등록.
    // hotFuncs 가 비어있고 scanAllExports_=false 면 해당 모듈은 검사에서 제외된다.
    void AddWatchedModule(const std::string& moduleLower,
                          std::vector<std::string> hotFuncs = {});

private:
    struct WatchModule {
        std::string              nameLower;
        std::vector<std::string> hotFuncs;  // 검사 대상 함수명(소문자 매칭 아님; 정확 일치)
    };

    void InstallDefaultWatchlist();
    void ResetSession(DWORD pid);

    void ScanExportHooks(HANDLE proc, const std::vector<RemoteModule>& mods);
    void ScanModuleExports(HANDLE proc, const std::vector<RemoteModule>& mods,
                           const RemoteModule& mod, const WatchModule& wm);
    void ScanIatHooks(HANDLE proc, const std::vector<RemoteModule>& mods);

    void Report(const std::string& type, Severity sev,
                const std::string& desc, const std::string& evidence);

    DetectionEventQueue* queue_ = nullptr;
    TargetSpec           target_;

    DWORD currentPid_ = 0;
    std::vector<WatchModule>        watch_;
    std::unordered_set<std::string> reported_;  // 중복 보고 방지 키

    uint32_t intervalMs_     = 5000;
    bool     exportScan_     = true;
    bool     iatScan_        = true;
    bool     scanAllExports_ = false;
};

}  // namespace ac
