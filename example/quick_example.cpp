#include <pure/runtime.h++>

#include <iostream>
// Note that only pure functions can be applied in the runtime.
double square(double x) { return x * x; }
double add(double a, double b) { return a + b; }

int main() {
    Runtime runtime;
    runtime.start();

    auto pure_square = runtime.func_register(square);
    auto pure_add = runtime.func_register(add);


    auto a = runtime.invoke(pure_square, 3.0); // 3 ^ 2
    auto b = runtime.invoke(pure_square, 4.0); // 4 ^ 2
    auto c = runtime.invoke(pure_add, a, b);

    std::cout << "result = " << *c.wait_and_get() << std::endl;

    runtime.shutdown();

    return 0;
}