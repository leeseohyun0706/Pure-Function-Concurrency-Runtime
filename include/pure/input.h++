

#ifndef CONCAL_INPUT_H
#define CONCAL_INPUT_H
#include <variant>
#include "pure/output_ref.h++"

template<typename T>
class Input {
private:
    using SlotRef = std::shared_ptr<const OutputSlot<T>>;
    using Source = std::variant<T, SlotRef>;

    Source _source;

public:
    explicit Input(OutputRef<T> source) : _source(source.slot()) { }
    explicit Input(SlotRef source) : _source(std::move(source)) {}
    explicit Input(const T& source) : _source(source) {}

    // If the source is SlotRef, return its value by using read_ready() (because task dependency is managed by DAG, it is not required to wait until ready when the worker tries to resolve the input).
    // Otherwise, return the source.
    T resolve() const {
        if (auto* val = std::get_if<SlotRef>(&_source)) return (*val)->read_ready(); // read_ready() is only once read (whereas OutputSlot::get() wait and read until it's ready, possibly deadlocked).
        return std::get<T>(_source);
    }

    // Return an error if the source is SlotRef type and contains an error, otherwise nullptr.
    std::exception_ptr error() const {
        if (auto* slot = std::get_if<SlotRef>(&_source)) return (*slot)->error();
        return nullptr;
    }
};

#endif //CONCAL_INPUT_H
