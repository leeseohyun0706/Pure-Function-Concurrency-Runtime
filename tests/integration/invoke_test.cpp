// #include <gtest/gtest.h>
#include <fstream>
#include <random>

#include "math_fixture.h++"
#include "pure/runtime.h++"

TEST_P(RuntimeMatrixFixture, TaskIndependency) {
    const auto n = GetParam().task_count;

    auto square = runtime->func_register(
        [](uint64_t x) {
            return x * x;
        }
    );

    std::vector<OutputRef<uint64_t>> outputs;
    outputs.reserve(n);

    for (uint64_t i = 0; i < n; ++i) {
        outputs.push_back(
            runtime->invoke(square, i)
        );
    }

    for (uint64_t i = 0; i < n; ++i) {
        ASSERT_EQ(*outputs[i].wait_and_get(), i * i)
            << "Task index: " << i;
    }
}

/*
 * Testing N amounts of x^2 tasks through runtime and verify correctness.
 */
TEST_P(RuntimeMatrixFixture, IndependentTasksReturnCorrectValue) {
    const auto count = GetParam().task_count;

    auto square = runtime->func_register([](double x) { return x * x; });

    std::vector<OutputRef<double>> outputs;
    outputs.reserve(count);
    for (int i = 0; i < count; ++i) {
        auto r = runtime->invoke(square, static_cast<double>(i));
        outputs.push_back(r);
    }

    for (int i = 0; i < count; ++i) {
        SCOPED_TRACE(i);

        const double x = static_cast<double>(i);
        // x^2 = x^2
        EXPECT_DOUBLE_EQ(*outputs[i].wait_and_get(), x * x);
    }

}


/*
 * Testing if a task in lambda function works
 */
TEST_F(RuntimeFixture, OutputRefLifeTime) {
    auto square = runtime.func_register([](double x) { return x * x; });

    auto result = [&] {
        auto tmp = runtime.invoke(square, 5.0);
        return tmp;
    }();

    EXPECT_DOUBLE_EQ(*result.wait_and_get(), 25.0);

}


uint64_t compute(uint64_t x, uint32_t iterations) {
    for (uint32_t i = 0; i < iterations; ++i) {
        x = x * 6364136223846793005ULL + 1;
    }

    return x;
}

TEST_F(RuntimeFixture, Granuly) {
    auto func = runtime.func_register(compute);

    std::vector<OutputRef<uint64_t>> outputs;
    for (uint32_t i = 0; i < 10; ++i) {
        auto r = runtime.invoke(func, i, i * 5);
        outputs.push_back(r);
    }

    for (uint32_t i = 0; i < 10; ++i) {
        SCOPED_TRACE(i);

        EXPECT_EQ(
            *outputs[i].wait_and_get(),
            compute(i, i * 5)
        );
    }
}

// TEST_F(RuntimeFixture, Simple) {
//
//     constexpr long long N = 10'000'000;
//     std::random_device rd;
//     std::mt19937_64 gen(rd());
//     std::uniform_real_distribution<double> dist(-1.0, 1.0);
//
//     long long inside_circle = 0;
//     for (long long i = 0; i < N; ++i) {
//         double x = dist(gen);
//         double y = dist(gen);
//         if (x * x + y * y <= 1.0) {
//             ++inside_circle;
//         }
//     }
//
//     double pi_estimate = 4.0 * static_cast<double>(inside_circle) / static_cast<double>(N);
//     std::cout << "Estimated pi: " << pi_estimate << '\n';
// }

TEST_F(DoubleFixture, MonteCarlo) {

    constexpr long long N = 10'000'0;
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_real_distribution<double> dist(-1.0, 1.0);

    std::atomic<unsigned long long> inside_circle{0};

    std::vector<OutputRef<bool>> outputs;
    // Note that the logic of increasing inside_circle is not a pure function, yet it's just test.
    auto calc = runtime.func_register([&inside_circle](double x, double y) { if (x * x + y * y <= 1.0) inside_circle.fetch_add(1, std::memory_order_relaxed); return true; });

    for (long long i = 0; i < N; ++i) {
        double x = dist(gen);
        double y = dist(gen);
        outputs.push_back(runtime.invoke(calc, x, y));
    }

    for (auto& r : outputs) {
        DoNotOptimize(*r.wait_and_get());
    }

    double pi_estimate = 4.0 * static_cast<double>(inside_circle.load()) / static_cast<double>(N);
    std::cout << "Estimated pi: " << pi_estimate << '\n';
}





INSTANTIATE_TEST_SUITE_P(
  Configuration,
  RuntimeMatrixFixture,
  ::testing::Values(
   RuntimeTestCase{1, 1000, false, 1},
   RuntimeTestCase{2, 1000, true, 1},
   RuntimeTestCase{4, 1000, true, 8},
   RuntimeTestCase{8, 1000, true, 8},
   RuntimeTestCase{4, 1000, false, 8},
   RuntimeTestCase{4, 1000, true, 32}
  )
);

// INSTANTIATE_TEST_SUITE_P(Sizes, RuntimeParamFixture, ::testing::Values(1, 10, 1000));