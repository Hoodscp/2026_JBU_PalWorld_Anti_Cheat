# 🎮 Palworld Pointer Analyzer Dashboard

Cheat Engine(치트 엔진) 메모리 해킹 중 발생하는 수만 개의 Pointer Scan(포인터 스캔) 결과 데이터에서 **오류 없이 구동되는 "진짜(Reliable) 공통 포인터"**만 극적으로 필터링해 주는 웹 기반 대시보드 도구입니다. 언리얼 엔진 기반 게임(Palworld 등)들의 메모리 구조 분석에 특화되어 있습니다.

> [!TIP]
> **주요 목적**
> 체력(HP), 스태미나 등 서로 다른 여러 인스턴스에서 뽑아낸 `.sqlite` 포인터 스캔 결과를 웹 인터페이스에 업로드하고 정교하게 교집합(Intersection)을 구하여 치트 엔진에서 튕기지 않는 **'절대적인 단일 조상 오프셋 체인'**을 발굴해 냅니다.

---

## 🚀 핵심 기능 (Key Features)

### 1. N-ary 트리 시각화 시스템
업로드한 포인터 체인 경로들을 분석하여 브라우저 환경에서 깔끔한 트리 구조(노드)로 병합하여 그려줍니다.
각 오프셋(e.g., `+0x4FC`와 `+0x400` 등) 들이 어디서부터 갈라지기 시작하는 지점인지 한눈에 추적할 수 있습니다.

### 2. 엄격한 교집합 필터 (Strict Intersection)
서로 다른 상황에서 스캔된 여러 개의 `.sqlite` 복사본 안에서 **모든 파일에 100% 동일하게 나타나는 안전한 오프셋 체인**만 남깁니다.

### 3. 언리얼 엔진 맞춤형 휴리스틱 고급 필터 (Heuristic Filters)
**[ Show Advanced Filters ⚙️ ]** 패널을 열어 찌꺼기 포인터들을 박멸할 수 있습니다:
- **Base Module Filter:** 시작 지점이 `Palworld-Win64-Shipping.exe` 같이 신뢰 가능한 메인 프로세스인 것만 통과시킵니다.
- **Max Depth Level:** 지나치게 깊고 비효율적인 분기(Depth Limit)를 잘라냅니다. 보통 7단계 이하를 권장합니다.
- **Max Offset Value:** `0xF20A8000` 처럼 비상식적으로 메모리를 크게 점프해버린 우연의 포인터를 차단합니다.
- **Require 8-byte Alignment:** 언리얼 엔진 포인터 자료구조 원리인 `8바이트 배열 정렬 규칙(divisible by 8)`을 위반하는 모든 홀수/비정상 데이터를 강제 삭제합니다.

### 4. 압도적인 용량 최적화 원본 내보내기 (Export to .zip)
위 필터 조건을 통과해서 살아남은 "최종 포인터 그룹" 이외의 모든 치트엔진 원본 `results` 행들을 완벽하게 도려냅니다 (`DELETE FROM results`).
이후 SQLite 의 `VACUUM` 명령을 동작시켜 텅 비어버린 50MB 의 잉여 디스크 공간 파편들마저 극한으로 압축시킨 후, Cheat Engine 으로 즉시 불러올 수 있는 순정 스키마 포맷 `.sqlite` 형태 그대로 `.zip` 아카이브에 담아 다운로드합니다.

---

## 🛠️ 기술 스택 (Tech Stack)

- **Backend:** `Python` (FastAPI), `SQLite3` 직접 트랜잭션 처리, `Uvicorn`
- **Frontend:** `React` (Vite), `Glassmorphism UI` 디자인 (CSS3)

---

## 📦 설치 및 실행 방법 (Getting Started)

> [!IMPORTANT]
> 로컬 컴퓨터에 Python 및 Node.js 패키지가 설치되어 있어야 합니다.

### 1. Backend API 서버 실행
백엔드는 포트 `8000`에서 가동되며 분석과 압축(Export) 로직을 전담합니다.
```bash
cd backend
pip install -r requirements.txt
python main.py
# 서버가 uvicorn http://0.0.0.0:8000 형태로 구동됩니다.
```

### 2. Frontend React 앱 실행
사용자가 보게 될 모던 대시보드 화면입니다. 포트 `5173` 으로 실행됩니다.
```bash
cd frontend
npm install
npm run dev
```

이후 표시된 로컬 네트워크 주소 혹은 `http://localhost:5173` 링크를 Ctrl + 클릭하여 웹브라우저 창을 여시면 됩니다.

---

## 📝 사용 가이드 (How to Use)
1. 치트엔진(Cheat Engine)에서 포인터 스캔을 돌리고 저장할 때, `.PTR` 텍스트 방식이 아닌 **SQLite Database 포맷**으로 추출(Export) 합니다.
2. 각기 상황이 다른 스캔 결과 SQLite 파일들(`.sqlite`)을 다중 선택하여 브라우저 대시보드의 **📁 Drag & Drop 영역** 에 떨궈줍니다.
3. 원하시는 Heuristic (뎁스 제한 등) 입맛에 맞게 숫자를 기입하고 **[ Analyze Tree ]** 를 눌러 개수 통계와 시각화를 점검합니다.
4. 최종적으로 맘에 드는 몇십 개의 오프셋 체인만 남았다면, 초록색 **[ Export Filtered to .zip ]** 버튼을 클릭하여 결과물을 다운로드 한 후 다시 치트 엔진에 로드하여 마음껏 사용하세요!
