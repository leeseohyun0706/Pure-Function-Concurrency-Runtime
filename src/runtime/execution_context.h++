
#ifndef CONCAL_EXECUTION_CONTEXT_H
#define CONCAL_EXECUTION_CONTEXT_H
#include "runtime/worker_signal.h++"
#include "runtime/scheduler_stats.h++"
#include "runtime/scheduler_control.h++"


struct WorkerExecutionContext {
    SchedulerStats stats;
    SchedulerControl control;
    WorkerSignal signal;
};
#endif //CONCAL_EXECUTION_CONTEXT_H
