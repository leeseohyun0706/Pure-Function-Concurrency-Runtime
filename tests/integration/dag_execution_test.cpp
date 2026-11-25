#include "math_fixture.h++"

// x^2 + sin(x)
TEST_F(DualFixture, BasicDAG) {

    Dual x{2.0, 1.0};

    auto a = runtime.invoke(math.pow, x, 2);
    auto b = runtime.invoke(math.sin, x);

    auto c = runtime.invoke(math.add, a, b);

    EXPECT_NEAR(c.wait_and_get()->value, 4.0 + std::sin(2.0), 1e-10);
    EXPECT_NEAR(c.wait_and_get()->derivative, 4.0 + std::cos(2.0), 1e-10);
}

// sin(sin(sin(x)))
TEST_F(DualFixture, Chain) {
    Dual x{1.0, 1.0};


    auto a = runtime.invoke(math.sin, x);
    auto b = runtime.invoke(math.sin, a);
    auto c = runtime.invoke(math.sin, b);

    const double expected = std::sin(std::sin(std::sin(1.0)));
    const double derivative = std::cos(std::sin(std::sin(1.0))) * std::cos(std::sin(1.0)) * std::cos(1.0);


    EXPECT_NEAR(c.wait_and_get()->value, expected, 1e-10);
    EXPECT_NEAR(c.wait_and_get()->derivative, derivative, 1e-10);
}


// (((10 + 5) * 2) + ((10 + 5) - 3)) / 6 = 7
TEST_F(DoubleFixture, DiamondDAG) {

    auto a = runtime.invoke(math.add, 10, 5);
    auto b = runtime.invoke(math.mul, a, 2);
    auto c = runtime.invoke(math.sub, a, 3);

    auto d = runtime.invoke(math.add, b, c);
    auto e = runtime.invoke(math.div, d, 6);

    EXPECT_EQ(static_cast<int>(*e.wait_and_get()), 7);

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

