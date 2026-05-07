#include <gtest/gtest.h>
#include "pid_controller/clock_interface.hpp"
#include <chrono>

TEST(SystemClockTest, NowWorks) {
    pid::SystemClock clock;

    auto t1 = clock.now();
    auto t2 = clock.now();

    EXPECT_LE(t1, t2); // t1 <= t2
}

TEST(ManualClockTest, Advance) {
    pid::ManualClock clock;

    auto t1 = clock.now();
    clock.advance(std::chrono::milliseconds(100));
    auto t2 = clock.now();

    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);

    EXPECT_EQ(diff.count(), 100);
}

TEST(ManualClockTest, SetTime) {
    using tp = pid::ManualClock::time_point;
    pid::ManualClock clock;

    tp custom{};
    clock.set_time(custom);
    EXPECT_EQ(clock.now(), custom);
}

TEST(ManualClockTest, Reset) {
    pid::ManualClock clock; 
    clock.advance(std::chrono::seconds(1));
    clock.reset();

    EXPECT_EQ(clock.now(), pid::ManualClock::time_point{});
}