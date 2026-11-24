
#ifndef CONCAL_TRACE_RECORDER_H
#define CONCAL_TRACE_RECORDER_H
#include <chrono>

#include "trace_event.h++"
#include "concurrency/spsc_queue.h++"
#include <pure/configuration.h++>

using WorkerRecordBuffer = EventSink<TraceEvent>;


class TraceRecorder {
public:
    using Clock = std::chrono::steady_clock;
private:
    WorkerRecordBuffer& _sink;
    Clock::time_point _origin;
    WorkerId _workerId;

    TracingConfiguration &_config;

    void emit(
        TraceEventShape shape,
        TraceEventKind kind,
        TaskId taskId,
        uint64_t timestamp,
        uint64_t duration = 0,
        uint64_t value = 0
    ) {
        _sink.record(TraceEvent{
            ._ts = timestamp,
            ._dur = duration,
            ._shape = shape,
            ._kind = kind,
            ._workerId = _workerId,
            ._taskId = taskId,
            ._value = value
        });
    }

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

    bool enabled() const noexcept { return _config.enable; }
};

#endif //CONCAL_TRACE_RECORDER_H
