# 2026_JBU_Pal_Anti_Cheat

> **학술 목적** — 팔월드(Palworld) 게임을 대상으로 내부 메모리 치트 기법을 분석하고, 이를 탐지하는 안티치트 시스템을 연구하기 위한 JBU 캡스톤 프로젝트입니다.

---

## 프로젝트 구조

```
2026_JBU_Pal_Anti_Cheat/
├── JBU_Pal_Hack/Project/   # 치트 클라이언트 (C++ DLL)
├── Anticheat_client/        # 안티치트 클라이언트 탐지 모듈 (C++)
├── Anticheat_server/        # 안티치트 서버 & 대시보드
└── Analyze/                 # 포인터 스캔 분석 도구
```

---

## JBU_Pal_Hack — 치트 클라이언트

Windows DLL 인젝션 방식의 Palworld 내부 치트 트레이너입니다.  
DirectX 11 후킹을 통한 ImGui 오버레이 메뉴와 두 가지 치트 트랙(Track 1 / Track 2)으로 구성됩니다.

### 아키텍처 개요

| 트랙 | 방식 | 동작 주기 | 범위 |
|------|------|-----------|------|
| **Track 1** | 메모리 폴링 (오프셋 체인 탐색 후 직접 쓰기) | 10ms/프레임 | 플레이어·슬롯 단위 |
| **Track 2 / P0** | AOB 스캐닝 + NOP 패치 (명령어 비활성화) | 토글 시 즉시 적용 | 게임 전역 메커니즘 |
| **Track 2 / P1** | SaveSpace 후킹 (레지스터 → 슬롯 캡처) | 해당 함수 호출 시 | 항상 활성, 포인터 백업용 |

---

### 모듈 구성

#### Core
- `dllmain.cpp` — DLL 진입점, 스레드 생성
- `HackMain.h/cpp` — 초기화·종료·메인 루프 총괄

#### SDK (`SDK/`)
- `Offsets.h` — 300+ 메모리 오프셋 정의
- `Signatures.h` — IDA 스타일 AOB 시그니처 모음
- `Pal.h/cpp` — 포인터 체인 헬퍼 40+ 함수 (Get/Set 쌍)
- `Engine_structs.h` — UE5 엔진 타입 정의
- `Pal_structs.h` — 게임 구조체 레이아웃

#### Memory (`Memory/`)
- `HookManager` — MinHook 래퍼
- `MidHook` — 범용 중간 함수 트램펄린 패치 (34바이트)
- `NopHook` — NOP 패턴 슈가 레이어
- `SaveSpaceHook` — 레지스터 값 캡처 헬퍼
- `Scanner` — AOB 패턴 스캐너 + RVA 해석
- `CursorHooks` — SetCursorPos/ClipCursor 바이패스

#### GUI (`GUI/`)
- `Overlay` — DirectX 11 Present 후킹 + ImGui 초기화
- `Menu` — 7탭 ImGui 오버레이 메뉴 (50+ 토글, 10+ 입력 필드)

---

### 구현된 치트 기능

#### Player 탭
| 기능 | 방식 | 설명 |
|------|------|------|
| 무적 (God Mode) | Track 1 | HP = 1,000,000,000 (매 프레임) |
| 무한 스태미나 | Track 1 + Track 2 | Track 1: SP 강제 쓰기 / Track 2: subss 명령어 NOP |
| 무한 탄약 | Track 1 + Track 2 | Track 1: 탄약 강제 쓰기 / Track 2: 탄약 감소 명령어 NOP |
| 배고픔 없음 | Track 1 + Track 2 | Track 1: FullStomach 강제 쓰기 / Track 2: hunger subtract NOP |
| 쉴드 부스트 | Track 1 | ShieldHP = 1,000,000,000 |
| 무한 스탯 포인트 | Track 1 | UnusedStatusPoint = 999 (재분배 무제한) |

#### Stat Boost 탭
| 기능 | 설명 |
|------|------|
| 최대 정신력 (Sanity) | Sanity = 100.0 |
| 무한 MP | MP = 1,000,000,000 |
| 최대 탤런트 (IV) | HP·근접·원거리·방어 모두 100 |
| 레벨 강제 설정 | Level 1–255 입력 값 적용 |
| 경험치 강제 설정 | Exp 입력 값 적용 |

#### Tech 탭
| 기능 | 설명 |
|------|------|
| 무한 기술 포인트 | TechnologyPoint = 9999 |
| 무한 보스 기술 포인트 | BossTechnologyPoint = 9999 |

#### Social 탭
| 기능 | 설명 |
|------|------|
| 우정 포인트 강제 설정 | FriendshipPoint 입력 값 적용 |
| 아레나 랭크 포인트 강제 설정 | ArenaRankPoint 입력 값 적용 |

#### Pals (팰 박스) 탭
| 기능 | 설명 |
|------|------|
| 팰 무적 | 선택 슬롯 HP = 1,000,000,000 |
| 팰 최대 정신력 | Sanity = 100.0 |
| 팰 무한 MP | MP = 1,000,000,000 |
| 팰 최대 탤런트 | 4종 IV 모두 100 |
| 팰 레벨 강제 설정 | Level 1–255 |
| 팰 경험치 강제 설정 | Exp ≥ 0 |

> 팰 박스 컨테이너 한정 (Party/Otomo 미지원)

#### Environment 탭
| 기능 | 설명 |
|------|------|
| 항상 적정 온도 | Temperature 상태값 0(Normal) 강제 (MidHook 트램펄린 방식) |

