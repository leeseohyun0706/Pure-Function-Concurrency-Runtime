
#ifndef CONCAL_RUNTIME_FIXTURE_H
#define CONCAL_RUNTIME_FIXTURE_H

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include "math_helper.h++"
#include "dual.h++"
#include <pure/runtime.h++>

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


    static RuntimeConfiguration makeConfig() {
        RuntimeConfiguration cfg;
        cfg.scheduler.worker_size = 4;
        cfg.tracing.enable = true;
        return cfg;
    }

    Runtime runtime{makeConfig()};
    static const char* traceDir() { return "./traces"; }


    void SetUp() override {
        runtime.start();
    }


    void TearDown() override {
        runtime.shutdown();

        // auto *info = ::testing::UnitTest::GetInstance()->current_test_info();
        // std::string name = std::string(info->test_suite_name()) + "." + info->name();
        // std::ranges::replace(name, '/', '_');
        //
        // std::filesystem::create_directories(traceDir());
        // std::ofstream out(std::filesystem::path(traceDir()) / (name + ".json"));
        // runtime.exportTrace(out);
    }
};

class RuntimeParamFixture : public RuntimeFixture, public ::testing::WithParamInterface<size_t> {};

template<typename T>
class MathRuntimeFixture : public RuntimeFixture {
protected:
    MathFuncs<T> math{runtime};
};

template<typename T>
class MathParamFixture : public MathRuntimeFixture<T>, public ::testing::WithParamInterface<size_t>{};


using DualFixture = MathRuntimeFixture<Dual>;
using DoubleFixture = MathRuntimeFixture<double>;


using DualParamFixture =
    MathParamFixture<Dual>;

using DoubleParamFixture =
    MathParamFixture<double>;



#endif //CONCAL_RUNTIME_FIXTURE_H
