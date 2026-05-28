#include "NopHook.h"
#include "Scanner.h"
#include <windows.h>
#include <iostream>
#include <cstring>

namespace NopHook
{
    MidHook::Handle* Install(const char* hookName,
                             const char* moduleName,
                             const char* signature,
                             size_t      patchLen,
                             size_t      nopInstrLen)
    {
        if (!hookName) hookName = "NopHook";

        if (nopInstrLen > patchLen) {
            std::cout << "[-] " << hookName << ": nopInstrLen("
                      << nopInstrLen << ") > patchLen(" << patchLen << ").\n";
            return nullptr;
        }

        // 원본 바이트를 미리 읽어와 bodyEnabled/bodyDisabled 를 구성한다.
        // (MidHook 도 내부적으로 backup 하지만, 본 함수에서 tail 분리를 위해 다시 읽음.)
        if (!signature || !*signature) {
            // MidHook 이 동일 메시지를 찍지만 여기서 미리 skip 하면
            // Scanner 호출도 안 하게 되어 더 가벼움.
            std::cout << "[*] " << hookName << ": signature empty, skip install.\n";
            return nullptr;
        }
        const uintptr_t addr = Scanner::FindPattern(moduleName, signature);
        if (!addr) {
            std::cout << "[-] " << hookName << ": pattern not found in " << moduleName << ".\n";
            return nullptr;
        }

        uint8_t orig[64]{};
        if (patchLen > sizeof(orig)) {
            std::cout << "[-] " << hookName << ": patchLen too large.\n";
            return nullptr;
        }
        std::memcpy(orig, (const void*)addr, patchLen);

        // toggle ON 분기 = tail 바이트 (skipped first nopInstrLen)
        const uint8_t* bodyEnabled = orig + nopInstrLen;
        const size_t   bodyEnabledLen = patchLen - nopInstrLen;
        // toggle OFF 분기 = 원본 전체
        const uint8_t* bodyDisabled = orig;
        const size_t   bodyDisabledLen = patchLen;

        // MidHook 이 Scanner 를 다시 호출하지만, 그 비용은 한 번 install 시 한 번뿐이라
        // 무시. 동일 addr 을 반환할 것이 보장됨 (모듈 메모리가 install 사이에 안 바뀐다고 가정).
        return MidHook::Install(hookName, moduleName, signature,
                                patchLen,
                                bodyEnabled, bodyEnabledLen,
                                bodyDisabled, bodyDisabledLen);
    }
}
