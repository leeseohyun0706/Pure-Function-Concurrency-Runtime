//
// Created by 이서현 on 11/4/26.
//

#ifndef CONCAL_TRACER_H
#define CONCAL_TRACER_H
#include <chrono>
#include <span>
#include <sstream>

#include "concurrency/spsc_queue.h++"
#include <pure/configuration.h++>
#include <pure/id.h++>

// Event Shape in which be expressed.
enum class TraceEventShape : uint8_t {
    Instant, // Instant Event (Ex. Steal Success, Wake Signal)
    Span, // Duration (Ex. Task Duration, Finding Time)
    Counter // Counted
};

enum class TraceEventKind : uint8_t {
    TaskSubmitted,
    TaskEnqueued,

    TaskStarted,
    TaskExecution,
    TaskFinished,

    StealAttempted,
    StealSucceed,
    StealFailed,

    WorkerParking,
    WorkerWaitEntered,
    WorkerWaitExited,

    WakeSignal,
    End,
};

std::string to_string(TraceEventKind kind);

struct TraceEvent {
    uint64_t _ts{};
    uint64_t _dur{}; // Duration

    TraceEventShape _shape;
    TraceEventKind _kind;

    WorkerId _workerId;
    TaskId _taskId;

    uint64_t _value{}; // For only mvp (Basic value storage as pointer size)

};

class TraceSink {
public:
    virtual ~TraceSink() = default;

    virtual void record(const TraceEvent& event) = 0;
};

using WorkerRecordBuffer = EventSink<TraceEvent>;

class TraceRecorder {
public:
    using Clock = std::chrono::steady_clock;
private:
    WorkerRecordBuffer& _sink;
    Clock::time_point _origin;
    WorkerId _workerId;

    TracingConfiguration &_config;
public:

    TraceRecorder(
        WorkerRecordBuffer& sink,
        WorkerId workerId,
        Clock::time_point origin,
        TracingConfiguration & config
        )
    : _sink(sink), _origin(origin), _workerId(workerId), _config(config) {}

    uint64_t nowNs() const {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - _origin).count());
    }

    void instant(
        TraceEventKind kind,
        TaskId taskId = {0},
        uint64_t value = 0);

    void span(TraceEventKind kind, TaskId taskId, uint64_t startNs, uint64_t endNs);

    void counter(TraceEventKind kind, uint64_t value);

};

class TraceScope {
private:
    TraceRecorder& _recorder;
    TraceEventKind _kind;
    TaskId _taskId;
    uint64_t _value;
    uint64_t _startNs;

public:
    TraceScope(TraceRecorder& recorder, TraceEventKind kind, TaskId taskId = {0}, uint64_t value = 0)
    : _recorder(recorder), _kind(kind), _taskId(taskId), _value(value), _startNs(_recorder.nowNs()) {}

    TraceScope(const TraceScope&) = delete;
    TraceScope(TraceScope&&) = delete;
    TraceScope& operator=(const TraceScope&) = delete;
    TraceScope& operator=(TraceScope&&) = delete;
    ~TraceScope() {
        _recorder.span(_kind, _taskId, _startNs, _recorder.nowNs());
    }
};

class TraceExporter {
public:
    // Only for perfetto web UI
    // TODO : Separate perfetto export and common json export.
    void exportTrace(
        const std::span<const TraceEvent>& events,
        std::ostream& output);

};

#endif //CONCAL_TRACER_H
