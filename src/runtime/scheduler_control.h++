
#ifndef CONCAL_SCHEDULER_CONTROL_H
#define CONCAL_SCHEDULER_CONTROL_H
#include <atomic>
#include <stdexcept>

enum class SchedulerState {
    Created,
    Started,
    Stopped
};

class SchedulerControl {
private:
    std::atomic<SchedulerState> _state{SchedulerState::Created};
    std::atomic<uint64_t> _outstanding{0};
public:

    void admit() {
        _outstanding.fetch_add(1);
        auto s = _state.load();
        if (s == SchedulerState::Stopped) {
            retire();
            throw std::runtime_error("Scheduler is stopped");
        }
    }

    void retire() {
        if (_outstanding.fetch_sub(1) == 1)
            _outstanding.notify_all();
    }

    void wait_until_drained() const {
        auto n = _outstanding.load();

        while (n != 0) {
            _outstanding.wait(n);
            n = _outstanding.load();
        }
    }

    SchedulerState state() const { return _state.load(std::memory_order_acquire); }

    void mark_started() { _state.store(SchedulerState::Started, std::memory_order_release); }
    void mark_stopped() { _state.store(SchedulerState::Stopped, std::memory_order_release); }
};

#endif //CONCAL_SCHEDULER_CONTROL_H
