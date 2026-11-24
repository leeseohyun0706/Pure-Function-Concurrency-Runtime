
#ifndef CONCAL_TRACE_EVENT_H
#define CONCAL_TRACE_EVENT_H
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

#endif //CONCAL_TRACE_EVENT_H
