

#ifndef CONCAL_TRACE_SCOPE_H
#define CONCAL_TRACE_SCOPE_H
#include "tracing/trace_recorder.h++"

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
        _recorder.span(_kind, _taskId, _startNs, _recorder.nowNs(), _value);
    }
};


#endif //CONCAL_TRACE_SCOPE_H
