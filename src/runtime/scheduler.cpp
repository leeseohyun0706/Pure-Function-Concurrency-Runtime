
#include "scheduler.h++"
#include "runtime/worker.h++"
#include <spdlog/stopwatch.h>
#include <pure/logging/logger.h++>
#include "tracing/trace_exporter.h++"

void Scheduler::exportTrace(std::ostream &out) {
    TraceExporter exporter;
    std::vector<TraceEvent> events;
    for (auto& worker : _workers) {
        for (auto& event: worker->records())
            events.push_back(event);
    }
    exporter.exportTrace(std::span<const TraceEvent>(events), out);
}

void Scheduler::seal(Task *task) {
    task->seal(_policy);
}


void Scheduler::start() {
    if (_executionContext.control.state() == SchedulerState::Started) {
        throw std::runtime_error("The scheduler is already started.");
    }
    for (auto& worker : _workers) {
        worker->start();
    }
    _executionContext.control.mark_started();
}

void Scheduler::shutdown() {
    if (_executionContext.control.state() == SchedulerState::Stopped) {
        throw std::runtime_error("The scheduler is already stopped");
    }

    if (_config.shutdown_mode == SchedulerConfiguration::ShutdownMode::Drain) {
        // for (auto n = _executionContext.control._outstanding.load(); n != 0; n = _executionContext.control._outstanding.load()) {
        //     _executionContext.control._outstanding.wait(n);
        // }
        _executionContext.control.wait_until_drained();
    }

    spdlog::stopwatch sw;
    stop_request();
    join_worker();

    const auto scheduler_stats = _executionContext.stats.snapshot();

    _logger.info("shutdown in {:.3}s    submitted={} complete={} skipped={} failed={}",
        sw, scheduler_stats.submitted,
        scheduler_stats.completed,
        scheduler_stats.skipped,
        scheduler_stats.failed
    );

    for (auto& w : _workers) {
        const auto s = w->stats();
        auto att = s.stealAttempts;
        auto ok = s.stealSuccesses;

        _logger.info("worker {} : executed={} steals={}/{} ({:.1f}%) parks={}",
            w->workerId(),
            s.taskExecuted,
            ok, att, att ? 100.0 * ok / att : 0.0,
            s.parkedAttempts
        );
    }
    _executionContext.control.mark_stopped();
}

void Scheduler::stop_request()  {
    for (auto& worker : _workers) {
        worker->stop_request();
    }
    _executionContext.signal.notify_all();
}

void Scheduler::join_worker() const {
    for (auto& worker : _workers) {
        worker->join();
    }
}


Scheduler::Scheduler(LoggingContext &logContext, SchedulerConfiguration &config, TracingConfiguration& tracing)
    : _config(config),
    _logger(*logContext.make("rt.sc")),
    _policy(_workers, _logger, _executionContext.stats, _config, _executionContext.signal),
    _loggingContext(logContext)
    // _executionContext(_stats, _control, _signal)
    {

    auto origin = TraceRecorder::Clock::now();

    for (size_t i = 0; i < _config.worker_size; i++) {
        const auto& w_id = _workerIdGenerator.next();
        _workers.push_back(std::make_unique<Worker>(
            w_id, _policy, origin,
            *_loggingContext.make("rt.sc.wo" + std::to_string(w_id._value)),
            _config.worker, tracing, _executionContext));
    }
}

Scheduler::~Scheduler() {
    if (_executionContext.control.state() != SchedulerState::Stopped) shutdown();
}
