// #include <gtest/gtest.h>
#include <fstream>
#include <random>

#include "math_helper.h++"
#include "runtime_fixture.h++"
#include "../../include/pure/runtime.h++"
#include "../common/dual.h++"

double _square(double x) { return x * x; }


/*
 * Testing N amounts of x^2 tasks through runtime and verify correctness.
 */
TEST_P(RuntimeParamFixture, SquareTest) {

    auto square = runtime.func_register([](double x) { return x * x; });

    std::vector<OutputRef<double>> outputs;
    for (int i = 0; i < GetParam(); ++i) {
        auto r = runtime.invoke(square, static_cast<double>(i));
        outputs.push_back(r);
    }

    for (int i = 0; i < GetParam(); ++i) {
        // x^2 = x^2
        EXPECT_DOUBLE_EQ(*outputs[i].wait_and_get(), static_cast<double>(i) * static_cast<double>(i));
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

/*
 * Testing whether the result of a completed task is available, instead of rerun the task.
 */
TEST_F(RuntimeFixture, ParentAlreadyCompleted) {

    auto square = runtime.func_register([](double x) { return x * x; });

    auto inc = runtime.func_register([](double x) { return x + 1; });

    auto parent = runtime.invoke(square, 4.0);
    EXPECT_DOUBLE_EQ(*parent.wait_and_get(), 16.0);

    auto child = runtime.invoke(inc, parent);
    EXPECT_DOUBLE_EQ(*child.wait_and_get(), 17.0);
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
        outputs[i].wait_and_get();
    }
}

TEST_F(RuntimeFixture, FailurePropagates) {
    auto boom = runtime.func_register([](double) -> double { throw std::runtime_error("boom"); });
    auto inc  = runtime.func_register([](double x) { return x + 1; });

    auto a = runtime.invoke(boom, 1.0);
    auto b = runtime.invoke(inc, a);
    auto c = runtime.invoke(inc, b);

    EXPECT_THROW(c.wait_and_get(), std::runtime_error);
    EXPECT_THROW(a.wait_and_get(), std::runtime_error);
}

TEST_F(RuntimeFixture, Simple) {

    constexpr long long N = 10'000'000;
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_real_distribution<double> dist(-1.0, 1.0);

    long long inside_circle = 0;
    for (long long i = 0; i < N; ++i) {
        double x = dist(gen);
        double y = dist(gen);
        if (x * x + y * y <= 1.0) {
            ++inside_circle;
        }
    }

    double pi_estimate = 4.0 * static_cast<double>(inside_circle) / static_cast<double>(N);
    std::cout << "Estimated pi: " << pi_estimate << '\n';
}
TEST_F(DoubleFixture, MonteCarlo) {

    constexpr long long N = 10'000'00;
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_real_distribution<double> dist(-1.0, 1.0);

    std::atomic<unsigned long long> inside_circle{0};

    std::vector<OutputRef<bool>> outputs;
    auto calc = runtime.func_register([&inside_circle](double x, double y) { if (x * x + y * y <= 1.0) inside_circle.fetch_add(1, std::memory_order_relaxed); return true; });

    for (long long i = 0; i < N; ++i) {
        double x = dist(gen);
        double y = dist(gen);
        outputs.push_back(runtime.invoke(calc, x, y));
    }

    for (auto& r : outputs) {
        if (*r.wait_and_get());
    }

    double pi_estimate = 4.0 * static_cast<double>(inside_circle.load()) / static_cast<double>(N);
    std::cout << "Estimated pi: " << pi_estimate << '\n';
}
INSTANTIATE_TEST_SUITE_P(Sizes, RuntimeParamFixture, ::testing::Values(1, 10, 1000));