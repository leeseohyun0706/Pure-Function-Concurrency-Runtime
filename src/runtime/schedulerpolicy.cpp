#include "schedulerpolicy.h++"
#include "runtime/worker.h++"
#include <pure/logging/logger.h++>
#include "scheduler.h++"

SchedulerPolicy::SchedulerPolicy(std::vector<std::unique_ptr<Worker>>& workers, spdlog::logger& logger, SchedulerStats& stats,  SchedulerConfiguration& config, WorkerSignal& signal)
    : _config(config),
    _workers(workers),
    _injectionQueue(1024 * 64),
    _spdlogger(logger),
    _stats(stats),
    _signal(signal)
{
}

// Takes batch size amounts of tasks from globalQueue and push them to local deque.
size_t SchedulerPolicy::transfer_batch(ChaseLevDeque<Task> &_deque) {
    size_t r = 0;
    for (size_t i = 1; i < _config.global_queue_batch_size; i++) {
        auto* task = _injectionQueue.try_pop();
        if (!task) break;

        if (!_deque.push(task)) {
            // Going back to global queue.
            if (!_injectionQueue.try_push(task)) {
                _spdlogger.error("global injection queue is full; task {} was not enqueued again.", task->taskId());
                throw std::runtime_error("global injection queue is full");
            }
            break;
        }
        ++r;
    }
    return r;
}



std::shared_ptr<Task> SchedulerPolicy::try_steal_from_workers(Worker& thief) {
    // TODO : Create other work-stealing logics (Currently, searching all workers)

    if (!_config.work_stealing) return nullptr; // Is there any more awesome way to create configuration branch?

    size_t n = _workers.size();
    size_t start = thief.workerId()._value; // Start with current index

    for (size_t i = 0; i < n; ++i) {
        // let start = 2, n = 4
        // when i = 0, victim = 2,
        // when i = 1, victim = 3,
        // when i = 2, victim = 0,
        // when i = 3, victim = 1.
        auto& victim = *_workers[(start + i) % n];
        if (&victim == &thief) continue;
        if (auto* task = victim.try_steal()) {
            return task->unpin();
        }
    }

    return nullptr;
}

Task* SchedulerPolicy::refill_from_global(ChaseLevDeque<Task> &_deque) {

    Task* first = _injectionQueue.try_pop();
    // If global queue is empty.
    if (!first) return nullptr;

    if (transfer_batch(_deque) > 0) _signal.notify_one();
    return first;
}

std::shared_ptr<Task> SchedulerPolicy::try_find_task(ChaseLevDeque<Task> &_deque) {
    // First, tries to pop from local thread deque.
    if (auto* task = _deque.pop()) {
        return task->unpin();
    }
    // Second, tries to steal from global queue.
    if (auto* task = refill_from_global(_deque))
        return task->unpin();
    return nullptr;
}


void SchedulerPolicy::inject(Task *task) {
    if (!_injectionQueue.try_push(task)) {
        _spdlogger.error("global injection queue is full; task {} was not enqueued.", task->taskId());
        throw std::runtime_error("global injection queue is full");
    }
    _signal.notify_one();
}

void SchedulerPolicy::enqueue_ready(Task *task) {
    if (auto*w = current_worker(); w &&  w->belongs_to(*this)) {
        w->submit(task);
    }
    else inject(task);

    _stats.submitted();
}

spdlog::logger & SchedulerPolicy::spdlog_for_caller() {
    if (auto* w = current_worker()) return w->spdlogger();
    return _spdlogger;
}
