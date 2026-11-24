
#ifndef CONCAL_FUNC_H
#define CONCAL_FUNC_H
#include <variant>
#include <functional>
#include <memory>


#include <pure/id.h++>


template<typename Signature>
class PureFunc;


// Warning : DO NOT USE a Capture Lambda (that is actually not pure function since a Lambda captures outside variables or states).
template<typename R, typename... Args>
class PureFunc<R(Args...)> {
private:
    struct Callable {
        virtual ~Callable() = default;
        virtual R invoke(Args... args) const = 0;
        // virtual std::shared_ptr<const Callable> clone() const = 0;
    };

    template<typename F>
    struct CallableModel : public Callable {
        F func;
        R invoke(Args... args) const override { return func(std::forward<Args>(args)...); }

        explicit CallableModel(F&& func) : func(func) {}
        explicit CallableModel(const F& func) : func(func) {} // For Lambda
    };
    FuncId _id;
    // IDK whether it occurs copy when function is moving.
    std::shared_ptr<const Callable> _callable;

public:

    template<typename F>
    PureFunc(const FuncId& id, F func) : _id(id), _callable(std::make_unique<CallableModel<F>>(std::move(func))) {}

    PureFunc(const PureFunc&) = default;
    PureFunc(PureFunc&& other) = default;

    PureFunc& operator=(const PureFunc&) = default;
    PureFunc& operator=(PureFunc&& other) = default;

    // To make compiler know that the class is callable.
    R operator()(Args... args) const {
        if (!_callable) {
            throw std::bad_function_call();
        }
        return _callable->invoke(std::forward<Args>(args)...);
    }

    explicit operator bool() const noexcept {
        return _callable != nullptr;
    }

    auto invoke(Args... args) const {
        return operator()(std::forward<Args>(args)...);
    }
};

// Type Deduction Specialization
// WHY THEY DON'T GENERALIZE FUNCTION TEMPLATE SIGNATURE???
template <typename R, typename... Args>
PureFunc(FuncId, R(*)(Args...)) -> PureFunc<R(Args...)>;

template <typename F, typename Op = decltype(&F::operator())>
struct deduce_signature;

// For Lambda, but IDK how R(ClassType::*)(Args...) works.
template <typename F, typename R, typename ClassType, typename... Args>
struct deduce_signature<F, R(ClassType::*)(Args...) const> {
    using type = R(Args...);
};

// When Lambda is received, its template type is deduced to R(Args...) which is a function type removed Lambda Class.
// <lambda R(Args...)> -> deduce_signature<lambda R(Args...)>
// deduce_signature<lambda R(Args...)>::type -> R(Args...)
template <typename F>
PureFunc(FuncId, F) -> PureFunc<typename deduce_signature<F>::type>;


#endif //CONCAL_FUNC_H
