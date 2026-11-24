
#ifndef CONCAL_WORKER_H
#define CONCAL_WORKER_H

#include <spdlog/spdlog.h>
#include <deque>
#include <thread>

#include "runtime/worker_stats.h++"
#include "concurrency/chase_lev_deque.h++"
#include "pure/task.h++"
#include "tracing/worker_trace.h++"


struct WorkerExecutionContext;

enum class WorkerState {
    Running,
    Searching, // For pop from local deque or steal from other worker.
    Parked, // There is no executable task.
    Stopped
};

/*
 * Worker executes its tasks included in chase-lev deque on its own thread.
 */
class Worker {
private:
    friend class scheduler;

    WorkerConfiguration _config;

    WorkerId _workerId; // Worker Identity
    WorkerStats _stats; // Statistics
    ChaseLevDeque<Task> _deque; // Chase-lev Deque

    std::thread _thread;
    std::atomic<bool> _is_stopped{true}; // Thread Stop
    std::atomic<WorkerState> _state{WorkerState::Stopped};
    SchedulerPolicy& _policy;

    // Debugging
    WorkerTrace _tracer;
    spdlog::logger& _logger;
    WorkerExecutionContext& _context;

    std::shared_ptr<Task> find_task();
    void park(uint64_t epoch);
    void task_run(Task* task);
    void run();
public:

    WorkerId workerId() const {
        return _workerId;
    }
    explicit Worker(
        WorkerId workerId,
        SchedulerPolicy& policy,
        TraceRecorder::Clock::time_point origin,
        spdlog::logger& logger,
        WorkerConfiguration& config,
        TracingConfiguration& tracingConfig,
        WorkerExecutionContext &context
    ) :
    _config(config),
    _workerId(workerId),
    _deque(_config.initial_deque_capacity),
    _policy(policy),
    _tracer(tracingConfig, workerId, origin),
    _logger(logger),
    _context(context)
    {
    }

    // Warning: Worker::join() does not mean to wait as long as local deque is not empty; it's thread-join.
    void join();
    void wake();
    void submit(Task* task);
    void start();

    // This is only called by other threads.
    Task* try_steal(); // Try to steal from the worker.


    // Because worker thread could be still running when stop signal is received, there are two ways in scheduler policy:
    // First : Just entirely stop the worker when stop is requested, regardless of whether local deque is empty or not (SchedulerConfiguration::ShutdownMode::Cancel).
    // Second : Stop the worker after all tasks in local deque entirely popped (SchedulerConfiguration::ShutdownMode::Drain).
    // These two options are depended on the SchedulerConfiguration::shutdown_mode
    void stop_request();
    std::span<const TraceEvent> records() const;


    auto get_deque()-> ChaseLevDeque<Task>& { // Local Chase-Lev deque
        return _deque;
    }
    spdlog::logger& spdlogger() { return _logger; }
    WorkerStats::Snapshot stats() const { return _stats.snapshot(); }

    bool belongs_to(SchedulerPolicy& policy) const { return &policy == &_policy;}
};

inline std::string_view format_as(WorkerState ws) {
    switch (ws) {
        case WorkerState::Running:
            return "running";
        case WorkerState::Searching:
            return "searching";
        case WorkerState::Parked:
            return "parked";
        case WorkerState::Stopped:
            return "stopped";
        default:
            return "unknown";
    }
}


Worker* current_worker();
#endif //CONCAL_WORKER_H
