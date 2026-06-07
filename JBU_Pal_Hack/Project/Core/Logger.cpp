#include "Logger.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <streambuf>

namespace {

    // ── Tee streambuf ──
    // std::cout 으로 가는 모든 바이트를 (1) 원래 콘솔 streambuf 와
    // (2) 파일 ofstream 양쪽에 출력한다. '\n' 마다 파일 flush — 게임이
    // 다음 순간 튕겨도 마지막 한 줄까지 디스크에 남는다.
    class TeeBuf : public std::streambuf {
    public:
        TeeBuf(std::streambuf* console, std::ofstream* file)
            : console_(console), file_(file) {}

    protected:
        int overflow(int c) override {
            if (c == EOF) return !EOF;
            const char ch = static_cast<char>(c);
            if (console_) console_->sputc(ch);
            if (file_) {
                file_->put(ch);
                if (ch == '\n') file_->flush();
            }
            return c;
        }
        int sync() override {
            if (console_) console_->pubsync();
            if (file_) file_->flush();
            return 0;
        }

    private:
        std::streambuf* console_ = nullptr;
        std::ofstream*  file_    = nullptr;
    };

    // ── 전역 상태 ──
    std::mutex                          g_mtx;        // 파일 write 보호
    std::ofstream                       g_file;       // 로그 파일
    std::string                         g_path;       // 로그 절대 경로
    std::streambuf*                     g_originalCoutBuf = nullptr;
    TeeBuf*                             g_teeBuf  = nullptr;
    std::atomic<const char*>            g_phase{ "(none)" };
    bool                                g_initialized = false;

    // %TEMP%\JBU_Pal_Hack.log . %TEMP% 실패 시 현재 디렉터리로 폴백.
    std::string ResolveLogPath() {
        char buf[MAX_PATH];
        DWORD n = ::GetTempPathA(MAX_PATH, buf);
        if (n == 0 || n > MAX_PATH) return "JBU_Pal_Hack.log";
        std::string p(buf, buf + n);
        if (!p.empty() && p.back() != '\\' && p.back() != '/') p.push_back('\\');
        p += "JBU_Pal_Hack.log";
        return p;
    }

    // 'YYYY-MM-DD HH:MM:SS.mmm'  (system local time)
    std::string Timestamp() {
        SYSTEMTIME st{};
        ::GetLocalTime(&st);
        char buf[40];
        std::snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u.%03u",
                      st.wYear, st.wMonth, st.wDay,
                      st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        return buf;
    }

    void WriteHeader() {
        char modName[MAX_PATH] = {};
        ::GetModuleFileNameA(::GetModuleHandleA(nullptr), modName, MAX_PATH);
        DWORD pid = ::GetCurrentProcessId();

        g_file << "================================================================\n";
        g_file << " JBU Palworld Hack — Log started " << Timestamp() << "\n";
        g_file << "   PID         : " << pid << "\n";
        g_file << "   Host process: " << modName << "\n";
        g_file << " (1 line/event, flushed on '\\n'. Look at the LAST lines after a crash.)\n";
        g_file << "================================================================\n";
        g_file.flush();
    }

}  // anonymous namespace

namespace Logger {

bool Init() {
    std::lock_guard<std::mutex> lock(g_mtx);
    if (g_initialized) return true;

    g_path = ResolveLogPath();

    // 기존 로그가 있으면 .prev 로 회전(덮어쓰지 않음). 첫 줄로 분석 시 비교 가능.
    std::string prev = g_path + ".prev";
    ::DeleteFileA(prev.c_str());
    ::MoveFileA(g_path.c_str(), prev.c_str());  // 없어도 무해

    g_file.open(g_path, std::ios::out | std::ios::trunc);
    if (!g_file.is_open()) return false;

    WriteHeader();

    // std::cout 을 Tee 로 갈아끼움 — 기존 후크 코드가 cout 만 써도 파일에 들어감.
    g_originalCoutBuf = std::cout.rdbuf();
    g_teeBuf = new TeeBuf(g_originalCoutBuf, &g_file);
    std::cout.rdbuf(g_teeBuf);

    g_initialized = true;
    return true;
}

void Shutdown() {
    std::lock_guard<std::mutex> lock(g_mtx);
    if (!g_initialized) return;

    if (g_file.is_open()) {
        g_file << "[" << Timestamp() << "] [INFO] Logger::Shutdown — clean exit.\n";
        g_file.flush();
    }

    // cout 의 buf 원복 후 tee 해제 — 순서 중요(역순 사용 방지).
    if (g_originalCoutBuf) {
        std::cout.rdbuf(g_originalCoutBuf);
        g_originalCoutBuf = nullptr;
    }
    if (g_teeBuf) {
        delete g_teeBuf;
        g_teeBuf = nullptr;
    }
    if (g_file.is_open()) g_file.close();

    g_initialized = false;
}

void WriteLineRaw(const char* level, const char* msg) {
    std::lock_guard<std::mutex> lock(g_mtx);
    if (!g_file.is_open()) return;
    g_file << "[" << Timestamp() << "] [" << (level ? level : "INFO") << "] "
           << (msg ? msg : "") << "\n";
    g_file.flush();
}

void Log(const char* level, const char* fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    int n = std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if (static_cast<size_t>(n) >= sizeof(buf)) buf[sizeof(buf) - 1] = '\0';

    // 1) 파일에 직접 (시간 prefix 보장)
    WriteLineRaw(level, buf);
    // 2) cout 으로 한 번 더 — Tee 가 콘솔에도 같은 줄 출력.
    //    (파일에는 2번 들어가지만 cout 경로는 prefix 없이 들어가 흔적이 다름)
    //    중복 방지를 위해 콘솔 only 경로 사용: 원본 buf 가 있으면 그 쪽으로만.
    if (g_originalCoutBuf) {
        // 원본(콘솔) 으로만 직접 write — 파일 이중기록 방지.
        const char prefix[] = "[hack] ";
        g_originalCoutBuf->sputn(prefix, sizeof(prefix) - 1);
        g_originalCoutBuf->sputn(buf, static_cast<std::streamsize>(std::strlen(buf)));
        g_originalCoutBuf->sputc('\n');
        g_originalCoutBuf->pubsync();
    }
}

void SetPhase(const char* phase) {
    if (!phase) phase = "(null)";
    g_phase.store(phase, std::memory_order_relaxed);
    WriteLineRaw("PHAS", phase);
}

void SetPhaseQuiet(const char* phase) {
    if (!phase) phase = "(null)";
    g_phase.store(phase, std::memory_order_relaxed);
    // 파일 X — atomic store 1회만(거의 0 비용).
}

const char* CurrentPhase() {
    return g_phase.load(std::memory_order_relaxed);
}

const std::string& FilePath() {
    return g_path;
}

}  // namespace Logger
