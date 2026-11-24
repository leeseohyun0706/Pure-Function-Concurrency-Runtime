

#ifndef CONCAL_RUNTIME_H
#define CONCAL_RUNTIME_H
#include <pure/logging/logger.h++>
#include <pure/configuration.h++>
#include <pure/task.h++>
class Scheduler;

class Runtime {
private:

    RuntimeConfiguration _runtimeConfig;

    LoggingContext _loggingContext;
    std::unique_ptr<Scheduler> _scheduler;
    IdGenerator<FuncId> _funcIdGenerator;
    IdGenerator<TaskId> _taskIdGenerator;

    std::shared_ptr<spdlog::logger> _logger;

    void admit();
    void seal(Task& task);

    template<typename R, typename... Params, typename... Args>
    auto create_task(
        const PureFunc<R(Params...)>& fn,
        Args&&... args
    ) {
        // By using std::decay_t, it removes 'const' or '&' stuffs in the type.
        using TaskType = PureFuncTask<R(Params...), std::decay_t<Params>...>;
        return std::make_shared<TaskType>(_taskIdGenerator.next(), fn, args...);
    }

    template<typename R, typename TaskT>
    static OutputRef<R> make_output_ref(const std::shared_ptr<TaskT>& task) {
        // Declare slot as std::shared_ptr to store output value.
        auto slot = std::shared_ptr<OutputSlot<R>>(task, std::addressof(task->output()));
        return OutputRef<R>{std::move(slot), task};
    }
public:

    explicit Runtime(const RuntimeConfiguration& config = RuntimeConfiguration::load_from_env());

    void start();
    void shutdown();

    template<typename F>
    auto func_register(F fn) {
        return PureFunc(_funcIdGenerator.next(), fn);
    }

    template<typename R, typename... Args>
    auto func_register(R(fn)(Args...)) {
        return PureFunc<R(Args...)>(_funcIdGenerator.next(), fn);
    }

    template<typename R, typename... Args1, typename... Args2>
    auto invoke(const PureFunc<R(Args1...)>& fn, Args2&&... args) {

        auto task = create_task(fn, args...);

        admit();
        task->pin();

        // Add dependency for Child Task indicating reference input.
        task->attach_dependencies(args...);
        auto output = make_output_ref<R>(task);
        seal(*task);
        return output;
    }

    ~Runtime();


    void exportTrace(std::ostream& os);
};
#endif //CONCAL_RUNTIME_H
