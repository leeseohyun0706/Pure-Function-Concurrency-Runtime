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
    Task* child = nullptr; // Vertex
    EdgeNode* next = nullptr; // Link
};

// Single Lock-Free Linked List to manage input dependency
class SuccessorList {
private:
    static inline EdgeNode _closed{}; // Static reference of last edge (to avoid using nullptr).

    std::atomic<EdgeNode*> _head{nullptr};

    static EdgeNode* closed() noexcept { return &_closed; }

public:
    SuccessorList() = default;
    SuccessorList(const SuccessorList&) = delete;
    SuccessorList& operator=(const SuccessorList&) = delete;

    bool try_attach(EdgeNode& node) {
        auto* head = _head.load(std::memory_order_acquire);

        while (head != closed()) {
            node.next = head;

            if (_head.compare_exchange_weak(head, &node, std::memory_order_acq_rel, std::memory_order_acquire)) return true;
        }

        return false;
    }

    template<typename Visitor> requires std::is_invocable_v<Visitor&, Task&>
    bool close_and_visit(Visitor&& visit) {

        auto *node = _head.exchange(closed(), std::memory_order_acq_rel);

        if (node == closed()) return false;

        while (node != nullptr) {
            auto* next = node->next;
            auto* child = node->child;

            visit(*child);

            node = next;
        }
        return true;
    }
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
    // Also, the task only runs correctly when pending is 0 (otherwise, it would occur output is not ready error).
    std::atomic<uint32_t> _pending{1};
    std::atomic<TaskState> _state{TaskState::Ready};


    std::shared_ptr<Task> _keepAlive; // The ownership of task is owned by itself and managed by pin() and unpin().

    SuccessorList _successors;
public:

    explicit Task(const TaskId taskId) : _taskId(taskId) {}

    virtual ~Task() = default;
    virtual ExecResult execute() = 0; // Execute the task function and return its value or error.

    // By using pin() and unpin(), this task owns itself so that it maintains its ownership until call unpin().
    void pin() { _keepAlive = shared_from_this(); }
    std::shared_ptr<Task> unpin() { return std::move(_keepAlive); }


    void complete(class SchedulerPolicy& scheduler);
    void depends_on(Task& parent, EdgeNode& node);
    TaskId taskId() const { return _taskId; }

    bool seal(SchedulerPolicy& scheduler);

    void resolve_dependency(SchedulerPolicy& scheduler);

};

// Expected input types are convertible with target types or the same types indicating OutputRef.
template<typename A, typename P>
concept InputFor = std::convertible_to<A, P> || std::same_as<std::remove_cvref_t<A>, OutputRef<P>>;



template<typename F, typename ...Args>
class PureFuncTask final : public Task {
protected: // I would ask for c++ WHAT ACTUAL WORTH TO DISTINGUISH BETWEEN PROTECTED AND PRIVATE??? (IT ALWAYS MAKES ME CONFUSE)

    using R = std::invoke_result_t<F, Args...>; // Deduct a return type from FunctionType


    std::array<EdgeNode, sizeof...(Args)> _edges{}; // Each edge maps by each argument of the function

    PureFunc<F> _function;

    // use std::decay_t for generalization.
    std::optional<std::tuple<Input<std::decay_t<Args>>...>> _inputs;

    OutputSlot<R> _outputs;

    ExecResult finish_skipped(const std::exception_ptr &error) {
        ExecResult result{ExecResult::Status::Skipped, error};
        _outputs.fail(result.error);
        _state.store(TaskState::Fail, std::memory_order_release);
        return result;
    }

    ExecResult finish_failed(const std::exception_ptr &error) {
        ExecResult result{ExecResult::Status::Failed, error};
        _outputs.fail(result.error);
        _state.store(TaskState::Fail, std::memory_order_release);
        return result;
    }

    std::exception_ptr input_error() {
        std::exception_ptr result{nullptr};
        // Use std::apply to grap errors from inputs
        if (_inputs.has_value())
            std::apply([&](const auto&... in) {
                ((result = result ? result : in.error()), ...);
            }, _inputs.value());

        return result;
    }
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

        if (const auto error = input_error()) {
            // If arguments have any errors, the task will be skipped.
            return finish_skipped(error);
        }
        try {
            R output = std::apply(
                [this](const auto&... input) {
                    return _function.invoke(input.resolve()...);
                },
                _inputs.value()
            );

            _outputs.publish(std::move(output));
            _state.store(TaskState::Completed, std::memory_order_release);
            _inputs.reset(); // If it's not, inputs are holding their output refs until program end without freed.
            return result;
        }
        catch (...) {
            // If an exception is detected in a task, it will be failed.
            return finish_failed(std::current_exception());
        }
    }

    TaskState state() const {
        return _state.load(std::memory_order_acquire);
    }
};


#endif //CONCAL_TASK_H
