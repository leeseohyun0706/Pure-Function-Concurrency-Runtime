
#ifndef CONCAL_RUNTIME_FIXTURE_H
#define CONCAL_RUNTIME_FIXTURE_H

#include <fstream>
#include <gtest/gtest.h>
#include <pure/runtime.h++>
#include "test_config.h++"
#include "test_case.h++"


#if defined(_MSC_VER)
#include <intrin.h>
#endif


// Clang/GNU
#if defined(__GNUC__) || defined(__clang__)

// The same logic as GoogleBenchmark::DoNotOptimize
template<typename T>
inline __attribute__((always_inline)) void DoNotOptimize(T const& value) {
    asm volatile("" : : "r,m"(value) : "memory");
}

template<typename T>
inline __attribute__((always_inline)) void DoNotOptimize(T& value) {
#if defined(__clang__)
    asm volatile("" : "+r,m"(value) : : "memory");
#else
    asm volatile("" : "+m,r"(value) : : "memory");
#endif
}
#elif defined(_MSC_VER)

template<typename T>
__forceinline void DoNotOptimize(T const& value) {
    _ReadWriteBarrier();
    volatile auto* unused = &value;
    (void)unused;
}

#else

template<typename T>
inline void DoNotOptimize(T const& value) {
    (void)&value;
}

#endif

// Burning cpu operations for given times
inline void burn_iterations(size_t iterations) {
    double acc = 1.0;
    for (int i = 0; i < iterations; ++i) {
        acc = acc * 1.0000001 + 1e-9; // Dummy Operation
        DoNotOptimize(acc);
    }
}


class RuntimeFixture : public ::testing::Test {
protected:

    Runtime runtime{test::make_config()};
    static const char* traceDir() { return "./traces"; }


    void SetUp() override {
        runtime.start();
    }


    void TearDown() override {
        runtime.shutdown();
    }
};

class RuntimeMatrixFixture : public RuntimeFixture, public ::testing::WithParamInterface<RuntimeTestCase> {
protected:

    std::unique_ptr<Runtime> runtime;

    void SetUp() override {
        const auto& params = GetParam();

        auto config = test::make_config();

        config.scheduler.worker_size = params.workers;
        config.scheduler.work_stealing = params.work_stealing;
        config.scheduler.global_queue_batch_size = params.batch_size;

        runtime = std::make_unique<Runtime>(config);
        runtime->start();
    }


    void TearDown() override {
        if (runtime)
            runtime->shutdown();
    }

};

// class RuntimeParamFixture : public RuntimeFixture, public ::testing::WithParamInterface<size_t> {};


#endif //CONCAL_RUNTIME_FIXTURE_H
