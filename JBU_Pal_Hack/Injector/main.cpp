#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string>

// 프로세스 이름으로 Process ID 찾기
DWORD GetProcessIdByName(const std::wstring& processName) {
    DWORD processId = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(processEntry);
        
        if (Process32FirstW(hSnapshot, &processEntry)) {
            do {
                if (!_wcsicmp(processEntry.szExeFile, processName.c_str())) {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnapshot, &processEntry));
        }
        CloseHandle(hSnapshot);
    }
    return processId;
}

int main() {
    std::cout << "=========================================\n";
    std::cout << "   JBU Pal_Hack DLL Injector (LoadLibrary)\n";
    std::cout << "=========================================\n\n";

    std::wstring targetProcessName = L"Palworld-Win64-Shipping.exe";
    std::wstring dllName = L"JBU_Pal_Hack.dll"; // 같은 폴더에 있다고 가정

    // 1. 현재 폴더에 있는 DLL 파일의 전체 경로 구하기
    wchar_t dllPath[MAX_PATH];
    GetFullPathNameW(dllName.c_str(), MAX_PATH, dllPath, nullptr);

    if (GetFileAttributesW(dllPath) == INVALID_FILE_ATTRIBUTES) {
        std::wcerr << L"[!] DLL 파일을 찾을 수 없습니다: " << dllPath << L"\n";
        system("pause");
        return 1;
    }

    std::wcout << L"[*] 주입할 DLL 경로: " << dllPath << L"\n";
    std::wcout << L"[*] 대상 프로세스(" << targetProcessName << L") 찾는 중...\n";

    // 2. 타겟 프로세스 핸들 얻기
    DWORD processId = 0;
    while (!processId) {
        processId = GetProcessIdByName(targetProcessName);
        if (!processId) {
            Sleep(1000); // 1초 대기 후 다시 시도
        }
    }
    std::cout << "[+] 프로세스 찾음! PID: " << processId << "\n";

    // PROCESS_ALL_ACCESS 권한으로 핸들 열기
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        std::cerr << "[!] 권한 에러: 게임 프로세스에 접근할 수 없습니다 (인젝터를 관리자 권한으로 실행하세요!).\n";
        system("pause");
        return 1;
    }

    // 3. 게임 메모리 공간 내에 DLL 경로 문자열을 저장할 공간 할당 (VirtualAllocEx)
    size_t pathSize = (wcslen(dllPath) + 1) * sizeof(wchar_t);
    LPVOID pAllocatedMem = VirtualAllocEx(hProcess, nullptr, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pAllocatedMem) {
        std::cerr << "[!] 메모리 할당 실패 (VirtualAllocEx).\n";
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    // 4. 할당된 공간에 DLL 경로 문자열 쓰기 (WriteProcessMemory)
    if (!WriteProcessMemory(hProcess, pAllocatedMem, dllPath, pathSize, nullptr)) {
        std::cerr << "[!] 메모리 쓰기 실패 (WriteProcessMemory).\n";
        VirtualFreeEx(hProcess, pAllocatedMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    // 5. LoadLibraryW 함수의 주소 찾기 (모든 프로세스에서 Kernel32.dll의 기본 주소 오프셋은 동일함)
    FARPROC pLoadLibraryW = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryW");
    if (!pLoadLibraryW) {
        std::cerr << "[!] LoadLibraryW 주소 찾기 실패.\n";
        VirtualFreeEx(hProcess, pAllocatedMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    // 6. 대상 프로세스에 스레드 생성 명령을 내려 아까 쓴 메모리(DLL 경로)를 인자로 LoadLibraryW 실행 (CreateRemoteThread)
    std::cout << "[*] 원격 스레드 생성 중...\n";
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryW, pAllocatedMem, 0, nullptr);
    if (!hThread) {
        std::cerr << "[!] 원격 스레드 생성 실패 (CreateRemoteThread). 보안 솔루션이 차단했을 수 있습니다.\n";
        VirtualFreeEx(hProcess, pAllocatedMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    // 7. 주입 완료 후 스레드 종료 대기 (선택 사항) 및 정리
    WaitForSingleObject(hThread, INFINITE);
    std::cout << "[+] 인젝션 성공! DLL 구동 시작.\n";

    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pAllocatedMem, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    Sleep(2000);
    return 0;
}
