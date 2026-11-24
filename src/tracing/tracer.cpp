#include "tracing/tracer.h++"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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
        {TraceEventKind::WorkerParking, "WorkerParking"}

    };
    return eventNames.at(static_cast<TraceEventKind>(kind));
}

void TraceRecorder::instant(TraceEventKind kind, TaskId taskId, uint64_t value) {
    if (!_config.enable) return;
    _sink.record(TraceEvent{
    ._ts=nowNs(), ._dur=0, ._shape=TraceEventShape::Instant, ._kind=kind,._workerId=this->_workerId,
        ._taskId=taskId, ._value=value,
    });
}

void TraceRecorder::span(TraceEventKind kind, TaskId taskId, uint64_t startNs, uint64_t endNs) {
    if (!_config.enable) return;
    _sink.record(TraceEvent{
    ._ts=startNs, ._dur=endNs - startNs, ._shape=TraceEventShape::Span, ._kind=kind,._workerId=this->_workerId,
        ._taskId=taskId, ._value=0,
    });
}

void TraceRecorder::counter(TraceEventKind kind, uint64_t value) {
    if (!_config.enable) return;
    _sink.record(TraceEvent{
    ._ts=nowNs(), ._dur=0, ._shape=TraceEventShape::Counter, ._kind=kind,._workerId=this->_workerId,
        ._taskId={0}, ._value=value,
    });
}

void TraceExporter::exportTrace(const std::span<const TraceEvent> &events, std::ostream &output) {
    json js;
    
    for (auto& event : events) {
        switch (event._shape) {
            case TraceEventShape::Instant:
                js["traceEvents"].push_back({
                    {"ts", event._ts  / 1000.0},
                    {"name", to_string(event._kind)},
                    {"ph", "I"},
                    {"pid",  1},
                    {"tid", event._workerId._value + 1},
                    {"args", {{"taskId", event._taskId._value}}}
                });
                break;
            case TraceEventShape::Counter:
                js["traceEvents"].push_back({
                    {"ts", event._ts  / 1000.0},
                    {"name", to_string(event._kind)},
                    {"ph", "C"},
                    {"pid",  1},
                    {"tid", event._workerId._value + 1},
                    {"args", {{"value", event._value}}}
                });
                break;
            case TraceEventShape::Span:
                js["traceEvents"].push_back({
                    {"ts", event._ts / 1000.0},
                    {"dur", event._dur  / 1000.0},
                    {"name", to_string(event._kind)},
                    {"ph", "X"},
                    {"pid",  1},
                    {"tid", event._workerId._value + 1},
                    {"args", {{"taskId", event._taskId._value}}}
                });
                break;
        }
    }

    output << js.dump();

}
