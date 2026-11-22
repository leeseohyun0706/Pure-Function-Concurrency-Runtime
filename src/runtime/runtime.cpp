#include "pure/runtime.h++"
#include "runtime/scheduler.h++"

Runtime::Runtime(const RuntimeConfiguration &config)
    : _runtimeConfig(config),
    _loggingContext(config.logging),
    _scheduler(std::make_unique<Scheduler>(_loggingContext, _runtimeConfig.scheduler, _runtimeConfig.tracing))
{
    _logger = _loggingContext.make("rt");
}

void Runtime::start()  {
    const auto& s = _runtimeConfig.scheduler;
    _logger->info("starting: workers={} stealing={} batch={} tracing={} (buf={})",
               s.worker_size, s.work_stealing,
               s.global_queue_batch_size,
               _runtimeConfig.tracing.enable, _runtimeConfig.tracing.buffer_size);
    _scheduler->start();
}

void Runtime::shutdown() {
    _scheduler->shutdown();
    _logger->info("safety-shutdown completed");
    _loggingContext.flush();
}

Runtime::~Runtime() = default;

void Runtime::seal(Task &task) {
    // task.seal(_scheduler->getPolicy());
    _scheduler->seal(&task);
}

void Runtime::exportTrace(std::ostream &os) {
    _scheduler->exportTrace(os);
}

void Runtime::admit() {
    _scheduler->admit();
}
