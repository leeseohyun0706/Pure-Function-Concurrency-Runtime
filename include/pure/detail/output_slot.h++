
#ifndef CONCAL_OUTPUT_SLOT_H
#define CONCAL_OUTPUT_SLOT_H
#include <memory>

enum class ResultState {
    Empty,
    Ready,
    Failed
};



template<typename T>
class OutputSlot {
private:
    T _slot;
    std::exception_ptr _error;
    std::atomic<ResultState> _state{ResultState::Empty};

public:
    void publish(T slot) {
        if (_state.load(std::memory_order_acquire) != ResultState::Empty)
            return;
        _slot = slot;
        _state.store(ResultState::Ready, std::memory_order_release);
        _state.notify_all();
    }

    void wait() const {
        _state.wait(ResultState::Empty, std::memory_order_acquire);
    }

    // read_ready() returns instant slot value or throw error if state is not ready or contains error
    const T& read_ready() const {
        auto s = _state.load(std::memory_order_acquire);

        if (s == ResultState::Failed) std::rethrow_exception(_error);

        if (s != ResultState::Ready) throw std::logic_error("Output is not ready");

        return _slot;
    }

    // Set its error from the given argument
    void fail(std::exception_ptr e) {
        if (_state.load(std::memory_order_acquire) != ResultState::Empty) return;

        _error = std::move(e);
        _state.store(ResultState::Failed, std::memory_order_release);
        _state.notify_all();
    }


    bool failed() const { return _state.load(std::memory_order_acquire) == ResultState::Failed; }

    std::exception_ptr error() const { return failed() ? _error : nullptr; }

    const T& get() const {
        while (true) {
            auto s = _state.load(std::memory_order_acquire);

            if (s == ResultState::Ready)
                return _slot;
            if (s == ResultState::Failed)
                throw std::logic_error("Output occurs exception");
            _state.wait(ResultState::Empty, std::memory_order_acquire);
        }
    }

};



#endif //CONCAL_OUTPUT_SLOT_H
