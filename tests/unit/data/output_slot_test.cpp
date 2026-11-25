#include <gtest/gtest.h>

#include <pure/detail/output_slot.h++>
#include <latch>
#include <thread>

// Ensure that the state, error, and result of read_ready() are initially empty, nullptr, and std::logic_error respectively.
TEST(OutputSlotTest, InitialEmpty) {
    OutputSlot<int> slot;

    EXPECT_FALSE(slot.failed());
    EXPECT_EQ(slot.error(), nullptr);

    EXPECT_THROW(slot.read_ready(), std::logic_error);
}


TEST(OutputSlotTest, PublishesValue) {
    OutputSlot<int> slot;

    slot.publish(42);

    EXPECT_EQ(slot.read_ready(), 42);
    EXPECT_EQ(slot.get(), 42);
    EXPECT_FALSE(slot.failed());
}

// Ensure when producer calls fail(), its states are correct.
TEST(OutputSlotTest, PublishesException) {
    OutputSlot<int> slot;

    slot.fail(std::make_exception_ptr(
        std::runtime_error("Something Exception")
    ));

    EXPECT_TRUE(slot.failed());
    EXPECT_NE(slot.error(), nullptr);

    EXPECT_THROW(
        slot.read_ready(),
        std::runtime_error
    );
}


TEST(OutputSlotTest, PublishesFromAnotherThread) {
    OutputSlot<int> slot;

    std::latch started{1};
    int result = 0;

    std::thread consumer([&] {
        started.count_down();

        slot.wait();
        result = slot.read_ready();
    });

    started.wait();

    slot.publish(123);

    consumer.join();

    EXPECT_EQ(result, 123);
}


TEST(OutputSlotTest, PublishesFromMultipleThreads) {
    OutputSlot<int> slot;

    constexpr size_t N = 8;

    std::array<int, N> results{};
    std::latch started{N};

    std::vector<std::thread> consumers;

    for (size_t i = 0; i < N; ++i) {
        consumers.emplace_back([&, i] {
            started.count_down();
            slot.wait(); // N amounts of threads are waiting until the slot published.
            results[i] = slot.read_ready();
        });
    }

    started.wait();

    slot.publish(42);

    for (auto& thread : consumers)
        thread.join();

    for (auto result : results)
        EXPECT_EQ(result, 42);
}