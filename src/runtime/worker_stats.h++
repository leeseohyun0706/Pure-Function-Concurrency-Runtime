
#ifndef CONCAL_WORKER_STATS_H
#define CONCAL_WORKER_STATS_H
#include <atomic>

class alignas(64) WorkerStats {
    private:
    std::atomic<uint64_t> _taskExecuted{0};

    std::atomic<uint64_t> _stealAttempts{0};
    std::atomic<uint64_t> _stealSuccesses{0};

    std::atomic<uint64_t> _parkedAttempts{0};
    std::atomic<uint64_t> _waitReturns{0};
    std::atomic<uint64_t> _totalExecutionNs{0};

    public:
    struct Snapshot {
        uint64_t taskExecuted;
        uint64_t stealAttempts;
        uint64_t stealSuccesses;
        uint64_t parkedAttempts;
        uint64_t waitReturns;
        uint64_t totalExecutionNs;
    };

    void task_executed() noexcept {
        _taskExecuted.fetch_add(1, std::memory_order_relaxed);
    }

    void steal_attempted() noexcept {
        _stealAttempts.fetch_add(1, std::memory_order_relaxed);
    }

    void steal_succeeded() noexcept {
        _stealSuccesses.fetch_add(1, std::memory_order_relaxed);
    }

    void park_attempted() noexcept {
        _parkedAttempts.fetch_add(1, std::memory_order_relaxed);
    }

    void total_execution_ns(uint64_t ns) noexcept {
        _totalExecutionNs.store(ns, std::memory_order_relaxed);
    }

    Snapshot snapshot() const noexcept {
        return {
            _taskExecuted.load(std::memory_order_relaxed),
            _stealAttempts.load(std::memory_order_relaxed),
            _stealSuccesses.load(std::memory_order_relaxed),
            _parkedAttempts.load(std::memory_order_relaxed),
            _waitReturns.load(std::memory_order_relaxed),
            _totalExecutionNs.load(std::memory_order_relaxed),
        };
    }

};


#endif //CONCAL_WORKER_STATS_H
