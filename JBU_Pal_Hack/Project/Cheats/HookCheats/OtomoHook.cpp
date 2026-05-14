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

    // UPalOtomoHolderComponentBase::CharacterContainer 멤버 오프셋 (Reference/SDK
    // Pal_classes.hpp:48527).
    static constexpr uintptr_t kCharacterContainerOffset = 0x110;

    // 로컬 PlayerState 의 OtomoCharacterContainerId(16 B) 와 캡쳐된 컨테이너의
    // ID(+0x38, 16 B) 가 일치하는지 strict 검증. 이전 버전은 로컬 ID 가 아직
    // 동기화 안 됐을 때 "검증 보류 → 통과" 시켜서 NPC 트레이너 / 다른 플레이어
    // 의 OtomoHolder 가 잘못 캡쳐되는 사고가 있었음. 지금은 정보 부족 = false
    // 반환 → 다음 호출(=PlayerState 동기화 후)을 기다림.
    static bool MatchesLocalOtomoId(uintptr_t container)
    {
        if (!container) return false;
        const uintptr_t cIdAddr = container + SDK::Offsets::ContainerBase::ID;
        if (IsBadReadPtr((const void*)cIdAddr, SDK::Offsets::ContainerBase::ID_Size)) return false;

        const uintptr_t ps = SDK::GetLocalPlayerState();
        if (!ps) return false; // PlayerState 미동기화 → 다음 호출 대기

        const uintptr_t odAddr = ps + SDK::Offsets::PlayerState::OtomoData;
        if (IsBadReadPtr((const void*)odAddr, sizeof(uintptr_t))) return false;
        const uintptr_t otomoData = *(uintptr_t*)odAddr;
        if (!otomoData) return false; // OtomoData 미동기화 → 다음 호출 대기

        const uintptr_t localIdAddr = otomoData + SDK::Offsets::PlayerOtomoData::OtomoCharacterContainerId;
        if (IsBadReadPtr((const void*)localIdAddr, SDK::Offsets::ContainerBase::ID_Size)) return false;

        const uint64_t llo = *(const uint64_t*)(localIdAddr + 0x0);
        const uint64_t lhi = *(const uint64_t*)(localIdAddr + 0x8);
        if (llo == 0 && lhi == 0) return false; // 로컬 ID 아직 미발급 → 다음 호출 대기

        const uint64_t clo = *(const uint64_t*)(cIdAddr + 0x0);
        const uint64_t chi = *(const uint64_t*)(cIdAddr + 0x8);
        return (clo == llo) && (chi == lhi);
    }

    static void __fastcall hkOtomoCallback(void* self)
    {
        // 원본 먼저 호출 — 게임이 컨테이너 생성/RepNotify 로직을 마치게 함.
        // 그 다음 this+0x110 을 읽으면 가장 신선한 값을 잡을 수 있음.
        if (oOtomoCallback) oOtomoCallback(self);

        if (!self) return;

        const uintptr_t ccAddr = reinterpret_cast<uintptr_t>(self) + kCharacterContainerOffset;
        if (IsBadReadPtr((const void*)ccAddr, sizeof(uintptr_t))) return;
        const uintptr_t container = *(uintptr_t*)ccAddr;
        if (!container) return; // 아직 미생성 — 다음 호출 시 재시도

        // 멀티 안전: 로컬 OtomoId 와 일치하는 컨테이너만 등록. ID 미동기화 시
        // 잘못된 캡쳐 대신 deferral. g_captured 플래그 없이 매 호출 검증 →
        // 캐릭터 전환 / 세션 재로드 시 자동으로 새 컨테이너로 갱신.
        if (!MatchesLocalOtomoId(container)) return;

        // 이미 같은 값이면 atomic store 는 사실상 no-op, 다른 값이면 갱신.
        SDK::SetOtomoContainerOverride(container);
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
