#include "schedulerpolicy.h++"
#include "runtime/worker.h++"
#include <pure/logging/logger.h++>
#include "scheduler.h++"

SchedulerPolicy::SchedulerPolicy(SchedulerControl& control, std::vector<std::unique_ptr<Worker>>& workers, spdlog::logger& logger, SchedulerStats& stats,  SchedulerConfiguration& config)
    : _config(config),
    _workers(workers),
    _globalInjectionQueue2(1024 * 64),
    _spdlogger(logger),
    _stats(stats),
    _control(control)
{
}

Task * SchedulerPolicy::try_pop(ChaseLevDeque<Task> &_deque)  {
    if (auto* task = _deque.pop()) {
        return task;
    }
    return nullptr;
}


std::shared_ptr<Task> SchedulerPolicy::try_steal() {
    // TODO : Create other work-stealing logics (Currently, searching all workers)

    if (!_config.work_stealing) return nullptr; // Is there any more awesome way to create configuration branch?

    auto self = current_worker();
    size_t n = _workers.size();
    size_t start = self ? self->workerId()._value : 0; // Start with current index

    for (size_t i = 0; i < n; ++i) {
        // let start = 2, n = 4
        // when i = 0, victim = 2,
        // when i = 1, victim = 3,
        // when i = 2, victim = 0,
        // when i = 3, victim = 1.
        Worker* victim = _workers[(start + i) % n].get();
        if (victim == self) continue;
        if (auto* task = victim->try_steal()) {
            return task->unpin();
        }
    }

    return nullptr;
}

Task* SchedulerPolicy::try_steal_from_global_queue(ChaseLevDeque<Task> &_deque) {

    Task* first{nullptr};
    _globalInjectionQueue2.dequeue(first);
    // If global queue be empty.
    if (!first) return nullptr;

    size_t r = 0; // tasks local deque pushed

    // Takes batch size amounts of tasks from globalQueue and push them to local deque.
    for (size_t i = 1; i < _config.global_queue_batch_size; i++) {
        Task* task{nullptr};
        _globalInjectionQueue2.dequeue(task);
        if (!task) break;

        if (!_deque.push(task)) {
            // Going back to global queue.
            if (_globalInjectionQueue2.enqueue(task)) {
            _spdlogger.error("global injection queue is full; task {} was not enqueued again.", task->taskId());
                throw std::runtime_error("global injection queue is full");
            }
            break;
        }
        ++r;
    }

    if (r > 0)
        wake_one();
    return first;
}

std::shared_ptr<Task> SchedulerPolicy::try_find_task(ChaseLevDeque<Task> &_deque) {
    // First, tries to pop from local thread deque.
    if (auto* task = try_pop(_deque)) {
        return task->unpin();
    }
    // Second, tries to steal from global queue.
    if (auto* task = try_steal_from_global_queue(_deque))
        return task->unpin();
    return nullptr;
}

void SchedulerPolicy::wake_one() {
    _control._workEpoch.fetch_add(1);
    // vs notify_one() ?????
    // _workEpoch.notify_all();
    _control._workEpoch.notify_one();
}

void SchedulerPolicy::wake_all() {
    _control._workEpoch.fetch_add(1);
    _control._workEpoch.notify_all();
}

void SchedulerPolicy::wait(size_t old) const {
    _control._workEpoch.wait(old);
}

void SchedulerPolicy::submit(Task *task) {
    if (!_globalInjectionQueue2.enqueue(task)) {
        _spdlogger.error("global injection queue is full; task {} was not enqueued.", task->taskId());
        throw std::runtime_error("global injection queue is full");
    }
    wake_one();
}

void SchedulerPolicy::enqueue_ready(Task *task) {
    if (auto*w = current_worker()) {
        w->submit(task);
    }
    else submit(task);

    _stats._submitted.fetch_add(1, std::memory_order_relaxed);
}

spdlog::logger & SchedulerPolicy::spdlog_for_caller() {
    if (auto* w = current_worker()) return w->spdlogger();
    return _spdlogger;
}

size_t SchedulerPolicy::workEpoch() const {
    return _control._workEpoch.load();
}

void SchedulerPolicy::admit() {
    _control.admit();
}

void SchedulerPolicy::retire() {
    _control.retire();
}
