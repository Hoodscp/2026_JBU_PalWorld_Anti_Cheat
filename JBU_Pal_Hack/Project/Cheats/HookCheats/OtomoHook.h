#pragma once

namespace OtomoHook
{
    // UPalOtomoHolderComponentBase 의 no-arg 멤버 함수에 후크를 걸어 진입 시
    // this+0x110 (CharacterContainer) 을 1 회 캡쳐 → SDK::SetOtomoContainerOverride
    // 에 등록한다. 이로써 GUObjectArray 풀스캔을 완전히 대체.
    //
    // 시그니처는 SDK::Signatures::OtomoContainerCapture. 빈 문자열이면 install
    // skip (UI 의 수동 hex 입력만 살아 있음).
    //
    // 멀티플레이 안전성: detour 안에서 컨테이너의 ID(+0x38, 16B)가 로컬
    // PlayerState.OtomoData.OtomoCharacterContainerId 와 일치할 때만 등록 →
    // 다른 플레이어의 OtomoHolder 가 잘못 잡히는 경우를 차단. 로컬 ID 가
    // 아직 동기화 전(=0)이면 첫 일치 시 그대로 등록 (솔로 케이스).
    //
    // Shutdown 은 HookManager::Shutdown 의 MH_DisableHook(MH_ALL_HOOKS) 가
    // 일괄 처리하므로 따로 노출하지 않음.
    bool Install();
}
