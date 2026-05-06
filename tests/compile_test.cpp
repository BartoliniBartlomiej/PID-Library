#include "pid_controller/concepts.hpp"
#include "pid_controller/types.hpp"
#include "pid_controller/clock_interface.hpp"
#include "pid_controller/pid_controller.hpp"

#include <iostream>

// Concepts tests
static_assert(pid::FloatingPoint<double>);
static_assert(pid::FloatingPoint<float>);
static_assert(!pid::FloatingPoint<int>);

// Clock interface tests
static_assert(pid::ClockInterface<pid::SystemClock>);
static_assert(pid::ClockInterface<pid::ManualClock>);

int main() {
    std::cout << "=== PID Controller Library - Compile Test ===" << std::endl;
    
    // Test error handling
    auto err = pid::make_error_code(pid::PIDError::InvalidGains);
    std::cout << "Error code: " << err.message() << std::endl;
    
    // Test SystemClock
    pid::SystemClock sys_clock;
    auto t1 = sys_clock.now();
    (void)t1; // Suppress unused warning
    std::cout << "SystemClock works!" << std::endl;
    
    // Test ManualClock
    pid::ManualClock manual_clock;
    auto t2 = manual_clock.now();
    manual_clock.advance(std::chrono::milliseconds(100));
    auto t3 = manual_clock.now();
    
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2);
    std::cout << "ManualClock advanced by: " << diff.count() << "ms" << std::endl;
    
    // Test PIDController compilation
    std::cout << "\n=== Testing PID Controller ===" << std::endl;
    
    pid::PIDController<double>::Config pid_config{
        .gains = {.kp = 1.0, .ki = 0.1, .kd = 0.05},
        .output_limits = {-100.0, 100.0},
        .form = pid::PIDForm::Parallel
    };

    pid::PIDController<double> controller(pid_config);
        
    std::cout << controller.get_gains().kd;

    double output = controller.compute(100.0, 95.0);
    // std::cout << "PID output for error=5.0: " << output << std::endl;
    
    auto debug = controller.get_debug_info();
    std::cout << "  P term: " << debug.p_term << std::endl;
    std::cout << "  I term: " << debug.i_term << std::endl;
    std::cout << "  D term: " << debug.d_term << std::endl;

    std::cout << "\n✅ All compile-time tests passed!" << std::endl;
    return 0;
}