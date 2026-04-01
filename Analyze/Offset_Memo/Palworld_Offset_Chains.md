# Palworld Pointer Chain Analysis (Verified)

언리얼 엔진 5.1 기반 팰월드(Palworld)의 SDK 분석 및 실 메모리 오프셋 검증 데이터입니다.

## 1. 요약 표 (Summary Table)

| 항목 | 상세 포인터 체인 | 최종 오프셋 | 데이터 타입 | 비고 |
| :--- | :--- | :--- | :--- | :--- |
| **현재 체력** | `Pawn + 0x628 -> + 0x130 ->` | **0x400** | int64 | Value / 1000 |
| **기력 (SP)** | `Pawn + 0x628 ->` | **0x328** | int64 | Value / 1000 |
| **허기** | `Pawn + 0x628 -> + 0x130 ->` | **0x40C** | float | |
| **쉴드량** | `Pawn + 0x628 -> + 0x130 ->` | **0x488** | int64 | Value / 1000 |
| **스탯 포인트** | `Pawn + 0x628 -> + 0x130 ->` | **0x4FC** | uint16 | 미사용 포인트 |
| **기술 포인트** | `PlayerState + 0x5E8 ->` | **0x150** | int32 | 테크놀로지 포인트 |
| **인벤토리 수량** | `PlayerState + 0x5D8 -> ... -> Slot ->` | **0x154** | int32 | 개별 슬롯 내부 |
| **플레이어 좌표** | `Pawn + 0x198 ->` | **0x128** | FVector | X, Y, Z (Float) |

---

## 2. 베이스 포인터 (Base Pointer)
*   **GWorld:** `[base + 0x090D3030]`
*   **GameState:** `[[GWorld] + 0x158]`
*   **LocalPlayerState:** `[[GameState] + 0x2A8 + 0x00]` (PlayerArray의 첫 번째 요소)
*   **LocalPawn (APalCharacter):** `[[LocalPlayerState] + 0x308]` (PawnPrivate)

---

## 3. 항목별 수직 오프셋 체인 리스트

### 3.1 스탯 (미사용 포인트)
```text
4FC     ← (0x388 + 0x174) UnusedStatusPoint
130     ← IndividualParameter
628     ← CharacterParameterComponent
308     ← PawnPrivate
0       ← PlayerArray[0] (첫 번째 원소)
2A8     ← PlayerArray
158     ← GameState
```

### 3.2 현재 체력 (Current HP)
```text
400     ← (0x388 + 0x78) HP Value
130     ← IndividualParameter
628     ← CharacterParameterComponent
308     ← PawnPrivate
0       ← PlayerArray[0]
2A8     ← PlayerArray
158     ← GameState
```

### 3.3 기력 (Stamina / SP)
```text
328     ← SP (Stamina)
628     ← CharacterParameterComponent
308     ← PawnPrivate
0       ← PlayerArray[0]
2A8     ← PlayerArray
158     ← GameState
```

### 3.4 허기 (Hunger)
```text
40C     ← (0x388 + 0x84) FullStomach
130     ← IndividualParameter
628     ← CharacterParameterComponent
308     ← PawnPrivate
0       ← PlayerArray[0]
2A8     ← PlayerArray
158     ← GameState
```

### 3.5 쉴드량 (Shield HP)
```text
488     ← (0x388 + 0x100) ShieldHP
130     ← IndividualParameter
628     ← CharacterParameterComponent
308     ← PawnPrivate
0       ← PlayerArray[0]
2A8     ← PlayerArray
158     ← GameState
```

### 3.6 기술 포인트 (Technology Point)
```text
150     ← TechnologyPoint
5E8     ← TechnologyData
0       ← PlayerArray[0]
2A8     ← PlayerArray
158     ← GameState
```

### 3.7 인벤토리 (첫 번째 슬롯 아이템 수량)
```text
154     ← StackCount
0       ← SlotArray[0] (첫 번째 슬롯 객체)
70      ← ItemSlotArray
0       ← Containers[0] (첫 번째 컨테이너)
38      ← Containers
190     ← InventoryMultiHelper
5D8     ← InventoryData
0       ← PlayerArray[0]
2A8     ← PlayerArray
158     ← GameState
```

### 3.8 플레이어 좌표 (Player Coordinates - X)
```text
128     ← RelativeLocation (X) - 12C는 Y, 130은 Z
198     ← RootComponent
308     ← PawnPrivate
0       ← PlayerArray[0]
2A8     ← PlayerArray
158     ← GameState
```

---

## 4. 데이터 분석 요약
1.  **인라인 구조체 합산:** `SaveParameter`는 `IndividualParameter` 내부 `0x388`에 위치하며, 내부 필드와 오프셋을 합산하여 접근해야 합니다.
2.  **데이터 스케일:** HP, SP, Shield는 `int64`이며 실제 값에 **1000**이 곱해진 상태입니다.
3.  **좌표:** `RelativeLocation`은 `FVector` 타입으로 3개의 `Float`(X, Y, Z)이 연속되어 있습니다.
