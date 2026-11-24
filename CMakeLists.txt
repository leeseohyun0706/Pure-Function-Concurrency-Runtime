cmake_minimum_required(VERSION 4.2)
project(Concal VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


find_package(Threads REQUIRED)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

find_package(libenvpp CONFIG REQUIRED)

# Options
option(FFF_BUILD_TESTS "Build unit, integration, and stress tests" ${PROJECT_IS_TOP_LEVEL})

option(FFF_BUILD_BENCHMARKS "Build benchmark executables" ${PROJECT_IS_TOP_LEVEL})
option(FFF_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(FFF_TSAN "Enable ThreadSanitizer" OFF)

# Header directory for concurrency implementations
add_library(concal_concurrency INTERFACE)
target_include_directories(concal_concurrency
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)



add_library(concal STATIC
        src/runtime/scheduler.cpp
        src/runtime/schedulerpolicy.cpp
        src/runtime/task.cpp
        src/runtime/worker.cpp
        src/tracing/trace_recorder.cpp
        src/tracing/trace_exporter.cpp
        src/runtime/runtime.cpp
)

target_compile_features(concal PUBLIC cxx_std_20)

target_include_directories(concal
        PUBLIC ${PROJECT_SOURCE_DIR}/include
        PRIVATE ${PROJECT_SOURCE_DIR}/src
)

target_compile_options(concal PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive- /Zc:__cplusplus /wd4324>   # 4324 = alignas padding note
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>)


target_link_libraries(concal
        PRIVATE
        concal_concurrency
        nlohmann_json::nlohmann_json
        Threads::Threads
        PUBLIC spdlog::spdlog libenvpp::libenvpp
)

target_compile_definitions(concal PRIVATE
        SPDLOG_ACTIVE_LEVEL=$<IF:$<CONFIG:Debug>,SPDLOG_LEVEL_TRACE,SPDLOG_LEVEL_INFO>)

if(FFF_BUILD_TESTS OR FFF_BUILD_BENCHMARKS)
    enable_testing()
    add_subdirectory(tests)
endif()


add_executable(quick_example example/quick_example.cpp)
target_link_libraries(quick_example PUBLIC concal)
target_include_directories(quick_example PUBLIC concal_concurrency)
