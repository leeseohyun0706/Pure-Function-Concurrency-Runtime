
#ifndef CONCAL_TEST_CASE_H
#define CONCAL_TEST_CASE_H

#include <format>

struct RuntimeTestCase {
    uint32_t workers = 4;
    size_t task_count = 1000;

    bool work_stealing = true;

    uint32_t batch_size = 8;
};


inline void PrintTo(const RuntimeTestCase& test_case, std::ostream* os) {
    *os << std::format("RuntimeTestCase{{worker={}, taskN={}, steal={}, batch={}}}",
        test_case.workers,
        test_case.task_count,
        test_case.work_stealing,
        test_case.batch_size
    );
}

#endif //CONCAL_TEST_CASE_H
