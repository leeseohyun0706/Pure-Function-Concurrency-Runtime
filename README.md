
# A Concurrency Runtime with DAG Dependancy Management

A work-stealing task runtime that runs a DAG of pure functions on a pool of worker threads.

### System Requirements
| Name         | Version      |
|--------------|--------------|
| C++ Compiler | C++20        |
| cmake        | 4.2 or newer |
| conan        | 2.x          |

### Package Requirements

| Name          | Version |
|---------------|---------|
| spdlog        | 1.17.0  |
| nlohmann_json | 3.12.0  |
| gtest         | 1.18.0  |
| benchmaek     | 1.9.4   |

## Installation Setup

**macOS**
> This build was only tested in a macOS m1 environment.
```bash
xcode-select --install    # Apple Clang
brew install cmake ninja  # Build tools
brew install conan        # Package management
conan profile detect --force
```

## Build

```bash
cmake -S . -B build/debug -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=conan_provider.cmake
cmake --build build/debug
```
## Quick Example

```cpp
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
```