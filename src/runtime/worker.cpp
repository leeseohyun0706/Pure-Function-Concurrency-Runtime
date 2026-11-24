#include "worker.h++"
#include "schedulerpolicy.h++"
#include <pure/logging/logger.h++>
#include <spdlog/stopwatch.h>

#include "scheduler.h++"

thread_local Worker* tls_worker = nullptr;

// Return local worker indicating called thread
Worker* current_worker() {
    return tls_worker;
}


// Return string explanation of given exception pointer
std::string describe(const std::exception_ptr& error) {
    try { std::rethrow_exception(error); }
    catch (const std::exception& e) { return e.what(); }
    catch (...) { return "unknown exception"; }
}

// Run given task and handle error from that task.
void Worker::task_run(Task *task) {
    _state.store(WorkerState::Running, std::memory_order_release);
    ExecResult result;

    _tracer.task_started(task->taskId());
    {
        auto scope = _tracer.task_execution(task->taskId());
        result = task->execute();
    }

    switch (result.status) {
        case ExecResult::Status::Ok:
            _context.stats.completed();
            break;
        case ExecResult::Status::Failed :
            _logger.error("task {} threw : {}", task->taskId(), describe(result.error));
            _context.stats.failed();
            break;
        case ExecResult::Status::Skipped :
            _logger.error("task {} skipped: parents failed", task->taskId());
            _context.stats.skipped();
            break;
    }
    task->complete(_policy);
    _context.control.retire();
    _tracer.task_finished(task->taskId());
    _stats.task_executed();
}

std::shared_ptr<Task> Worker::find_task() {
    // Try pop from local queue and global queue
    if (auto task = _policy.try_find_task(_deque)) {
        return task;
    }

    _stats.steal_attempted();
    _tracer.steal_attempted();
    // Try to steal from another deque by SchedulerPolicy
    if (auto task = _policy.try_steal_from_workers(*this)) {
        _stats.steal_succeeded();
        _tracer.steal_successed(task->taskId());
        return task;
    }
    _tracer.steal_failed();
    return nullptr;
}

void Worker::park(uint64_t epoch) {
    _stats.park_attempted();
    _state.store(WorkerState::Parked, std::memory_order_release);

    // Run Stolen Task
    {
        auto scope = _tracer.parking(epoch);
        // wait(observed) makes a current thread block as long as _wakeEpoch is the same as observed.
        // What if this part be spin-lock?
        _context.signal.wait(epoch);
    }

}

/*
 * Current Worker State Changes
 *
 * Worker::Run() -> Set Searching -> Try Find Task -> Try Steal -> Set Parked
 *
 */
void Worker::run() {
        tls_worker = this;

        SPDLOG_LOGGER_DEBUG(&_logger, "thread started");
        spdlog::stopwatch sw;
        // As long as atomic variable _is_stopped is false (Use acquire to read released value).
        while (true) {
            // Store wakeEpoch because it could change (new wakeup) during trying pop and steal.
            const auto observed = _context.signal.snapshot();

            _state.store(WorkerState::Searching, std::memory_order_release);

            if (auto task = find_task()) {
                task_run(task.get());
                continue;
            }

            if (_is_stopped.load(std::memory_order_acquire))
                break;

            park(observed);
        }

        auto exec_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(sw.elapsed());
        _stats.total_execution_ns(exec_ns.count());
        SPDLOG_LOGGER_DEBUG(&_logger,
            "thread exiting, executed={} exec_time={}ms",
            _stats.snapshot().taskExecuted, sw.elapsed_ms().count());
        tls_worker = nullptr;
}

void Worker::join() {
    if (_thread.joinable()) {
        _thread.join();
    }
}

void Worker::wake() {
    _context.signal.notify_one();
    // _recorder.instant(TraceEventKind::WakeSignal, {0}, 0);
}

void Worker::submit(Task *task) {
    if (!task) return;
    // _recorder.instant(TraceEventKind::TaskSubmitted, task->taskId(), 0);
    _deque.push(task);
    wake();
}


void Worker::start() {
    // Store false in variable _is_stopped with release.
    _is_stopped.store(false, std::memory_order_release);
    // When start() executes, creates new thread executing Worker::run.
    _thread = std::thread(&Worker::run, this);
}

void Worker::stop_request() {
    _state.store(WorkerState::Stopped, std::memory_order_release);
    _is_stopped.store(true, std::memory_order_release);
    wake();
}

Task* Worker::try_steal() {
    return _deque.steal();
}

std::span<const TraceEvent> Worker::records() const {
    // Because WorkerRecordBuffer uses std::vector, it doesn't ensure lock-free or mutex something. (Edited)
    if (!_is_stopped.load(std::memory_order_acquire)) {
        throw std::runtime_error("Worker::records() called on running state");
    }
    return _tracer.events();
}

