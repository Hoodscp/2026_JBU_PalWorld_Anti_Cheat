#include "OtomoHook.h"
#include "../../Memory/HookManager.h"
#include "../../Memory/Scanner.h"
#include "../../SDK/Pal.h"
#include "../../SDK/Offsets.h"
#include "../../SDK/Signatures.h"
#include <windows.h>
#include <atomic>
#include <iostream>

namespace OtomoHook
{
    // 후크 대상: void __fastcall UPalOtomoHolderComponentBase::SomeNoArgMethod(self).
    // x64 ABI 에서 this 는 rcx, 다른 인자 없음. detour 도 같은 시그니처라
    // 원본 호출 시 ABI 보존 (인자 있는 메서드에 잘못 걸면 rdx 가 깨져 크래시).
    using OtomoCallbackFn = void(__fastcall*)(void* self);
    static OtomoCallbackFn oOtomoCallback = nullptr;

    // 이미 캡쳐했으면 더 이상 작업 안 함 (atomic — UE 메인 스레드에서 호출).
    static std::atomic<bool> g_captured{false};

    // UPalOtomoHolderComponentBase::CharacterContainer 멤버 오프셋 (Reference/SDK
    // Pal_classes.hpp:48527).
    static constexpr uintptr_t kCharacterContainerOffset = 0x110;

    // 로컬 PlayerState 의 OtomoCharacterContainerId(16 B) 와 캡쳐된 컨테이너의
    // ID(+0x38, 16 B) 가 일치하는지 검증. 로컬 ID 가 0/미동기화면 그대로 통과
    // (솔로 / 첫 등장 시 false-negative 방지).
    static bool MatchesLocalOtomoId(uintptr_t container)
    {
        if (!container) return false;
        const uintptr_t cIdAddr = container + SDK::Offsets::ContainerBase::ID;
        if (IsBadReadPtr((const void*)cIdAddr, SDK::Offsets::ContainerBase::ID_Size)) return false;

        uintptr_t ps = SDK::GetLocalPlayerState();
        if (!ps) return true; // PlayerState 아직 — 검증 보류, 일단 등록 허용

        uintptr_t otomoData = 0;
        const uintptr_t odAddr = ps + SDK::Offsets::PlayerState::OtomoData;
        if (IsBadReadPtr((const void*)odAddr, sizeof(uintptr_t))) return true;
        otomoData = *(uintptr_t*)odAddr;
        if (!otomoData) return true;

        const uintptr_t localIdAddr = otomoData + SDK::Offsets::PlayerOtomoData::OtomoCharacterContainerId;
        if (IsBadReadPtr((const void*)localIdAddr, SDK::Offsets::ContainerBase::ID_Size)) return true;

        const uint64_t llo = *(const uint64_t*)(localIdAddr + 0x0);
        const uint64_t lhi = *(const uint64_t*)(localIdAddr + 0x8);
        if (llo == 0 && lhi == 0) return true; // 로컬 ID 미발급 — 검증 보류

        const uint64_t clo = *(const uint64_t*)(cIdAddr + 0x0);
        const uint64_t chi = *(const uint64_t*)(cIdAddr + 0x8);
        return (clo == llo) && (chi == lhi);
    }

    static void __fastcall hkOtomoCallback(void* self)
    {
        // 원본 먼저 호출 — 게임이 컨테이너 생성/RepNotify 로직을 마치게 함.
        // 그 다음 this+0x110 을 읽으면 가장 신선한 값을 잡을 수 있음.
        if (oOtomoCallback) oOtomoCallback(self);

        if (g_captured.load(std::memory_order_acquire)) return;
        if (!self) return;

        const uintptr_t ccAddr = reinterpret_cast<uintptr_t>(self) + kCharacterContainerOffset;
        if (IsBadReadPtr((const void*)ccAddr, sizeof(uintptr_t))) return;
        const uintptr_t container = *(uintptr_t*)ccAddr;
        if (!container) return; // 아직 미생성 — 다음 호출 시 재시도

        // 멀티 안전: 로컬 OtomoId 와 일치하는 컨테이너만 등록.
        if (!MatchesLocalOtomoId(container)) return;

        SDK::SetOtomoContainerOverride(container);
        g_captured.store(true, std::memory_order_release);
    }

    bool Install()
    {
        const char* sig = SDK::Signatures::OtomoContainerCapture;
        if (!sig || !*sig) {
            std::cout << "[*] OtomoHook: signature empty, skip install (UI 의 수동 hex 입력으로 대체).\n";
            return true; // placeholder 모드 — 실패 아님 (ExampleHook 패턴 동일)
        }

        const uintptr_t addr = Scanner::FindPattern("Palworld-Win64-Shipping.exe", sig);
        if (!addr) {
            std::cout << "[-] OtomoHook: OtomoContainerCapture pattern not found.\n";
            return false;
        }

        std::cout << "[+] OtomoHook: target @ 0x" << std::hex << addr << std::dec << "\n";

        if (!HookManager::CreateHook((void*)addr, &hkOtomoCallback, (void**)&oOtomoCallback)) {
            std::cout << "[-] OtomoHook: hook install failed.\n";
            return false;
        }

        std::cout << "[+] OtomoHook installed — container will be captured on first invocation.\n";
        return true;
    }
}
