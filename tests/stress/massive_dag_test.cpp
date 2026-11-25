
#include "math_fixture.h++"
#include "workloads/dag_builder.h++"

TEST_P(DualParamFixture, MassiveDAG) {
    const auto N = GetParam();

    auto fn = runtime.func_register([] (Dual x, int k) -> Dual {
        burn_iterations(10000);
        return {
            std::sin(k * x.value) / k,
                std::cos(k * x.value) * x.derivative
            };
    });

    Dual x{0.0, 1.0};

    std::vector<OutputRef<Dual>> outputs;
    for (int i = 1; i <= N; ++i) {
        auto r = runtime.invoke(fn, x, i);
        outputs.push_back(r);
    }

    auto result = test::dag::reduce(runtime, math.add, outputs);


    EXPECT_NEAR(result.wait_and_get()->value, 0.0, 1e-10); // Given inaccuracy of floating-point in double type, I use EXPECT_NEAR
    EXPECT_NEAR(result.wait_and_get()->derivative, double(N), 1e-8);

}



Dual testFunc(Dual x) {
    return Dual::mul(Dual::add(Dual::pow(x, 2), Dual::sin(x)), Dual::add(Dual::exp(x), Dual::pow(x, 3)));
}



TEST_P(DualParamFixture, DiffTests) {
    std::vector<Dual> inputs;
    std::vector<OutputRef<Dual>> outputs;
    for (size_t i = 0; i < GetParam(); ++i) {
        Dual x{i * 0.01};
        inputs.push_back(x);

        auto x_pow_2 = runtime.invoke(math.pow, x, 2);
        auto sin_x = runtime.invoke(math.sin, x);

        auto exp_x = runtime.invoke(math.exp, x);
        auto x_pow_3 = runtime.invoke(math.pow, x, 3);

        auto x_pow_2_add_sin_x = runtime.invoke(math.add, x_pow_2, sin_x);
        auto exp_x_add_x_pow_3 = runtime.invoke(math.add, exp_x, x_pow_3);

        auto y = runtime.invoke(math.mul, x_pow_2_add_sin_x, exp_x_add_x_pow_3);
        outputs.push_back(y);
    }
    for (size_t i = 0; i < GetParam(); ++i) {
        Dual expected = testFunc(inputs[i]);
        auto got = outputs[i].wait_and_get();
        if constexpr (std::is_same_v<Dual, Dual>) {
            EXPECT_NEAR(got->value, expected.value, 1e-9);
            EXPECT_NEAR(got->derivative, expected.derivative, 1e-9);
        }
    }
}



INSTANTIATE_TEST_SUITE_P(Sizes, DualParamFixture, ::testing::Values(1, 10, 1000));