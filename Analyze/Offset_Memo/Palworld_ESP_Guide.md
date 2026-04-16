# Palworld ESP (Extra Sensory Perception) 모드 제작 가이드

본 문서는 언리얼 엔진 5.1 기반 팰월드(Palworld)에서 야생 팰(Wild Pals)의 위치와 주요 정보를 추적하여 ESP 모드를 제작하기 위한 포인터 체인과 구현 원리를 설명합니다.

---

## 1. 주요 포인터 체인 및 오프셋 (Pointer Chains)

ESP 모드를 만들기 위해서는 월드에 존재하는 모든 액터(Actor)를 순회하면서, 해당 액터가 팰(Pal)인지, 야생 상태인지 판별한 후 좌표와 상태 정보를 읽어야 합니다.

### 1.1 베이스 포인터 및 액터 배열 (Actors Array)
월드에 스폰된 모든 객체는 `ULevel`의 `Actors` 배열에 저장됩니다.

| 항목 | 상세 포인터 체인 | 오프셋 / 타입 | 비고 |
| :--- | :--- | :--- | :--- |
| **GWorld** | `[base + 0x090D3030]` | - | 전역 월드 포인터 |
| **PersistentLevel** | `[[GWorld] + 0x30]` | - | 현재 유지되는 레벨 |
| **Actors Array** | `[[PersistentLevel] + 0x98]` | `TArray<AActor*>` | 액터 포인터 배열 |
| **Actors Count** | `[[PersistentLevel] + 0xA0]` | `int32` | 배열에 들어있는 액터 수 |

### 1.2 액터(Actor) 정보 읽기
`Actors` 배열에서 읽어온 개별 액터 포인터(`AActor*`)를 기준으로 다음 정보들을 탐색합니다.

| 항목 | 상세 포인터 체인 | 최종 오프셋 | 데이터 타입 | 비고 |
| :--- | :--- | :--- | :--- | :--- |
| **RootComponent** | `Actor + 0x198 ->` | - | 포인터 | |
| **좌표 (Location)** | `RootComponent + 0x128` | **0x128** | `FVector` | X, Y, Z (각 8바이트 Double) |
| **CharacterParameter** | `Actor + 0x628 ->` | - | 포인터 | |
| **IndividualParameter** | `CharacterParameter + 0x130 ->` | - | 포인터 | |
| **SaveParameter** | `IndividualParameter + 0x388` | **0x388** | 구조체 | 아래 1.3 참조 |

### 1.3 야생 팰 세부 정보 (SaveParameter 구조체 내부)
ESP에 표시할 팰의 이름, 레벨, 체력 등은 `SaveParameter` 구조체 안에 존재합니다. (`IndividualParameter + 0x388` 위치부터 시작)

| 항목 | `SaveParameter` 기준 오프셋 | 데이터 타입 | 비고 |
| :--- | :--- | :--- | :--- |
| **CharacterID** | `+ 0x0000` | `FName` (8바이트) | 팰의 내부 영문 ID (예: SheepBall) |
| **Level** | `+ 0x0020` | `uint8` (1바이트) | 팰의 레벨 |
| **현재 HP** | `+ 0x0078` | `int64` (8바이트) | 실제 값 = 읽은 값 / 1000 |
| **최대 HP** | `+ 0x00E0` | `int64` (8바이트) | 실제 값 = 읽은 값 / 1000 |
| **IsPlayer** | `+ 0x00B0` | `bool` (1바이트) | 1이면 플레이어, 0이면 팰/NPC |
| **OwnerPlayerUId** | `+ 0x00C0` | `FGuid` (16바이트) | 0으로 채워져 있으면 **야생(Wild)**, 값이 있으면 포획된 팰 |

---

## 2. ESP 모드 작동 원리 및 구현 방법

ESP(Extra Sensory Perception) 모드는 게임 메모리에서 추출한 3D 좌표를 플레이어의 화면(2D)에 투영하여 사각형(Box)이나 텍스트를 그리는 방식으로 작동합니다.

### 1단계: 액터 순회 및 필터링 (Iteration & Filtering)
1. `GWorld -> PersistentLevel -> Actors` 배열 포인터와 `Count`를 읽어옵니다.
2. `0`부터 `Count - 1`까지 반복문을 돌며 `Actors[i]` 포인터를 가져옵니다.
3. 포인터가 유효한지(Null이 아닌지) 검사합니다.
4. `SaveParameter` 내부의 **`IsPlayer`** 값을 확인하여 플레이어 캐릭터를 제외합니다.
5. `SaveParameter` 내부의 **`OwnerPlayerUId`**를 검사하여 이 값이 0(Empty)이라면 해당 객체가 **야생 팰(Wild Pal)**임을 식별합니다.

### 2단계: 정보 수집 (Gathering Information)
필터링된 야생 팰 객체에 대해 다음 메모리를 읽어옵니다.
*   `RootComponent -> RelativeLocation`을 읽어 3D 공간상의 절대 좌표(X, Y, Z)를 획득합니다. (LWC 적용으로 Double 자료형을 사용해야 합니다.)
*   `SaveParameter`에서 `CharacterID`, `Level`, `HP`, `MaxHP` 값을 읽어 화면에 표시할 문자열을 구성합니다. 
    *   *(참고: `FName`인 `CharacterID`는 게임의 `GNames` 배열을 참조하여 실제 문자열로 변환해야 합니다.)*

### 3단계: 화면 투영 (World To Screen, W2S)
3D 좌표(X, Y, Z)를 플레이어의 모니터 해상도에 맞는 2D 좌표(X, Y)로 변환해야 합니다.
이를 위해 플레이어의 **카메라 정보(Camera Location, Camera Rotation, Field of View)**가 필요합니다.
*   `LocalPlayer` -> `PlayerController` -> `PlayerCameraManager`를 추적하여 카메라 정보를 가져옵니다.
*   투영 행렬(Projection Matrix) 수식을 사용하여 3D 위치를 화면상의 2D 픽셀 위치로 변환합니다. 만약 변환된 Z값이 0 이하라면 카메라 뒤에 있는 것이므로 그리지 않습니다.

### 4단계: 오버레이 렌더링 (Rendering Overlay)
게임 화면 위에 투명한 창을 띄우거나 후킹(Hooking)을 통해 직접 렌더링합니다. (주로 ImGui, DirectX/OpenGL Hooking이 사용됩니다.)
*   W2S로 변환된 2D 좌표를 중심으로 `[ Level 15 | 램무 (HP: 1000/1000) ]` 형태의 텍스트를 출력합니다.
*   팰의 거리에 따라 텍스트의 크기를 조절하거나, 박스(ESP Box)를 그려 시각적인 편의성을 높입니다.

---

## 3. 주의사항
*   언리얼 엔진 5.1의 **Large World Coordinates(LWC)** 적용으로 인해 공간 좌표나 변환 행렬 계산 시 `float` 대신 `double` 정밀도를 사용해야 정확한 W2S 계산이 가능합니다.
*   배열 순회 중 게임이 액터를 생성하거나 파괴할 수 있으므로, 메모리 읽기(ReadProcessMemory 등) 시 잘못된 주소 접근으로 인한 크래시(Crash) 방지 예외 처리가 필수적입니다.
*   온라인 멀티플레이나 공식 서버에서의 사용은 계정 정지(Ban)의 대상이 될 수 있습니다. 로컬 또는 허용된 환경에서만 사용 및 테스트해야 합니다.