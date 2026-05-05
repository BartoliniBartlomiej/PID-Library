# PID Controller

A modern C++20, header-only PID (Proportional-Integral-Derivative) controller library. Designed with embedded systems, testability, and high performance in mind, it provides a robust, zero-allocation implementation with advanced control features.

## Overview

This library provides a flexible `PIDController` template class constrained by C++20 concepts. It supports both Parallel and Series algorithm forms, built-in anti-windup mechanisms, derivative filtering, and dependency injection for time measurement to facilitate deterministic unit testing and simulations.

## Features

* **Header-only:** Easy integration into any project without building binaries.
* **Modern C++20:** Leverages Concepts (`std::floating_point`, etc.) for safer template instantiation and robust type checking.
* **Zero Heap Allocation:** Fully embedded-friendly; does not use `new`, `malloc`, or standard library containers that allocate dynamically.
* **Configurable Forms:** Choose between Parallel and Series (Interactive) PID algorithm implementations.
* **Anti-Windup Protection:** Prevents integral windup when the control output hits saturation limits using the clamping method.
* **Derivative Filtering:** Built-in low-pass filter for the derivative term to suppress high-frequency noise.
* **Dependency Injection for Time:** Comes with `SystemClock` for production and `ManualClock` for completely deterministic simulations and unit tests.
* **Robust Error Handling:** Uses C++23 `std::expected` (with a C++20 fallback to `std::variant`) and custom `std::error_code` structures.

## Mathematical Background

The controller supports two standard mathematical forms of the PID algorithm:

### Parallel Form
The standard independent form where P, I, and D terms operate independently:
$$u(t) = K_p \cdot e(t) + K_i \cdot \int e(t)dt + K_d \cdot \frac{de(t)}{dt}$$

### Series (Interactive) Form
A form commonly used in older analog controllers where the terms interact:
$$u(t) = K_p \cdot \left[ e(t) + \frac{1}{T_i} \cdot \int e(t)dt + T_d \cdot \frac{de(t)}{dt} \right]$$
*(where $T_i = K_p/K_i$ and $T_d = K_d/K_p$)*

## Requirements

* C++20 compatible compiler (GCC, Clang, MSVC)
* CMake 3.20 or higher

## Integration

### Using CMake

Since it is a header-only library, you can easily integrate it using `FetchContent` or by copying it into your project directory. 

```cmake
# Add the subdirectory if you cloned it locally
add_subdirectory(path/to/PIDController)

# Link against your target
target_link_libraries(your_executable PRIVATE PIDController::pid_controller)
```

### CMake Options
* `PID_BUILD_EXAMPLES`: Build example programs (Default: `ON`)
* `PID_BUILD_TESTS`: Build unit tests (Default: `ON`)
* `PID_BUILD_DOCS`: Build Doxygen documentation (Default: `OFF`)
* `PID_ENABLE_WARNINGS`: Enable strict compiler warnings (Default: `ON`)

## Quick Start

```cpp
#include <pid_controller/pid_controller.hpp>
#include <iostream>

int main() {
    // 1. Configure the PID Controller
    pid::PIDController<double>::Config config{
        .gains = {.kp = 1.0, .ki = 0.1, .kd = 0.05},
        .output_limits = {-100.0, 100.0},
        .form = pid::PIDForm::Parallel,
        .derivative_filter_coeff = 0.1, // Optional: low-pass filter for D-term
        .enable_anti_windup = true
    };

    // 2. Initialize the controller
    pid::PIDController<double> controller(config);

    // 3. Control loop simulation
    double setpoint = 100.0;
    double process_variable = 95.0; // Current measured state
    
    // Compute the control output
    double control_output = controller.compute(setpoint, process_variable);
    
    std::cout << "Control signal to apply: " << control_output << '\n';

    // 4. (Optional) Inspect internal state
    auto debug = controller.get_debug_info();
    std::cout << "P term: " << debug.p_term << '\n'
              << "I term: " << debug.i_term << '\n'
              << "D term: " << debug.d_term << '\n';

    return 0;
}
```

## Testing and Simulation

The library includes a `ManualClock` that makes testing deterministic by allowing you to manually advance the internal time of the controller:

```cpp
#include <pid_controller/clock_interface.hpp>
#include <pid_controller/pid_controller.hpp>

// Initialize clock and inject it into the controller
pid::ManualClock mock_clock;
pid::PIDController<double, pid::ManualClock> controller(config, mock_clock);

// Compute step 1
controller.compute(10.0, 0.0);

// Advance time by 100ms
mock_clock.advance(std::chrono::milliseconds(100));

// Compute step 2
controller.compute(10.0, 5.0);
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
Copyright (c) 2026 Bartłomiej Kuś.