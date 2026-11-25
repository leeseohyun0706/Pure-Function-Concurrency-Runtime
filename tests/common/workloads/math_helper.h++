
#ifndef CONCAL_MATH_HELPER_H
#define CONCAL_MATH_HELPER_H
#include <cmath>

#include <pure/func.h++>
#include <pure/runtime.h++>
#include "dual.h++"

// Adapters so Dual works with GenericMath (found by argument-dependent lookup)
inline Dual operator+(Dual a, Dual b) { return Dual::add(a, b); }
inline Dual operator-(Dual a, Dual b) { return Dual::sub(a, b); }
inline Dual operator*(Dual a, Dual b) { return Dual::mul(a, b); }
inline Dual operator/(Dual a, Dual b) { return Dual::div(a, b); }
inline Dual sin(Dual x)               { return Dual::sin(x); }
inline Dual cos(Dual x)               { return Dual::cos(x); }
inline Dual exp(Dual x)               { return Dual::exp(x); }
inline Dual pow(Dual x, int p)        { return Dual::pow(x, p); }


namespace GenericMath {

    template<typename T>
     T add(T a, T b) { return a + b; }
    template<typename T>
     T sub(T a, T b) { return a - b; }
    template<typename T>
     T mul(T a, T b) { return a * b; }
    template<typename T>
     T div(T a, T b) { return a / b; }

    template<typename T> T pow(T a, int p) { using std::pow; return pow(a, p); }
    template<typename T> T exp(T a)        { using std::exp; return exp(a); }
    template<typename T> T sin(T a)        { using std::sin; return sin(a); }
    template<typename T> T cos(T a)        { using std::cos; return cos(a); }
};

template<typename T>
struct MathFuncs {
 PureFunc<T(T, T)> add, sub, mul, div;
 PureFunc<T(T, int)> pow;
 PureFunc<T(T)> exp, sin, cos;


 explicit MathFuncs(Runtime& rt)
     : add(rt.func_register(GenericMath::add<T>)),
       sub(rt.func_register(GenericMath::sub<T>)),
       mul(rt.func_register(GenericMath::mul<T>)),
       pow(rt.func_register(GenericMath::pow<T>)),
       exp(rt.func_register(GenericMath::exp<T>)),
       sin(rt.func_register(GenericMath::sin<T>)),
       cos(rt.func_register(GenericMath::cos<T>)),
       div(rt.func_register(GenericMath::div<T>)) {}
};


#endif //CONCAL_MATH_HELPER_H
