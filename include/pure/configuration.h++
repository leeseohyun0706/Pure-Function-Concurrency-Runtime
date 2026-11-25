
#ifndef CONCAL_CONFIGURATION_H
#define CONCAL_CONFIGURATION_H
#include <cstdint>
#include <envcfg/envcfg.h++>
#include <thread>
#include <iostream>
#include <unordered_map>


template<typename T>
std::optional<T> from_string(const std::string_view& str);


struct WorkerConfiguration {
    enum class IdleStrategy {
        Park, // default
        SpinThenPark, // spin before park
        Yield, // use Thread Yield (For special purposes)
    };


    uint32_t initial_deque_capacity = 32;
    IdleStrategy idle = IdleStrategy::Park;
    uint32_t spin_before_park = 0; // Only used for SpinThenPark

    static WorkerConfiguration load_from_env() {
        WorkerConfiguration config;

        config.initial_deque_capacity = envcfg::get<uint32_t>("FFF_WORKER_INITIAL_DEQUE_CAPACITY", 32);
        config.idle = envcfg::get<IdleStrategy>("FFF_WORKER_IDLE_STRATEGY", IdleStrategy::Park);
        config.spin_before_park = envcfg::get<uint32_t>("FFF_SPIN_BEFORE_PARK", 0);

        return config;
    }
};

// Configuration for Scheduler
struct SchedulerConfiguration {

    enum class ShutdownMode {
        Drain, // Wait until all tasks completed.
        Cancel
    } shutdown_mode{ShutdownMode::Drain};
    uint32_t worker_size = 4; // Thread size in current implementation
    bool work_stealing = true;
    uint32_t global_queue_batch_size = 8;

    WorkerConfiguration worker;

    static SchedulerConfiguration load_from_env() {

        SchedulerConfiguration config;
        config.worker = WorkerConfiguration::load_from_env();
        config.worker_size = envcfg::get<uint32_t>("FFF_WORKER_SIZE", std::max(1u, std::thread::hardware_concurrency()));
        config.work_stealing = envcfg::get<bool>("FFF_WORK_STEALING_ENABLE", true);
        config.global_queue_batch_size = envcfg::get<uint32_t>("FFF_GLOBAL_DEQUE_BATCH_SIZE", 8);
        config.shutdown_mode = envcfg::get<ShutdownMode>("FFF_SCHEDULER_SHUTDOWN_MODE", ShutdownMode::Drain);
        return config;
    }
};


struct TaskConfiguration {

};

struct TracingConfiguration {
    bool enable = true;
    uint32_t buffer_size = 4096 * 2;

    static TracingConfiguration load_from_env() {
        TracingConfiguration config;

        config.enable = envcfg::get<bool>("FFF_TRACING_ENABLE", true);
        config.buffer_size = envcfg::get<uint32_t>("FFF_TRACING_BUFFER_SIZE", 4096 * 2);
        return config;
    }
};


struct LoggingConfiguration {
    // Copy from spdlog::level::level_enum structure
    enum class Level {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Critical,
        Off
    };

    bool enable = true;

    Level level = Level::Info;
    Level flush_on = Level::Warn;

    bool console = true;

    static LoggingConfiguration load_from_env() {
        LoggingConfiguration config;
        config.enable = envcfg::get<bool>("FFF_LOGGING_ENABLE", true);
        config.level = envcfg::get<Level>("FFF_LOGGING_LEVEL", Level::Info);
        config.flush_on = envcfg::get<Level>("FFF_LOGGING_FLUSH_LEVEL", Level::Warn);
        return config;
    }

};

// each Runtime owns its RuntimeConfiguration
struct RuntimeConfiguration {
    SchedulerConfiguration scheduler;
    TracingConfiguration tracing;
    LoggingConfiguration logging;

    static RuntimeConfiguration load_from_env() {
        RuntimeConfiguration config;

        config.scheduler = SchedulerConfiguration::load_from_env();
        config.tracing = TracingConfiguration::load_from_env();
        config.logging = LoggingConfiguration::load_from_env();
        return config;
    }
};


template<>
inline std::optional<LoggingConfiguration::Level> from_string(const std::string_view& str) {
    static const std::unordered_map<std::string_view, LoggingConfiguration::Level> Entries = {
        {"trace", LoggingConfiguration::Level::Trace},
        {"debug", LoggingConfiguration::Level::Debug},
        {"info", LoggingConfiguration::Level::Info},
        {"warn", LoggingConfiguration::Level::Warn},
        {"error", LoggingConfiguration::Level::Error},
        {"critical", LoggingConfiguration::Level::Critical},
        {"off", LoggingConfiguration::Level::Off}
    };
    return Entries.count(str) > 0 ? Entries.at(str) : std::optional<LoggingConfiguration::Level>{};

}

template<>
inline std::optional<SchedulerConfiguration::ShutdownMode> from_string(const std::string_view& str) {
    static const std::unordered_map<std::string_view, SchedulerConfiguration::ShutdownMode> Entries = {
        {"drain", SchedulerConfiguration::ShutdownMode::Drain},
          {"cancel", SchedulerConfiguration::ShutdownMode::Cancel}
    };

    return Entries.count(str) > 0 ? Entries.at(str) : std::optional<SchedulerConfiguration::ShutdownMode>{};
}

template<>
inline std::optional<WorkerConfiguration::IdleStrategy> from_string(const std::string_view& str) {
    static const std::unordered_map<std::string_view, WorkerConfiguration::IdleStrategy> Entries = {
        {"park", WorkerConfiguration::IdleStrategy::Park},
        {"spin_then_park", WorkerConfiguration::IdleStrategy::SpinThenPark},
        {"yield", WorkerConfiguration::IdleStrategy::Yield},
    };
    return Entries.contains(str) ? Entries.at(str) : std::optional<WorkerConfiguration::IdleStrategy>{};
}




namespace envcfg {
    template<>
    struct Parser<LoggingConfiguration::Level> {
        static LoggingConfiguration::Level parse(std::string_view str)  {
            auto r = from_string<LoggingConfiguration::Level>(str);
            if (!r) throw std::invalid_argument("Invalid Logging Level");
            return r.value();
        }
    };
    template<>
    struct Parser<SchedulerConfiguration::ShutdownMode> {
        static SchedulerConfiguration::ShutdownMode parse( std::string_view str)  {
            auto r = from_string<SchedulerConfiguration::ShutdownMode>(str);
            if (!r) throw std::invalid_argument("Invalid Shutdown Mode");
            return r.value();
        }
    };
    template<>
    struct Parser<WorkerConfiguration::IdleStrategy> {
        static WorkerConfiguration::IdleStrategy parse( std::string_view str)  {
            auto r = from_string<WorkerConfiguration::IdleStrategy>(str);
            if (!r) throw std::invalid_argument("Invalid Idle Strategy");
            return r.value();
        }
    };
}

#endif //CONCAL_CONFIGURATION_H
