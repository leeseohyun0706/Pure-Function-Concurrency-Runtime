
#ifndef CONCAL_WORKER_SIGNAL_H
#define CONCAL_WORKER_SIGNAL_H
#include <atomic>

class WorkerSignal {
private:
    std::atomic<uint64_t> _epoch{0};

public:
    uint64_t snapshot() const noexcept { return _epoch.load(std::memory_order_acquire); }

    void notify_one() { _epoch.fetch_add(1, std::memory_order_release); _epoch.notify_one(); }
    void notify_all() { _epoch.fetch_add(1, std::memory_order_release); _epoch.notify_all(); }

    void wait(uint64_t old) const { _epoch.wait(old, std::memory_order_acquire); }
};

#endif //CONCAL_WORKER_SIGNAL_H
