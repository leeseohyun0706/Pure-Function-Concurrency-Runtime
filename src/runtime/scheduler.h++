
#ifndef CONCAL_SCHEDULER_H
#define CONCAL_SCHEDULER_H

#include "pure/task.h++"
#include "pure/configuration.h++"
#include "schedulerpolicy.h++"
#include "runtime/execution_context.h++"

class LoggingContext;



/*
 * Scheduler distributes tasks through worker threads.
 */
class Scheduler {
private:

    SchedulerConfiguration& _config;
    size_t _schedulerId{0};

    spdlog::logger& _logger;

    WorkerExecutionContext _executionContext;
    std::vector<std::unique_ptr<class Worker>> _workers;
    SchedulerPolicy _policy;
    IdGenerator<WorkerId> _workerIdGenerator;
    LoggingContext &_loggingContext;

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
    void admit() { _executionContext.control.admit(); }

};



#endif //CONCAL_SCHEDULER_H
