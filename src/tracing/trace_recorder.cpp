
#include "trace_recorder.h++"


std::string to_string(TraceEventKind kind) {
    static const std::unordered_map<TraceEventKind, std::string> eventNames = {
        {TraceEventKind::TaskSubmitted, "TaskSubmitted"},
        {TraceEventKind::TaskStarted, "TaskStarted"},
        {TraceEventKind::TaskFinished, "TaskFinished"},
        {TraceEventKind::StealAttempted, "StealAttempted"},
        {TraceEventKind::StealSucceed, "StealSucceed"},
        {TraceEventKind::StealFailed, "StealFailed"},
        {TraceEventKind::WakeSignal, "WakeSignal"},
        {TraceEventKind::TaskExecution, "TaskExecution"},
        {TraceEventKind::WorkerWaitEntered, "WorkerWaitEntered"},
        {TraceEventKind::WorkerWaitExited, "WorkerWaitExited"},
        {TraceEventKind::WorkerParking, "WorkerParking"},
        {TraceEventKind::TaskEnqueued, "TaskEnqueued"},

    };
    return eventNames.at(static_cast<TraceEventKind>(kind));
}

void TraceRecorder::instant(TraceEventKind kind, TaskId taskId, uint64_t value) {
    if (!enabled()) return;
    emit(TraceEventShape::Instant, kind, taskId, nowNs(), 0, value);
}

void TraceRecorder::span(TraceEventKind kind, TaskId taskId, uint64_t startNs, uint64_t endNs, uint64_t value) {
    if (!enabled()) return;
    emit(TraceEventShape::Span, kind, taskId, startNs, endNs - startNs, value);
}

void TraceRecorder::counter(TraceEventKind kind, uint64_t value) {
    if (!enabled()) return;
    emit(TraceEventShape::Counter, kind, {0}, nowNs(), 0, value);
}
