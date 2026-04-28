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
        std::wcerr << L"[!] Could not find DLL file: " << dllPath << L"\n";
        system("pause");
        return 1;
    }

    std::wcout << L"[*] DLL path to inject: " << dllPath << L"\n";
    std::wcout << L"[*] Looking for target process (" << targetProcessName << L")...\n";

    // 2. 타겟 프로세스 핸들 얻기
    DWORD processId = 0;
    while (!processId) {
        processId = GetProcessIdByName(targetProcessName);
        if (!processId) {
            Sleep(1000); // Wait 1 second and retry
        }
    }
    std::cout << "[+] Process found! PID: " << processId << "\n";

    // PROCESS_ALL_ACCESS 권한으로 핸들 열기
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        std::cerr << "[!] Permission error: Cannot access the game process (Run injector as Administrator!).\n";
        system("pause");
        return 1;
    }

    // 3. 게임 메모리 공간 내에 DLL 경로 문자열을 저장할 공간 할당 (VirtualAllocEx)
    size_t pathSize = (wcslen(dllPath) + 1) * sizeof(wchar_t);
    LPVOID pAllocatedMem = VirtualAllocEx(hProcess, nullptr, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pAllocatedMem) {
        std::cerr << "[!] Memory allocation failed (VirtualAllocEx).\n";
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    // 4. 할당된 공간에 DLL 경로 문자열 쓰기 (WriteProcessMemory)
    if (!WriteProcessMemory(hProcess, pAllocatedMem, dllPath, pathSize, nullptr)) {
        std::cerr << "[!] Memory write failed (WriteProcessMemory).\n";
        VirtualFreeEx(hProcess, pAllocatedMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    // 5. LoadLibraryW 함수의 주소 찾기 (모든 프로세스에서 Kernel32.dll의 기본 주소 오프셋은 동일함)
    FARPROC pLoadLibraryW = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryW");
    if (!pLoadLibraryW) {
        std::cerr << "[!] Failed to find LoadLibraryW address.\n";
        VirtualFreeEx(hProcess, pAllocatedMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    // 6. 대상 프로세스에 스레드 생성 명령을 내려 아까 쓴 메모리(DLL 경로)를 인자로 LoadLibraryW 실행 (CreateRemoteThread)
    std::cout << "[*] Creating remote thread...\n";
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryW, pAllocatedMem, 0, nullptr);
    if (!hThread) {
        std::cerr << "[!] Failed to create remote thread (CreateRemoteThread). It might be blocked by an Anti-Cheat / Antivirus.\n";
        VirtualFreeEx(hProcess, pAllocatedMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    // 7. 주입 완료 후 스레드 종료 대기 (선택 사항) 및 정리
    WaitForSingleObject(hThread, INFINITE);
    std::cout << "[+] Injection successful! DLL is now running.\n";

    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pAllocatedMem, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    Sleep(2000);
    return 0;
}
