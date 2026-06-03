// ProcessMonitorModule.cpp
#include "ProcessMonitorModule.h"

#include <windows.h>
#include <tlhelp32.h>
#include <bcrypt.h>

#include <chrono>
#include <string>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

namespace {

std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int len = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()),
                                    nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string out(static_cast<size_t>(len), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()),
                          out.data(), len, nullptr, nullptr);
    return out;
}

std::string ToLowerAscii(std::string s) {
    for (char& c : s) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return s;
}

uint64_t NowMsSystem() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

} // anonymous namespace

namespace ac {

ProcessMonitorModule::ProcessMonitorModule() = default;

bool ProcessMonitorModule::Initialize(DetectionEventQueue* queue) {
    queue_ = queue;
    return queue_ != nullptr;
}

void ProcessMonitorModule::Shutdown() {
    queue_ = nullptr;
    blacklist_.clear();
    reported_.clear();
    hashCache_.clear();
}

bool ProcessMonitorModule::EnumerateProcesses(std::vector<ProcInfo>& out) {
    HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    if (!::Process32FirstW(snap, &pe)) {
        ::CloseHandle(snap);
        return false;
    }

    do {
        ProcInfo info;
        info.pid = pe.th32ProcessID;
        info.imageNameLower = ToLowerAscii(WideToUtf8(pe.szExeFile));

        // 전체 경로 조회 (해시용). 권한 부족/시스템 프로세스면 실패할 수 있고,
        // 그 경우 이름 매칭만 수행한다.
        HANDLE h = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, info.pid);
        if (h) {
            wchar_t buf[MAX_PATH];
            DWORD sz = MAX_PATH;
            if (::QueryFullProcessImageNameW(h, 0, buf, &sz)) {
                info.fullPath.assign(buf, sz);
            }
            ::CloseHandle(h);
        }

        out.push_back(std::move(info));
    } while (::Process32NextW(snap, &pe));

    ::CloseHandle(snap);
    return true;
}

bool ProcessMonitorModule::ComputeFileSha256(const std::wstring& path, std::string& outHex) {
    if (path.empty()) return false;

    HANDLE file = ::CreateFileW(path.c_str(), GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;

    BCRYPT_ALG_HANDLE  alg  = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    bool ok = false;

    do {
        if (::BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0)
            break;

        DWORD hashLen = 0, cb = 0;
        if (::BCryptGetProperty(alg, BCRYPT_HASH_LENGTH,
                                reinterpret_cast<PUCHAR>(&hashLen),
                                sizeof(hashLen), &cb, 0) != 0)
            break;

        if (::BCryptCreateHash(alg, &hash, nullptr, 0, nullptr, 0, 0) != 0)
            break;

        std::vector<BYTE> buffer(64 * 1024);
        DWORD read = 0;
        bool feedErr = false;
        while (::ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr)
               && read > 0) {
            if (::BCryptHashData(hash, buffer.data(), read, 0) != 0) { feedErr = true; break; }
        }
        if (feedErr) break;

        std::vector<BYTE> digest(hashLen);
        if (::BCryptFinishHash(hash, digest.data(), hashLen, 0) != 0)
            break;

        static const char hexd[] = "0123456789ABCDEF";
        outHex.clear();
        outHex.reserve(static_cast<size_t>(hashLen) * 2);
        for (BYTE b : digest) {
            outHex.push_back(hexd[b >> 4]);
            outHex.push_back(hexd[b & 0x0F]);
        }
        ok = true;
    } while (false);

    if (hash) ::BCryptDestroyHash(hash);
    if (alg)  ::BCryptCloseAlgorithmProvider(alg, 0);
    ::CloseHandle(file);
    return ok;
}

void ProcessMonitorModule::Report(const BlacklistEntry& e, const ProcInfo& p,
                                  const char* matchType) {
    DetectionEvent ev;
    ev.moduleName    = Name();
    ev.detectionType = std::string("ProcessBlacklist.") + matchType;
    ev.severity      = e.severity;
    ev.description   = "Blacklisted tool detected: " + e.label;
    ev.evidence      = "pid=" + std::to_string(p.pid) +
                       " image=" + p.imageNameLower +
                       " match=" + matchType;
    ev.timestampMs   = NowMsSystem();
    if (queue_) queue_->Push(std::move(ev));
}

void ProcessMonitorModule::Tick() {
    if (!queue_) return;

    // 해시 항목이 하나도 없으면 디스크 해싱 자체를 건너뛴다 (성능).
    bool needHash = false;
    if (hashCheck_) {
        for (const auto& e : blacklist_) {
            if (!e.sha256.empty()) { needHash = true; break; }
        }
    }

    std::vector<ProcInfo> procs;
    if (!EnumerateProcesses(procs)) return;

    for (const auto& p : procs) {
        std::string hex;          // 프로세스당 1회만 계산
        bool hexAttempted = false;

        for (const auto& e : blacklist_) {
            const std::string key = std::to_string(p.pid) + ":" + e.label;
            if (reported_.count(key)) continue;

            // 1) 이름 매칭 (싸지만 리네임에 약함)
            if (!e.imageName.empty() && p.imageNameLower == e.imageName) {
                Report(e, p, "Name");
                reported_.insert(key);
                continue;
            }

            // 2) 해시 매칭 (리네임은 잡지만 재컴파일에 약함)
            if (needHash && !e.sha256.empty() && !p.fullPath.empty()) {
                if (!hexAttempted) {
                    auto it = hashCache_.find(p.fullPath);
                    if (it != hashCache_.end()) {
                        hex = it->second;
                    } else if (ComputeFileSha256(p.fullPath, hex)) {
                        hashCache_[p.fullPath] = hex;
                    } else {
                        hex.clear();
                    }
                    hexAttempted = true;
                }
                if (!hex.empty() && hex == e.sha256) {
                    Report(e, p, "Hash");
                    reported_.insert(key);
                }
            }
        }
    }
}

} // namespace ac
