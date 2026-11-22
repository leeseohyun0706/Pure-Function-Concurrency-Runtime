
# Package Requirements GTest and nlohmann_json
find_package(GTest CONFIG REQUIRED)

include(GoogleTest)

function(c_add_test name label)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common)
    target_link_libraries(${name} PRIVATE concal GTest::gtest_main concal_concurrency)
    gtest_discover_tests(${name} PROPERTIES LABELS ${label} TIMEOUT 30)
endfunction()

c_add_test(integration_tests integration integration/invoke_test.cpp
        integration/dag_execution_test.cpp)

c_add_test(stress_tests stress
        stress/massive_dag_test.cpp)

c_add_test(unit_tests unit unit/concurrency/deque_test.cpp)

find_package(benchmark CONFIG REQUIRED)

#add_executable(stress_bench benchmark/stress_bench.cpp)
#target_include_directories(stress_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common)
#target_link_libraries(stress_bench PRIVATE concal benchmark::benchmark_main)