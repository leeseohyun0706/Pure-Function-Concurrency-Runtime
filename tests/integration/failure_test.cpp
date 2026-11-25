#include "runtime_fixture.h++"


TEST_F(RuntimeFixture, FailurePropagates) {
    auto boom = runtime.func_register([](double) -> double { throw std::runtime_error("boom"); });
    auto inc  = runtime.func_register([](double x) { return x + 1; });

    auto a = runtime.invoke(boom, 1.0);
    auto b = runtime.invoke(inc, a);
    auto c = runtime.invoke(inc, b);

    EXPECT_THROW(c.wait_and_get(), std::runtime_error);
    EXPECT_THROW(a.wait_and_get(), std::runtime_error);
}
