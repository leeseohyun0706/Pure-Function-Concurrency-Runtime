
#ifndef CONCAL_SCHEDULER_STATS_H
#define CONCAL_SCHEDULER_STATS_H
#include <atomic>

class alignas(64) SchedulerStats {
    private:
    std::atomic<uint64_t> _submitted{0};
    std::atomic<uint64_t> _started{0};
    std::atomic<uint64_t> _completed{0};
    std::atomic<uint64_t> _skipped{0};
    std::atomic<uint64_t> _failed{0};
    public:

    struct Snapshot {
        uint64_t submitted;
        uint64_t started;
        uint64_t completed;
        uint64_t skipped;
        uint64_t failed;
    };


    void submitted() {
        _submitted.fetch_add(1, std::memory_order_relaxed);
    }
    void completed() {
        _completed.fetch_add(1, std::memory_order_relaxed);
    }
    void started() {
        _started.fetch_add(1, std::memory_order_relaxed);
    }
    void failed() {
        _failed.fetch_add(1, std::memory_order_relaxed);
    }
    void skipped() {
        _skipped.fetch_add(1, std::memory_order_relaxed);
    }

    Snapshot snapshot() const {
        return Snapshot{
            _submitted.load(std::memory_order_relaxed),
            _started.load(std::memory_order_relaxed),
            _completed.load(std::memory_order_relaxed),
            _skipped.load(std::memory_order_relaxed),
            _failed.load(std::memory_order_relaxed)
        };
    }
};

#endif //CONCAL_SCHEDULER_STATS_H
