// AcCommon.h
// 외부 감시(External) 탐지 모듈들이 공유하는 헤더 전용 헬퍼 모음.
//
// 설계 의도:
//   - 인젝션/후킹/무결성 탐지 모듈은 모두 "팰월드 프로세스를 외부에서 PID로
//     열어 ReadProcessMemory 로 들여다본다"는 동일한 패턴을 쓴다.
//   - 그 과정에서 반복되는 작업(프로세스 찾기, 모듈 열거, 원격 메모리 읽기,
//     디스크상의 PE 파일 파싱: 섹션/익스포트/베이스 재배치)을 한곳에 모은다.
//   - 헤더 전용(inline)으로 두어 각 모듈이 .cpp 링크 의존성 없이 포함만 하면
//     쓰도록 한다. (기존 IDetectionModule.h 도 헤더 전용 큐를 쓰는 것과 동일 결.)
//
// 전제: 대상 프로세스는 x64. (팰월드 = UE5 x64 Shipping 빌드)
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "IDetectionModule.h"  // ac::DetectionEvent / Severity 등 (include dir 로 해석)

namespace ac {

// ─────────────────────────────────────────────────────────────────────────────
// 1) 문자열 / 시간 헬퍼
// ─────────────────────────────────────────────────────────────────────────────

inline std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int len = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()),
                                    nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string out(static_cast<size_t>(len), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()),
                          out.data(), len, nullptr, nullptr);
    return out;
}

inline std::string ToLowerAscii(std::string s) {
    for (char& c : s) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return s;
}

// 소문자 base 파일명(utf-8). "C:\\X\\Pal.EXE" -> "pal.exe"
inline std::string BaseNameLower(const std::wstring& path) {
    size_t slash = path.find_last_of(L"\\/");
    std::wstring base = (slash == std::wstring::npos) ? path : path.substr(slash + 1);
    return ToLowerAscii(WideToUtf8(base));
}

