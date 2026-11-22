//
// Created by 이서현 on 11/2/26.
//

#ifndef CONCAL_TASK_H
#define CONCAL_TASK_H
#include <functional>
#include "pure/input.h++"
#include "func.h++"
#include "id.h++"

using SlotType = size_t;

enum class TaskState {
    Running,
    Completed,
    Ready,
    // Queued,
    // Building,
    // Waiting,
    Fail,
};

struct ExecResult {
    enum class Status {
        Ok,
        Failed,
        Skipped
    } status{Status::Ok};
    std::exception_ptr error;
};
class Task;

// Can I utilize memoization?
struct EdgeNode {
    Task* child = nullptr;
    EdgeNode* next = nullptr;
};

/*
 * Base Abstraction Task.
 * I want this Task class not to use vtable, yet it's really hard idioms in C++ that I need to implement.
 * Otherwise, I can use a fixed type function so that every task has the same argument types and return type.
 */
class Task : public std::enable_shared_from_this<Task> {
protected:
    const TaskId _taskId;

    // Starts at 1 because of seal() guard.
    // Thus, the task cannot become ready while dependencies are still being attached.
    // Also, the task only runs correctly when pending is 0 (otherwise, it would occurs output is not ready error).
    std::atomic<uint32_t> _pending{1};
    std::atomic<EdgeNode*> _successors{nullptr};
    std::atomic<TaskState> _state{TaskState::Ready};


    std::shared_ptr<Task> _keepAlive; // The ownership of task is owned by itself and managed by pin() and unpin().

    static inline EdgeNode kClosed{}; // Static reference of last edge (to avoid using nullptr).
    static EdgeNode* closed() noexcept { return &kClosed; }

public:

    explicit Task(const TaskId taskId) : _taskId(taskId) {}

    virtual ~Task() = default;
    virtual ExecResult execute() = 0; // Execute the task function and return its value or error.


    void pin() { _keepAlive = shared_from_this(); }
    std::shared_ptr<Task> unpin() { return std::move(_keepAlive); }


    void complete(class SchedulerPolicy& scheduler);
    void depends_on(Task& parent, EdgeNode& node);
    TaskId taskId() const { return _taskId; }

    bool try_make_ready(SchedulerPolicy& scheduler);

    bool seal(SchedulerPolicy& scheduler) {
        // If there is no dependency at the time (also, the early-completed task), make ready itself to the scheduler.
        if (_pending.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            try_make_ready(scheduler);
            return true;
        }
        return false;
    }

};

// Expected input types are convertible with target types or the same types indicating OutputRef.
template<typename A, typename P>
concept InputFor = std::convertible_to<A, P> || std::same_as<std::remove_cvref_t<A>, OutputRef<P>>;


template<typename F, typename ...Args>
class PureFuncTask final : public Task {
protected: // I would ask for c++ WHAT ACTUAL WORTH TO DISTINGUISH BETWEEN PROTECTED AND PRIVATE???

    std::array<EdgeNode, sizeof...(Args)> _edges{}; // Each edge maps each input

    using R = std::invoke_result_t<F, Args...>; // Deduct a return type from FunctionType

    PureFunc<F> _function;

    // use std::decay_t for generalization.
    std::optional<std::tuple<Input<std::decay_t<Args>>...>> _inputs;

    OutputSlot<R> _outputs;

public:


    EdgeNode& edge(size_t i)  { return _edges[i]; }
    OutputSlot<R>& output() noexcept { return _outputs;}


    // A count of template arguments and a count of inputs should be the same.
    template<typename ...CallArgs>
        requires (sizeof...(CallArgs) == sizeof...(Args)) && (InputFor<CallArgs, Args> && ...)
    explicit PureFuncTask(
        const TaskId& taskId,
        PureFunc<F> function,
        CallArgs... inputs
        )
    : Task(taskId),
    _function(std::move(function)),
    _inputs(std::in_place, Input<Args>(std::forward<CallArgs>(inputs))...)
    {}

    ExecResult execute() override {
        ExecResult result;

        auto &inputs = _inputs.value();
        auto state = _state.load(std::memory_order_acquire);

        // Task only runs when its state is Ready.
        if (state != TaskState::Ready) throw std::logic_error("Task state not ready");

        // Use std::apply to grap errors from inputs
        std::apply([&](const auto&... in) {
            ((result.error = result.error ? result.error : in.error()), ...);
        }, inputs);

        if (result.error) {
            // If arguments have any errors, the task will be skipped.
            result.status = ExecResult::Status::Skipped;
            _outputs.fail(result.error);
            _state.store(TaskState::Fail, std::memory_order_release);
        }else {
            try {
                R output = std::apply(
                        [this](const auto&... input) {
                            return _function.invoke(input.resolve()...);
                        },
                        _inputs.value()
                    );

                _outputs.publish(std::move(output));
                _state.store(TaskState::Completed, std::memory_order_release);
            }catch (...) {
                // If an exception is detected in a task, it will be failed.
                result.status = ExecResult::Status::Failed;
                result.error = std::current_exception(); // Is that safe?
                _outputs.fail(result.error);
                _state.store(TaskState::Fail, std::memory_order_release);
            }
        }

        _inputs.reset(); // If it's not, inputs are holding their output refs until program end without freed.
        return result;
    }

    TaskState state() const {
        return _state.load(std::memory_order_acquire);
    }
};


#endif //CONCAL_TASK_H
