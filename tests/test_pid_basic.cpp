#include <gtest/gtest.h>
#include "pid_controller/pid_controller.hpp"
#include "pid_controller/clock_interface.hpp"

TEST(PIDTest, ConstructorValid) {
    pid::PIDController<double>::Config config{
        .gains = {.kp = 1.0, .ki = 0.1, .kd = 0.01},
        .output_limits = {-10.0, 10.0}
    };

    EXPECT_NO_THROW(pid::PIDController<double> pid(config));
}

TEST(PIDTest, ConstructorInvalid) {
    pid::PIDController<double>::Config config{
        .gains = {.kp = -1.0, .ki = 0.1, .kd = 0.01}
    };

    EXPECT_THROW(pid::PIDController<double> pid(config), std::invalid_argument);
}

TEST(PIDTest, FirstComputeReturnsZero) {
    pid::ManualClock clock;
    pid::PIDController<double, pid::ManualClock>::Config config{
        .gains = {.kp = 1.0, .ki = 0.0, .kd = 0.0}
    };
    pid::PIDController<double, pid::ManualClock> pid(config, clock);

    double out = pid.compute(10.0, 5.0);
    EXPECT_EQ(out, 0.0);
}

TEST(PIDTest, BasicResponse) {
    pid::ManualClock clock;
    pid::PIDController<double, pid::ManualClock>::Config config{
        .gains = {.kp = 2.0, .ki = 0.0, .kd = 0.0},
        .output_limits = {-100.0, 100.0}
    };

    pid::PIDController<double, pid::ManualClock> pid(config, clock);

    pid.compute(10.0, 5.0); // first
    clock.advance(std::chrono::seconds(1));

    double out = pid.compute(10.0, 5.0);
    EXPECT_DOUBLE_EQ(out, 10.0);
}