// IDetectionModule.h
// 안티치트 클라이언트 공통 탐지 모듈 인터페이스 + 단일 이벤트 큐.
//
// 설계 근거 (9~10주차 보고서):
//  - 모든 탐지 모듈은 IDetectionModule 로 추상화하고 결과를 단일
//    DetectionEventQueue 로 송신한다.
//  - 폴링 기반 모듈(메모리 무결성, 프로세스 감시)과 이벤트 트리거 기반
//    모듈(디버거/인젝션 탐지)의 실행 주기를 스케줄러 레벨에서 분리한다.
#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>

namespace ac {

// 탐지 심각도. 서버 측 정책 엔진이 가중치/임계값 계산에 사용.
enum class Severity {
    Info,
    Low,
    Medium,
    High,
    Critical
};

// 모듈 실행 방식. 스케줄러가 폴링/이벤트를 분리 처리.
enum class ModuleKind {
    Polling,      // 주기적으로 Tick() 호출 (메모리 무결성, 프로세스 감시)
    EventTrigger  // 후킹/콜백에서 동작 (디버거/인젝션 탐지)
};

// 단일 탐지 이벤트. 모듈 -> 큐 -> (소비 스레드 / 서버 송신 레이어).
struct DetectionEvent {
    std::string moduleName;        // 예: "ProcessMonitor"
    std::string detectionType;     // 예: "ProcessBlacklist.Name"
    Severity    severity = Severity::Info;
    std::string description;       // 사람이 읽는 설명
    std::string evidence;          // 근거 (pid, 이미지명, 해시 등)
    uint64_t    timestampMs = 0;   // system_clock epoch ms
};

// 모든 탐지 모듈이 결과를 보내는 단일 큐 (thread-safe).
// 폴링/이벤트 모듈이 서로 다른 스레드에서 Push 해도 안전하도록 잠금 처리.
class DetectionEventQueue {
public:
    void Push(DetectionEvent ev) {
        std::lock_guard<std::mutex> lock(mtx_);
        q_.push(std::move(ev));
    }

    bool Pop(DetectionEvent& out) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (q_.empty()) return false;
        out = std::move(q_.front());
        q_.pop();
        return true;
    }

    std::size_t Size() {
        std::lock_guard<std::mutex> lock(mtx_);
        return q_.size();
    }

private:
    std::mutex mtx_;
    std::queue<DetectionEvent> q_;
};

// 공통 탐지 모듈 인터페이스.
class IDetectionModule {
public:
    virtual ~IDetectionModule() = default;

    // 모듈 초기화. 실패 시 false. queue 는 비소유 포인터.
    virtual bool Initialize(DetectionEventQueue* queue) = 0;

    // 폴링 모듈: 스케줄러가 PollIntervalMs() 주기로 호출.
    // 이벤트 모듈: 후킹/콜백에서 처리하고 여기선 no-op 가능.
    virtual void Tick() = 0;

    // 정리 (핸들 해제, 후킹 해제 등).
    virtual void Shutdown() = 0;

    virtual const char* Name() const = 0;
    virtual ModuleKind  Kind() const = 0;
    virtual uint32_t    PollIntervalMs() const { return 1000; }
};

// 스케줄러 타이밍용 단조 시계 (ms). 절대 시각이 아님.
inline uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

} // namespace ac
