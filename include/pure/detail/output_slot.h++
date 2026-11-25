
#ifndef CONCAL_OUTPUT_SLOT_H
#define CONCAL_OUTPUT_SLOT_H
#include <memory>
#include <exception>
#include <atomic>

enum class ResultState {
    Empty,
    Ready,
    Failed
};



template<typename T>
class OutputSlot {
private:
    std::optional<T> _slot;
    std::exception_ptr _error;
    std::atomic<ResultState> _state{ResultState::Empty};

    void publish_state(ResultState state) {
        _state.store(state, std::memory_order_release);
        _state.notify_all();
    }

public:

    ResultState state() const { return _state.load(std::memory_order_acquire); }

    void publish(T slot) {
        if (state() != ResultState::Empty)
            return;
        _slot.emplace(std::move(slot));
        publish_state(ResultState::Ready);
    }

    void wait() const {
        _state.wait(ResultState::Empty, std::memory_order_acquire);
    }

    // read_ready() returns instant slot value or throw error if state is not ready or contains error
    const T& read_ready() const {
        switch (state()) {
            case ResultState::Ready:
                return *_slot;
            case ResultState::Failed:
                std::rethrow_exception(_error);
            case ResultState::Empty:
                throw std::logic_error("Output is not ready");
        }
        throw std::logic_error("Invalid result state");
    }

    // Set its error from the given argument
    void fail(std::exception_ptr e) {
        if (state() != ResultState::Empty) return;

        _error = std::move(e);
        publish_state(ResultState::Failed);
    }


    bool failed() const { return _state.load(std::memory_order_acquire) == ResultState::Failed; }

    std::exception_ptr error() const { return failed() ? _error : nullptr; }

    const T& get() const {
        wait();
        return read_ready();
    }

};



#endif //CONCAL_OUTPUT_SLOT_H
