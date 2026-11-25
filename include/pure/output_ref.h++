

#ifndef CONCAL_OUTPUT_REF_H
#define CONCAL_OUTPUT_REF_H
#include <memory>
#include "pure/detail/output_slot.h++"

class Task;

template<typename T>
class OutputRef {
private:
    using Slot = OutputSlot<T>;

    std::shared_ptr<const Slot> _slot;
    // Task does not need to be owned by OutputRef (It's managed on runtime)
    std::shared_ptr<Task> _task;

    friend class Runtime;
    template<typename U>
    friend class Input;
    template<typename F, typename ...Args>
    friend class PureFuncTask;

    auto slot() const { return _slot; }
    Task& get_task() const { return *_task; }

public:

    explicit OutputRef(std::shared_ptr<const Slot> slot, std::shared_ptr<Task> task)
    : _slot(std::move(slot)), _task(std::move(task)) {}

    std::shared_ptr<const T> wait_and_get() const {
        return {
            _slot,
            std::addressof(_slot->get())
        };
    }

    const T& read_ready() const {
        return _slot->read_ready();
    }

    void wait() const { return _slot->wait(); }
};


#endif //CONCAL_OUTPUT_REF_H