#### Inventory 탭
| 기능 | 방식 | 설명 |
|------|------|------|
| 아이템 수량 강제 설정 | Track 1 | 선택 컨테이너·슬롯 StackCount 강제 쓰기 |
| 음식 신선도 유지 (단일) | Track 1 | 선택 슬롯 Corruption = 0 |
| 음식 신선도 유지 (전체) | Track 1 | 컨테이너 전 슬롯 Corruption = 0 |
| 내구도 최대 유지 | Track 1 | DynamicItemData Durability = MaxDurability (GUObjectArray 해석) |
| 남은 총알 강제 설정 | Track 1 | DynamicItemData RemainingBullets 입력 값 적용 |
| 아이템 감소 없음 | Track 2 | StackCount 쓰기 명령어 NOP |
| 무한 내구도 | Track 2 | Durability 쓰기 명령어 NOP |
| 음식 타이머 동결 | Track 2 | 조리·숙성 타이머 쓰기 명령어 NOP |

---

### Track 2 AOB 시그니처 목록

| 훅 이름 | 패치 대상 명령어 | 바이트 수 |
|---------|----------------|-----------|
| StaminaSubHook | `subss xmm0, xmm2` | 4 bytes |
| FoodHungerHook | `subss xmm2, xmm7` | 12 bytes |
| FoodTimerHook | `movss [rbx+0x158], xmm0` | 8 bytes |
| WriteItemsHook | `mov [rbx+0x154], r14d` | 7 bytes |
| DurabilityWriteHook | `movss [rcx+0x10], xmm0` | 5 bytes |
| AmmoDecreaseHook | `lea eax,[rdx-1]` + write | 9 bytes |
| TemperatureHook | eax 강제 0 (커스텀 트램펄린) | 14 bytes |

---

### 폴백 포인터 캡처 (P1)

GWorld 오프셋이 게임 패치로 무효화될 경우를 대비한 항상 활성 백업 훅:

| 훅 이름 | 캡처 레지스터 | 용도 |
|---------|-------------|------|
| PlayerPtrCaptureHook | RCX | APalCharacter this 포인터 |
| ItemSlotCaptureHook | RCX | 인벤토리 컨테이너 루트 |
| TechPointCaptureHook | RDI | UPalTechnologyData* |
| ExpPtrCaptureHook | RCX+0x338 | EXP 오브젝트 |

---

### 빌드 요구사항

- **플랫폼**: Windows 10/11 x64
- **컴파일러**: MSVC (Visual Studio 2022)
- **빌드 시스템**: CMake 3.20+
- **의존성** (자동 다운로드 또는 `Vendor/` 포함):
  - [MinHook](https://github.com/TsudaKageyu/minhook)
  - [ImGui](https://github.com/ocornut/imgui) v1.90+
  - [Kiero](https://github.com/Rebzzel/kiero) (DirectX 11 후킹)

```bash
# 벤더 설정
setup_vendor.bat

# CMake 빌드
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

---

## Anticheat Client — 탐지 모듈

치트를 탐지하는 클라이언트 측 C++ 모듈 모음입니다. 모든 모듈은 공통 인터페이스
`IDetectionModule` 를 구현하고 결과를 단일 `DetectionEventQueue` 로 보냅니다.

### 외부 감시(External) 모듈 — 게임 프로세스 밖에서 PID 로 검사

| 모듈 | 탐지 대상 | 노리는 치트 |
|------|-----------|-------------|
| **ProcessMonitor** | 블랙리스트 외부 도구(CE/x64dbg/IDA 등) 이름·SHA-256 | external R/W·디버깅 도구 |
| **InjectionScanner** | 비신뢰/미서명/신규 DLL, 수동 매핑(reflective) 영역 | DLL 인젝션·수동 매핑 |
| **HookScanner** | 시스템 API 익스포트 인라인 후크 + 메인 모듈 IAT 후크 | MinHook/Detours 인라인·IAT 후크 |
| **IntegrityScanner** | 라이브 `.text` vs 디스크 원본 PE 비교(ASLR 보정) | Track 2 NOP 패치·MidHook 트램펄린 |

네 모듈은 `ac_external_monitor` 실행파일로 **통합**되어 단일 스케줄러/큐로 동작합니다.
자세한 내용은 [`Anticheat_client/External/README.md`](Anticheat_client/External/README.md) 참고.

### 안티디버깅 모듈

PID 대상 디버거 탐지(원격 디버거·디버그 포트/오브젝트·PEB 플래그·INT3/하드웨어
브레이크포인트·시간 기반 정체 등). 자세한 내용은
[`Anticheat_client/Module_Anti_Debugging/README.md`](Anticheat_client/Module_Anti_Debugging/README.md) 참고.

---

## Anticheat Server

Node.js 기반 REST API 서버 및 React 대시보드.  
자세한 내용은 [`Anticheat_server/Rest_api_dashboard/README.md`](Anticheat_server/Rest_api_dashboard/README.md) 참고.

---

## Analyze — 포인터 스캔 보드

메모리 오프셋 분석 도구.  
자세한 내용은 [`Analyze/PointerScanBoard/README.md`](Analyze/PointerScanBoard/README.md) 참고.

---

## 면책 조항

이 프로젝트는 **교육 및 연구 목적**으로만 제작되었습니다.  
실제 멀티플레이어 환경에서 사용하는 것은 서비스 약관 위반이며, 다른 플레이어에게 피해를 줄 수 있습니다.
