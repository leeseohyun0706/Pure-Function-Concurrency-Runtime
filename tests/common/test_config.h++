
#ifndef CONCAL_TEST_CONFIG_H
#define CONCAL_TEST_CONFIG_H
#include <pure/configuration.h++>

namespace test {

    inline RuntimeConfiguration make_config() {
        RuntimeConfiguration config;

        config.scheduler.worker_size = 4;
        config.scheduler.work_stealing = true;
        config.scheduler.global_queue_batch_size = 8;

        config.tracing.enable = false;
        config.logging.enable = false;

        return config;
    }
}

#endif //CONCAL_TEST_CONFIG_H
