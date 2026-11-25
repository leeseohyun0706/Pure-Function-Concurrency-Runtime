
# A Concurrency Runtime with DAG Dependancy Management

A work-stealing task runtime that runs a DAG of pure functions on a pool of worker threads.

### System Requirements
| Name         | Version            |
|--------------|--------------------|
| C++ Compiler | C++20 (LLVM/clang) |
| cmake        | 4.2 or newer       |
| Ninja        | Required           |
| conan        | 2.x                |

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

### CMake Presets

Build configurations are defined in `CmakePresets.json`.

| Preset  | Build Type     | Description                  |
|---------|----------------|------------------------------|
| debug   | Debug          | Development and debugging    |
| release | Release        | Optimized executables        |
| tsan    | RelWithDebInfo | ThreadSanitizer-enable build |

Make sure that the preset list is available.
```bash
cmake --list-presets
cmake --build --list-presets
ctest --list-presets
```

## Build With Configuration

### Debug Build

```bash
cmake --preset debug
cmake --build --preset debug
```

### Release Build

```bash
cmake --preset release
cmake --build --preset release
```

### ThreadSanitizer Build

```bash
cmake --preset tsan
cmake --build --preset tsan
```

## Tests

Tests are implemented using `GoogleTest` and executed through `CTtest`.
> Make sure the corresponding build preset has been built before running tests.

```bash
ctest --preset debug
ctest --preset release
ctest --preset tsan
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