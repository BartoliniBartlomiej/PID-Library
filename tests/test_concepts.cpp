#include <gtest/gtest.h>
#include "pid_controller/concepts.hpp"

TEST(ConceptTest, FloatingPoint) {
    EXPECT_TRUE(pid::FloatingPoint<double>);
    EXPECT_TRUE(pid::FloatingPoint<float>);
    EXPECT_FALSE(pid::FloatingPoint<int>);
}

TEST(ConceptTest, Numeric) {
    EXPECT_TRUE(pid::Numeric<int>);
    EXPECT_TRUE(pid::Numeric<double>);
    EXPECT_TRUE(pid::Numeric<float>);
}

// TEST(ConceptTest, ClockInterface) {
//     EXPECT_TRUE(pid::ClockInterface<pid::SystemClock>);
//     EXPECT_TRUE(pid::ClockInterface<pid::ManualClock>);
// }