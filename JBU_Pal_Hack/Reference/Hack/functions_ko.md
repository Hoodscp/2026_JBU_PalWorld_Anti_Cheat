# 치트 기능 분석 (Cheat Functions Analysis)

## 플레이어 스탯 (Player Stats)
트레이너는 대부분의 플레이어 관련 치트를 관리하기 위해 주요 코드 케이브(`aobplayerstats`)를 사용합니다.

- **무한 체력 / 갓 모드 (Infinite Health/God Mode)**:
    - `SParam_HP` 오프셋을 모니터링합니다.
    - 치트가 활성화되면 현재 HP를 `SParam_MaxHP`와 일치시키거나 매우 높은 상수값(예: 1000배 증폭)으로 계속 설정합니다.
- **무한 쉴드 (Infinite Shield)**:
    - `SParam_ShieldHP`를 거대한 값(예: `9,999,999,000`)으로 설정합니다.
- **무한 스태미나 (Infinite Stamina)**:
    - `Param_SP`를 수정하여 스태미나 수치를 최대치로 고정합니다.
- **무한 포만감 (Infinite Food)**:
    - `SParam_HungerType`을 0(없음)으로 설정하고 `SParam_FullStomach`를 최대치로 유지합니다.
- **제작 속도 배율 (Craft Speed Multiplier)**:
    - `SParam_CraftSpeed` 값을 사용자 정의 또는 하드코딩된 높은 값으로 수정합니다.
- **무게 제한 무시 (Max Weight Override)**:
    - `InventoryData_maxInventoryWeight`에 접근하여 매우 높은 값으로 덮어씀으로써 아이템을 무제한으로 소지할 수 있게 합니다.

## 전투 및 월드 (Combat and World)
- **스텔스 모드/투명화 (Stealth Mode)**:
    - 게임의 시야/감지 함수를 패치합니다. 감지 체크 후의 조건부 점프를 변경하여, 실제 상태와 상관없이 게임이 플레이어를 감지되지 않은 상태로 취급하도록 강제합니다.
- **온도 면역 (Temperature Immunity)**:
    - 환경 온도 효과 값을 0으로 만들어 극한의 더위나 추위로 인한 피해를 방지합니다.
- **희귀 리스폰 / 높은 스폰율 (Rare Respawn / High Spawn Rates)**:
    - 스폰율 계산이나 아이템 드롭률에 배율(스크립트에서 `10000.0`으로 확인됨)을 적용합니다.

## 진행도 (Progression)
- **무한 속성 포인트 (Infinite Attribute Points)**:
    - `SParam_UnusedStatusPoint`를 높은 값으로 수정합니다.
- **무한 기술 포인트 (Infinite Technology Points)**:
    - `PTD_TechnologyPoint` 및 `PTD_bossTechnologyPoint` 오프셋을 수정합니다.

## 유틸리티 (Utility)
- **게임 속도 조절 (Game Speed Control)**:
    - Unreal Engine의 글로벌 시간 팽창(Time Dilation) 계수를 직접 수정하거나 후킹하여 전체 게임 월드의 속도를 높이거나 낮춥니다.
