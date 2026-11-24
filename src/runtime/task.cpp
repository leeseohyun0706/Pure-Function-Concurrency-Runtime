#include "pure/task.h++"

#include "scheduler.h++"

// Set task as complete
void Task::complete(SchedulerPolicy &scheduler)  {
    _successors.close_and_visit(
      [&scheduler] (Task& child) {
            child.resolve_dependency(scheduler);
      }
    );
}

// Parent is a targeted input dependency that needs to connect with node.
// Thus, depends_on() connects parent with node
void Task::depends_on(Task &parent, EdgeNode& node) {

    node.child = this;

    // Add pending.
    _pending.fetch_add(1, std::memory_order_relaxed);

    // Put node indicating child into the parent's successor.

    if (!parent._successors.try_attach(node)) {
        _pending.fetch_sub(1, std::memory_order_acq_rel);
    }
}

bool Task::seal(SchedulerPolicy &scheduler) {
    resolve_dependency(scheduler);
    return true;
}

void Task::resolve_dependency(SchedulerPolicy &scheduler) {
    // If there is no dependency at the time (also, the early-completed task), make ready itself to the scheduler.
    // pending > 0 means its dependency is not yet resolved, so the pending count of condition that enqueues the task should be 0.
    if (_pending.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        scheduler.enqueue_ready(this);
    }
}
