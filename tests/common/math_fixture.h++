
#ifndef CONCAL_MATH_FIXTURE_H
#define CONCAL_MATH_FIXTURE_H
#include "runtime_fixture.h++"
#include "workloads/math_helper.h++"
#include "workloads/dual.h++"

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


#endif //CONCAL_MATH_FIXTURE_H