// 바이트 → 대문자 16진. spaced=true 면 "E9 12 34" 형태(근거 문자열용).
inline std::string BytesToHex(const uint8_t* data, size_t len, bool spaced = false) {
    static const char hexd[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(len * (spaced ? 3 : 2));
    for (size_t i = 0; i < len; ++i) {
        if (spaced && i) out.push_back(' ');
        out.push_back(hexd[data[i] >> 4]);
        out.push_back(hexd[data[i] & 0x0F]);
    }
    return out;
}

inline std::string PtrHex(uint64_t v) {
    static const char hexd[] = "0123456789ABCDEF";
    char buf[19] = "0x";
    int p = 2;
    bool started = false;
    for (int shift = 60; shift >= 0; shift -= 4) {
        int nib = static_cast<int>((v >> shift) & 0xF);
        if (nib || started || shift == 0) { buf[p++] = hexd[nib]; started = true; }
    }
    buf[p] = '\0';
    return std::string(buf);
}

// 절대 시각(system_clock epoch ms). DetectionEvent.timestampMs 용.
inline uint64_t NowMsSystem() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

// ─────────────────────────────────────────────────────────────────────────────
// 2) 대상 프로세스 탐색
// ─────────────────────────────────────────────────────────────────────────────

// 감시 대상 지정. 이름으로 매 틱 재해석(게임 재시작 추적) 하거나 PID 고정.
struct TargetSpec {
    std::string imageNameLower = "palworld-win64-shipping.exe";  // 팰월드 UE5 Shipping
    DWORD       fixedPid       = 0;  // 0 이면 이름으로 해석

    DWORD Resolve() const;  // 정의는 FindPidByImageNameLower 뒤
};

// 이미지명(소문자, 예 "palworld-win64-shipping.exe")으로 첫 PID 를 찾는다. 없으면 0.
inline DWORD FindPidByImageNameLower(const std::string& wantLower) {
    HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    DWORD found = 0;
    if (::Process32FirstW(snap, &pe)) {
        do {
            if (ToLowerAscii(WideToUtf8(pe.szExeFile)) == wantLower) {
                found = pe.th32ProcessID;
                break;
            }
        } while (::Process32NextW(snap, &pe));
    }
    ::CloseHandle(snap);
    return found;
}

inline DWORD TargetSpec::Resolve() const {
    return fixedPid ? fixedPid : FindPidByImageNameLower(imageNameLower);
}

// ─────────────────────────────────────────────────────────────────────────────
// 3) 원격 모듈 열거 (Toolhelp — 별도 psapi 링크 불필요)
// ─────────────────────────────────────────────────────────────────────────────

struct RemoteModule {
    std::string  nameLower;  // 소문자 base 이름
    std::wstring path;       // 전체 경로
    uintptr_t    base = 0;   // 로드 베이스
    uint32_t     size = 0;   // 이미지 크기(byte)

    bool Contains(uintptr_t addr) const {
        return addr >= base && addr < base + size;
    }
};

inline bool EnumRemoteModules(DWORD pid, std::vector<RemoteModule>& out) {
    out.clear();
    HANDLE snap = INVALID_HANDLE_VALUE;
    // 대상이 64비트면 SNAPMODULE 만으로 충분. ERROR_BAD_LENGTH 시 재시도.
    for (int attempt = 0; attempt < 8; ++attempt) {
        snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
        if (snap != INVALID_HANDLE_VALUE) break;
        if (::GetLastError() != ERROR_BAD_LENGTH) return false;
    }
    if (snap == INVALID_HANDLE_VALUE) return false;

    MODULEENTRY32W me{};
    me.dwSize = sizeof(me);
    if (::Module32FirstW(snap, &me)) {
        do {
            RemoteModule m;
            m.path      = me.szExePath;
            m.nameLower = ToLowerAscii(WideToUtf8(me.szModule));
            m.base      = reinterpret_cast<uintptr_t>(me.modBaseAddr);
            m.size      = me.modBaseSize;
            out.push_back(std::move(m));
        } while (::Module32NextW(snap, &me));
    }
    ::CloseHandle(snap);
    return !out.empty();
}

// addr 가 속한 모듈을 찾는다(없으면 nullptr).
inline const RemoteModule* ModuleOf(const std::vector<RemoteModule>& mods, uintptr_t addr) {
    for (const auto& m : mods) {
        if (m.Contains(addr)) return &m;
    }
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// 4) 원격 메모리 읽기
// ─────────────────────────────────────────────────────────────────────────────

inline bool ReadRemote(HANDLE proc, uintptr_t addr, void* buf, size_t n) {
    SIZE_T got = 0;
    return ::ReadProcessMemory(proc, reinterpret_cast<LPCVOID>(addr), buf, n, &got) && got == n;
}

template <typename T>
inline std::optional<T> ReadRemoteT(HANDLE proc, uintptr_t addr) {
    T v{};
    if (ReadRemote(proc, addr, &v, sizeof(T))) return v;
    return std::nullopt;
}

// 원격에서 NUL 종료 ASCII 문자열 읽기(최대 max).
inline std::string ReadRemoteCStr(HANDLE proc, uintptr_t addr, size_t max = 256) {
    std::string s;
    s.reserve(32);
    for (size_t i = 0; i < max; ++i) {
        char c = 0;
        if (!ReadRemote(proc, addr + i, &c, 1) || c == '\0') break;
        s.push_back(c);
    }
    return s;
}

// VirtualQueryEx 로 커밋된 영역 전체를 순회.
inline std::vector<MEMORY_BASIC_INFORMATION> EnumRegions(HANDLE proc) {
    std::vector<MEMORY_BASIC_INFORMATION> out;
    uintptr_t addr = 0;
    MEMORY_BASIC_INFORMATION mbi{};
    while (::VirtualQueryEx(proc, reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi)) == sizeof(mbi)) {
        out.push_back(mbi);
        uintptr_t next = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        if (next <= addr) break;  // 오버플로/정체 방지
        addr = next;
    }
    return out;
}

inline bool IsExecProtect(DWORD protect) {
    DWORD p = protect & 0xFF;
    return p == PAGE_EXECUTE || p == PAGE_EXECUTE_READ ||
           p == PAGE_EXECUTE_READWRITE || p == PAGE_EXECUTE_WRITECOPY;
}

// ─────────────────────────────────────────────────────────────────────────────
// 5) 디스크상의 PE 파일 평면 매핑 + 파싱 (x64 전용)
//    무결성 검사: 디스크 원본 .text vs 라이브 .text 비교용.
//    후킹 검사:   익스포트 원본 프롤로그 바이트 확보용.
// ─────────────────────────────────────────────────────────────────────────────

struct RelocEntry {
    uint32_t rva;    // 재배치가 적용되는 시작 RVA
    uint8_t  width;  // 바이트 폭(DIR64=8, HIGHLOW=4)
};

class DiskImage {
public:
    DiskImage() = default;
    ~DiskImage() { Close(); }
    DiskImage(const DiskImage&) = delete;
    DiskImage& operator=(const DiskImage&) = delete;

    bool Open(const std::wstring& path) {
        Close();
        file_ = ::CreateFileW(path.c_str(), GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_ == INVALID_HANDLE_VALUE) { file_ = nullptr; return false; }

        LARGE_INTEGER sz{};
        if (!::GetFileSizeEx(file_, &sz) || sz.QuadPart < static_cast<LONGLONG>(sizeof(IMAGE_DOS_HEADER))) {
            Close(); return false;
        }
        size_ = static_cast<size_t>(sz.QuadPart);

        mapping_ = ::CreateFileMappingW(file_, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!mapping_) { Close(); return false; }
        view_ = static_cast<const uint8_t*>(::MapViewOfFile(mapping_, FILE_MAP_READ, 0, 0, 0));
        if (!view_) { Close(); return false; }

        return Parse();
    }

    void Close() {
        if (view_)    { ::UnmapViewOfFile(view_); view_ = nullptr; }
        if (mapping_) { ::CloseHandle(mapping_);   mapping_ = nullptr; }
        if (file_)    { ::CloseHandle(file_);      file_ = nullptr; }
        size_ = 0; nt_ = nullptr; sections_.clear();
    }

    bool Valid() const { return view_ != nullptr && nt_ != nullptr; }
    uint64_t PreferredBase() const { return nt_ ? nt_->OptionalHeader.ImageBase : 0; }
    uint32_t SizeOfImage()  const { return nt_ ? nt_->OptionalHeader.SizeOfImage : 0; }
    const std::vector<IMAGE_SECTION_HEADER>& Sections() const { return sections_; }

    // RVA → 파일 오프셋. 실패 시 false.
    bool RvaToOffset(uint32_t rva, uint32_t& outOff) const {
        for (const auto& s : sections_) {
            uint32_t va = s.VirtualAddress;
            uint32_t rawSz = s.SizeOfRawData;
            if (rva >= va && rva < va + rawSz) {
                outOff = s.PointerToRawData + (rva - va);
                return outOff < size_;
            }
        }
        // 헤더 영역(섹션 밖, RVA==파일오프셋)
        if (rva < nt_->OptionalHeader.SizeOfHeaders && rva < size_) { outOff = rva; return true; }
        return false;
    }

    // RVA 위치의 파일 포인터(need 바이트 보장). 실패 시 nullptr.
    const uint8_t* RvaPtr(uint32_t rva, uint32_t need) const {
        uint32_t off = 0;
        if (!RvaToOffset(rva, off)) return nullptr;
        if (static_cast<size_t>(off) + need > size_) return nullptr;
        return view_ + off;
    }

    uint32_t DataDirRva(int idx) const {
        if (!nt_ || idx < 0 || idx >= IMAGE_NUMBEROF_DIRECTORY_ENTRIES) return 0;
        return nt_->OptionalHeader.DataDirectory[idx].VirtualAddress;
    }
    uint32_t DataDirSize(int idx) const {
        if (!nt_ || idx < 0 || idx >= IMAGE_NUMBEROF_DIRECTORY_ENTRIES) return 0;
        return nt_->OptionalHeader.DataDirectory[idx].Size;
    }

    // 익스포트 이름 → RVA. forwarder(익스포트 디렉터리 내부를 가리킴)는 제외.
    bool BuildExportMap(std::vector<std::pair<std::string, uint32_t>>& out) const {
        out.clear();
        uint32_t dirRva  = DataDirRva(IMAGE_DIRECTORY_ENTRY_EXPORT);
        uint32_t dirSize = DataDirSize(IMAGE_DIRECTORY_ENTRY_EXPORT);
        if (!dirRva) return false;
        auto* ed = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(
            RvaPtr(dirRva, sizeof(IMAGE_EXPORT_DIRECTORY)));
        if (!ed) return false;

        auto* funcs = reinterpret_cast<const uint32_t*>(RvaPtr(ed->AddressOfFunctions,
                                                               ed->NumberOfFunctions * 4));
        auto* names = reinterpret_cast<const uint32_t*>(RvaPtr(ed->AddressOfNames,
                                                               ed->NumberOfNames * 4));
        auto* ords  = reinterpret_cast<const uint16_t*>(RvaPtr(ed->AddressOfNameOrdinals,
                                                               ed->NumberOfNames * 2));
        if (!funcs || !names || !ords) return false;

        for (uint32_t i = 0; i < ed->NumberOfNames; ++i) {
            const char* nm = reinterpret_cast<const char*>(RvaPtr(names[i], 1));
            if (!nm) continue;
            uint16_t ord = ords[i];
            if (ord >= ed->NumberOfFunctions) continue;
            uint32_t funcRva = funcs[ord];
            if (!funcRva) continue;
            // forwarder 제외
            if (funcRva >= dirRva && funcRva < dirRva + dirSize) continue;
            out.emplace_back(ReadCStr(nm), funcRva);
        }
        return true;
    }

    // 베이스 재배치 항목들을 RVA 오름차순으로 수집. 무결성 비교 시 "이 바이트는
    // ASLR 재배치로 정상적으로 달라질 수 있다"고 판단해 건너뛰는 데 쓴다.
    bool BuildRelocs(std::vector<RelocEntry>& out) const {
        out.clear();
        uint32_t dirRva  = DataDirRva(IMAGE_DIRECTORY_ENTRY_BASERELOC);
        uint32_t dirSize = DataDirSize(IMAGE_DIRECTORY_ENTRY_BASERELOC);
        if (!dirRva || !dirSize) return true;  // 재배치 없음 → 빈 목록은 정상

        uint32_t consumed = 0;
        while (consumed < dirSize) {
            auto* block = reinterpret_cast<const IMAGE_BASE_RELOCATION*>(
                RvaPtr(dirRva + consumed, sizeof(IMAGE_BASE_RELOCATION)));
            if (!block || block->SizeOfBlock < sizeof(IMAGE_BASE_RELOCATION)) break;

            uint32_t count = (block->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / 2;
            auto* entries = reinterpret_cast<const uint16_t*>(
                RvaPtr(dirRva + consumed + sizeof(IMAGE_BASE_RELOCATION), count * 2));
            if (!entries) break;

            for (uint32_t i = 0; i < count; ++i) {
                uint16_t e   = entries[i];
                uint8_t  typ = static_cast<uint8_t>(e >> 12);
                uint16_t off = e & 0x0FFF;
                uint8_t  width = 0;
                if (typ == IMAGE_REL_BASED_DIR64)        width = 8;
                else if (typ == IMAGE_REL_BASED_HIGHLOW) width = 4;
                else continue;  // ABSOLUTE(0) 등은 무시
                out.push_back({ block->VirtualAddress + off, width });
            }
            consumed += block->SizeOfBlock;
        }
        std::sort(out.begin(), out.end(),
                  [](const RelocEntry& a, const RelocEntry& b) { return a.rva < b.rva; });
        return true;
    }

private:
    bool Parse() {
        if (size_ < sizeof(IMAGE_DOS_HEADER)) return false;
        auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(view_);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
        if (static_cast<size_t>(dos->e_lfanew) + sizeof(IMAGE_NT_HEADERS64) > size_) return false;

        auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(view_ + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return false;
        if (nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) return false;  // x64 전용
        nt_ = nt;

        auto* sec = IMAGE_FIRST_SECTION(nt);
        uint16_t n = nt->FileHeader.NumberOfSections;
        for (uint16_t i = 0; i < n; ++i) {
            // 범위 안전성 확인
            const uint8_t* p = reinterpret_cast<const uint8_t*>(&sec[i]);
            if (p + sizeof(IMAGE_SECTION_HEADER) > view_ + size_) break;
            sections_.push_back(sec[i]);
        }
        return true;
    }

    static std::string ReadCStr(const char* p) {
        std::string s;
        for (int i = 0; i < 512 && p[i]; ++i) s.push_back(p[i]);
        return s;
    }

    HANDLE         file_    = nullptr;
    HANDLE         mapping_ = nullptr;
    const uint8_t* view_    = nullptr;
    size_t         size_    = 0;
    const IMAGE_NT_HEADERS64* nt_ = nullptr;
    std::vector<IMAGE_SECTION_HEADER> sections_;
};

// reloc 정렬 목록에서 [rva, rva+len) 구간과 겹치는 재배치가 있는지.
inline bool RangeIsRelocated(const std::vector<RelocEntry>& relocs, uint32_t rva, uint32_t len) {
    if (relocs.empty()) return false;
    // 시작 rva 가 (검사구간끝)보다 작은 첫 후보부터 역방향으로 폭 검사.
    auto it = std::upper_bound(relocs.begin(), relocs.end(), rva + len - 1,
                               [](uint32_t v, const RelocEntry& e) { return v < e.rva; });
    while (it != relocs.begin()) {
        --it;
        if (it->rva + it->width <= rva) break;        // 더 앞쪽은 겹칠 수 없음
        if (it->rva < rva + len && it->rva + it->width > rva) return true;
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// 6) 라이브(원격) PE 헤더 정보 — IAT/Export 디렉터리 위치 등.
// ─────────────────────────────────────────────────────────────────────────────

struct RemotePeInfo {
    uint32_t sizeOfImage  = 0;
    uint32_t importRva    = 0, importSize  = 0;
    uint32_t exportRva    = 0, exportSize  = 0;
    bool     valid        = false;
};

inline RemotePeInfo ReadRemotePeInfo(HANDLE proc, uintptr_t base) {
    RemotePeInfo r;
    auto dos = ReadRemoteT<IMAGE_DOS_HEADER>(proc, base);
    if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE) return r;
    auto nt = ReadRemoteT<IMAGE_NT_HEADERS64>(proc, base + dos->e_lfanew);
    if (!nt || nt->Signature != IMAGE_NT_SIGNATURE) return r;
    if (nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) return r;

    r.sizeOfImage = nt->OptionalHeader.SizeOfImage;
    r.importRva   = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    r.importSize  = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
    r.exportRva   = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    r.exportSize  = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
    r.valid = true;
    return r;
}

// 함수 시작 바이트가 트램펄린/디투어 점프인지 식별. x64 인라인 후크 정형 패턴.
//   E9 ..              rel32 jmp
//   FF 25 .. (+rip)    jmp qword [rip+disp32]
//   68 .. C3           push imm32; ret
//   48 B8 <imm64> FF E0   mov rax, imm64; jmp rax
//   49 BB <imm64> 41 FF E3 mov r11, imm64; jmp r11
//   EB ..              short jmp
inline const char* ClassifyJumpStub(const uint8_t* b, size_t n) {
    if (n >= 1 && b[0] == 0xE9) return "jmp rel32";
    if (n >= 2 && b[0] == 0xFF && b[1] == 0x25) return "jmp [rip+x]";
    if (n >= 1 && b[0] == 0xEB) return "jmp short";
    if (n >= 6 && b[0] == 0x68 && b[5] == 0xC3) return "push/ret";
    if (n >= 12 && b[0] == 0x48 && b[1] == 0xB8 && b[10] == 0xFF && b[11] == 0xE0)
        return "mov rax;jmp rax";
    if (n >= 13 && b[0] == 0x49 && b[1] == 0xBB && b[11] == 0x41 && b[12] == 0xFF)
        return "mov r11;jmp r11";
    return nullptr;
}

}  // namespace ac
