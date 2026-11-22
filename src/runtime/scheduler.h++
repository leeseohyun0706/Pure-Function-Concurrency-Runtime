
#ifndef CONCAL_SCHEDULER_H
#define CONCAL_SCHEDULER_H

#include "pure/task.h++"
#include "pure/configuration.h++"
#include "schedulerpolicy.h++"


class LoggingContext;

struct alignas(64) SchedulerStats {
    std::atomic<uint64_t> _submitted{0};
    std::atomic<uint64_t> _started{0};
    std::atomic<uint64_t> _completed{0};
    std::atomic<uint64_t> _failed{0};
};

enum class SchedulerState {
    Created,
    Started,
    Stopped
};

struct SchedulerControl {
    std::atomic<SchedulerState> _state{SchedulerState::Created};
    std::atomic<uint64_t> _workEpoch{0};
    std::atomic<uint64_t> _outstanding{0};

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
};

/*
 * Scheduler distributes tasks through worker threads.
 */
class Scheduler {
private:

    SchedulerConfiguration& _config;
    size_t _schedulerId{0};

    spdlog::logger& _logger;
    SchedulerStats _stats;

    // size_t _worker_size;
    std::vector<std::unique_ptr<class Worker>> _workers;
    SchedulerPolicy _policy;
    IdGenerator<WorkerId> _workerIdGenerator;
    LoggingContext &_loggingContext;

    // std::atomic<SchedulerState> _state{SchedulerState::Created};

    SchedulerControl _control;
    void stop_request() ;
    void join_worker() const;

public:

    explicit Scheduler(LoggingContext& loggingContext, SchedulerConfiguration& config, TracingConfiguration& tracing);
    ~Scheduler();
    void start();
    void shutdown();
    auto& getPolicy() { return _policy; }
    void exportTrace(std::ostream& out);

    void seal(Task* task);
    void admit() { _control.admit(); }

};



#endif //CONCAL_SCHEDULER_H
