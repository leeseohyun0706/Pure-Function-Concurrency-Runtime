

#ifndef CONCAL_RUNTIME_H
#define CONCAL_RUNTIME_H
#include <pure/logging/logger.h++>
// #include <runtime/scheduler.h++>
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

    // std::mutex _mutex;
    std::shared_ptr<spdlog::logger> _logger;
    static void submit() {}
    void admit();
    void seal(Task& task);

    template<typename TaskT, typename T>
    void attach_dependency(TaskT&, const T&, size_t&) {}

    template<typename TaskT, typename T>
    void attach_dependency(TaskT& child, const OutputRef<T>& ref, size_t& i) {
        // Add dependency in child
        child.depends_on(ref.getTask(), child.edge(i++));
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

        // By using std::decay_t, it removes 'const' or '&' stuffs in the type.
        auto task =
            std::make_shared<PureFuncTask<R(Args1...), std::decay_t<Args1>...>>(_taskIdGenerator.next(), fn, args...);

        admit();
        task->pin();

        size_t edge = 0;
        // Add dependency for Child Task indicating reference input.
        (attach_dependency(*task, args, edge), ...);
        // Declare slot as std::shared_ptr to store output value.
        std::shared_ptr<OutputSlot<R>> slot(task, &task->output());
        seal(*task);
        return OutputRef<R>{slot, task};
    }

    ~Runtime();


    void exportTrace(std::ostream& os);
};
#endif //CONCAL_RUNTIME_H
