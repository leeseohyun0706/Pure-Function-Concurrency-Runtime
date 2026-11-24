
#ifndef CONCAL_SCHEDULERPOLICY_H
#define CONCAL_SCHEDULERPOLICY_H

// #include "runtime/worker.h++"
#include "pure/task.h++"
#include "concurrency/chase_lev_deque.h++"
#include "concurrency/mpmc_queue.h++"
#include <pure/configuration.h++>
#include <spdlog/spdlog.h>

class Worker;
class WorkerSignal;
class SchedulerStats;

class InjectionQueue {
private:
    MPMCQueue<Task*> _queue;

public:
    explicit InjectionQueue(size_t capacity) : _queue(capacity) {}


    bool try_push(Task* task) {
        return _queue.enqueue(task);
    }

    Task* try_pop() {
        Task* task = nullptr;
        return _queue.dequeue(task) ? task : nullptr;
    }
};

class SchedulerPolicy {
private:

    SchedulerConfiguration& _config;
    std::vector<std::unique_ptr<Worker>>& _workers;

    InjectionQueue _injectionQueue;
    spdlog::logger& _spdlogger;
    SchedulerStats& _stats;
    WorkerSignal& _signal;
    size_t transfer_batch(ChaseLevDeque<Task>& _deque);
    Task* refill_from_global(ChaseLevDeque<Task>& _deque);

public:
    explicit SchedulerPolicy(std::vector<std::unique_ptr<Worker>>& _workers, spdlog::logger& logger, SchedulerStats& stats, SchedulerConfiguration& config, WorkerSignal& signal);

    // Note that try_pop and try_steal depends on scheduler policy, separated from Scheduler itself (in order to compare and benchmark in the same scheduler).
    std::shared_ptr<Task> try_steal_from_workers(Worker& thief);
    std::shared_ptr<Task> try_find_task(ChaseLevDeque<Task> &_deque);

    void inject(Task* task);

    void enqueue_ready(Task* task);

    // Helper function to get spdlog (if the function is called by global, returns scheduler's spdlogger).
    spdlog::logger& spdlog_for_caller();

    spdlog::logger& spdlogger() { return _spdlogger; }
};
#endif //CONCAL_SCHEDULERPOLICY_H
