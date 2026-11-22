

#ifndef CONCAL_OUTPUT_REF_H
#define CONCAL_OUTPUT_REF_H
#include <memory>
#include "pure/detail/output_slot.h++"


template<typename T>
class OutputRef {
private:
    //
    std::shared_ptr<OutputSlot<T>> _slot;
    // Task does not need to be owned by OutputRef (It's managed on runtime)
    std::shared_ptr<class Task> _task;
public:
    using Slot_t = T;
    explicit OutputRef(std::shared_ptr<OutputSlot<T>> slot, std::shared_ptr<class Task> task)
    : _slot(std::move(slot)), _task(std::move(task)) {}

    std::shared_ptr<const T> wait_and_get() const {
        _slot->wait();

        return {
            _slot,
            std::addressof(_slot->read_ready())
        };
    }

    auto getSlot() { return _slot; }
    OutputSlot<T>* getOutputSlot() const { return _slot.get(); }

    const T& read_ready() const {
        return _slot->read_ready();
    }

    Task& getTask() const { return *_task; }

    void wait() { return _slot->wait(); }
};


#endif //CONCAL_OUTPUT_REF_H
