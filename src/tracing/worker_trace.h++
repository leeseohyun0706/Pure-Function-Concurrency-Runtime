

#ifndef CONCAL_WORKER_TRACE_H
#define CONCAL_WORKER_TRACE_H


#include "concurrency/spsc_queue.h++"
#include <pure/configuration.h++>
#include <pure/id.h++>
#include "tracing/trace_recorder.h++"
#include "tracing/trace_scope.h++"


class WorkerTrace {
private:
    WorkerRecordBuffer _buffer;
    TraceRecorder _recorder;

public:
    // explicit WorkerTrace(TraceRecorder& recorder) : _recorder(recorder) {}
    explicit WorkerTrace(TracingConfiguration& config, WorkerId workerId, TraceRecorder::Clock::time_point origin) :_buffer(config.buffer_size),
    _recorder(_buffer, workerId, origin, config) {
    }


    void task_started(TaskId id) {
        _recorder.instant(TraceEventKind::TaskStarted, id);
    }
    void task_finished(TaskId id) {
        _recorder.instant(TraceEventKind::TaskFinished, id);
    }

    void steal_attempted() {
        _recorder.instant(TraceEventKind::StealAttempted);
    }

    void steal_successed(TaskId id) {
        _recorder.instant(TraceEventKind::StealSucceed, id);
    }

    void steal_failed() {
        _recorder.instant(TraceEventKind::StealFailed);
    }

    TraceScope task_execution(TaskId id) {
        return TraceScope{
            _recorder,
            TraceEventKind::TaskExecution,
            id};
    }

    TraceScope parking(uint64_t epoch) {
        return TraceScope{
            _recorder,
            TraceEventKind::WorkerParking,
            {0}, epoch};
    }

    auto events() const { return _buffer.events(); }

};

#endif //CONCAL_WORKER_TRACE_H
