
#include <gtest/gtest.h>
#include <thread>
#include "concurrency/chase_lev_deque.h++"

// Basic logic test for ChaseLevDeque
// Push something for one time and pop, and then check whether it's empty.
TEST(DequeTests, BasicLogic) {
    ChaseLevDeque<size_t> deque;
    ASSERT_TRUE(deque.empty());
    deque.push(new size_t(100));
    ASSERT_FALSE(deque.empty());
    delete deque.pop();
    ASSERT_TRUE(deque.empty());
}


TEST(DequeTests, PushPop) {
}