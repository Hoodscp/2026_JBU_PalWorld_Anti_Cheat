#pragma once
#include <atomic>
#include <cstdarg>
#include <string>

// ────────────────────────────────────────────────────────────────────
// Logger — 크래시 디버깅용 파일 로그.
//
// 설계 의도:
//   - 게임이 튕기면 AllocConsole 콘솔은 닫혀 마지막 cout 이 사라진다.
//     → 같은 내용을 %TEMP%\JBU_Pal_Hack.log 에 매 줄 즉시 flush.
//   - std::cout 의 rdbuf 를 Tee 버퍼로 갈아끼워, 기존 모든 후크의
//     `std::cout << ...` 가 콘솔 + 파일에 동시 출력. 후크 코드 수정 0.
//   - "현재 단계" 를 atomic<const char*> 로 추적 — 평시엔 메모리 store
//     1회만(거의 0 비용), 크래시 시 핸들러가 마지막 단계명을 적는다.
//
// 사용:
//   Initialize  : Logger::Init()        — DLL_PROCESS_ATTACH 직후 1회.
//   일반 로그   : JBU_LOG("foo=%d", x)  — printf 형식.
//   단계 표시   : JBU_PHASE("Install:Temperature");
//                 이후 크래시 핸들러가 이 문자열을 "where we were" 로 기록.
//   현재 단계   : Logger::CurrentPhase()  — CrashHandler 가 호출.
//   종료        : Logger::Shutdown()    — DLL_PROCESS_DETACH 직전.
// ────────────────────────────────────────────────────────────────────

namespace Logger {

    // 파일 로그 + std::cout tee 시작. 멱등.
    // 동일 PID 에서 두 번 인젝션 시 기존 로그를 .prev 로 회전한 뒤 새로 시작.
    bool Init();

    // 정상 종료 마커 후 파일 닫기. 멱등.
    void Shutdown();

    // 파일에 한 줄 (system local time + 줄 끝 flush) 기록.
    // 콘솔에는 print 하지 않음 — 콘솔 출력은 std::cout (tee 됨) 으로.
    void WriteLineRaw(const char* level, const char* msg);

    // printf 형식 한 줄. 콘솔(std::cout)에도 동시 출력.
    void Log(const char* level, const char* fmt, ...);

    // 현재 단계 갱신 + 같은 메시지를 로그에 기록.
    // literal 문자열 포인터를 보관(수명 = 프로그램). 동적 문자열 쓰지 말 것.
    void SetPhase(const char* phase);

    // 현재 단계 갱신만(파일 쓰기 X). MainLoop 같은 hot path 에서 매 프레임
    // 단계 추적용 — 평시 비용은 atomic store 1회뿐. 크래시 핸들러가 마지막
    // 단계명을 보고할 때 같은 변수에서 읽으므로 충분.
    void SetPhaseQuiet(const char* phase);

    const char* CurrentPhase();

    // 로그 파일 절대 경로 (디버그/메시지박스용).
    const std::string& FilePath();

}  // namespace Logger

// ── 매크로 ─────────────────────────────────────────────────────────
// printf-형식 가변인자. 콘솔 + 파일 모두로 한 줄.
#define JBU_LOG(...)        ::Logger::Log("INFO", __VA_ARGS__)
#define JBU_WARN(...)       ::Logger::Log("WARN", __VA_ARGS__)
#define JBU_ERR(...)        ::Logger::Log("ERR ", __VA_ARGS__)

// 단계 진입(파일에도 기록). literal 만 — 수명 = 프로그램.
#define JBU_PHASE(name)     ::Logger::SetPhase(name)

// 단계 진입(메모리만 갱신, 파일 X). MainLoop 등 매 프레임 hot path 용.
#define JBU_PHASE_Q(name)   ::Logger::SetPhaseQuiet(name)
