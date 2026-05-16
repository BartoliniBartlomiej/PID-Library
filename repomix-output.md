This file is a merged representation of a subset of the codebase, containing files not matching ignore patterns, combined into a single document by Repomix.

# File Summary

## Purpose
This file contains a packed representation of a subset of the repository's contents that is considered the most important context.
It is designed to be easily consumable by AI systems for analysis, code review,
or other automated processes.

## File Format
The content is organized as follows:
1. This summary section
2. Repository information
3. Directory structure
4. Repository files (if enabled)
5. Multiple file entries, each consisting of:
  a. A header with the file path (## File: path/to/file)
  b. The full contents of the file in a code block

## Usage Guidelines
- This file should be treated as read-only. Any changes should be made to the
  original repository files, not this packed version.
- When processing this file, use the file path to distinguish
  between different files in the repository.
- Be aware that this file may contain sensitive information. Handle it with
  the same level of security as you would the original repository.

## Notes
- Some files may have been excluded based on .gitignore rules and Repomix's configuration
- Binary files are not included in this packed representation. Please refer to the Repository Structure section for a complete list of file paths, including binary files
- Files matching these patterns are excluded: Doxyfile
- Files matching patterns in .gitignore are excluded
- Files matching default ignore patterns are excluded
- Files are sorted by Git change count (files with more changes are at the bottom)

# Directory Structure
```
.github/
  workflows/
    doxygen.yml
    tests.yml
cmake/
  PIDControllerConfig.cmake.in
examples/
  basic_usage.cpp
  CMakeLists.txt
include/
  pid_controller/
    clock_interface.hpp
    concepts.hpp
    pid_controller.hpp
    types.hpp
tests/
  CMakeLists.txt
  compile_test.cpp
  test_clock.cpp
  test_concepts.cpp
  test_pid_basic.cpp
  test_types.cpp
.clang-format
.gitignore
.gitmodules
car
CMakeLists.txt
LICENSE
README.md
```

# Files

## File: .github/workflows/doxygen.yml
````yaml
name: Deploy Doxygen docs

on:
  push:
    branches: [ main ]

permissions:
  contents: write

jobs:
  build:
    runs-on: ubuntu-latest

    steps:
      - name: Checkout repo
        uses: actions/checkout@v4

      - name: Install Doxygen
        run: sudo apt-get update && sudo apt-get install -y doxygen

      - name: Generate docs
        run: doxygen Doxyfile

      - name: Disable Jekyll
        run: touch docs/html/.nojekyll

      - name: Deploy to GitHub Pages
        uses: peaceiris/actions-gh-pages@v3
        with:
          github_token: ${{ secrets.GITHUB_TOKEN }}
          publish_dir: ./docs/html
````

## File: .github/workflows/tests.yml
````yaml
name: Build and Run Tests

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]

jobs:
  test:
    runs-on: ubuntu-latest

    steps:
      - name: Checkout repository
        uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake g++

      - name: Configure CMake
        run: cmake -S . -B build -DPID_BUILD_TESTS=ON

      - name: Build
        run: cmake --build build

      - name: Run tests
        run: ctest --test-dir build --verbose --output-on-failure
````

## File: cmake/PIDControllerConfig.cmake.in
````
@PACKAGE_INIT@

include("${CMAKE_CURRENT_LIST_DIR}/PIDControllerTargets.cmake")

check_required_components(PIDController)
````

## File: examples/basic_usage.cpp
````cpp
/**
 * @file basic_usage.cpp
 * @brief Demonstration of the PID Controller library capabilities with CSV export
 */

#include "pid_controller/pid_controller.hpp"
#include "pid_controller/clock_interface.hpp"
#include "pid_controller/types.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <fstream> // Dodano do obsługi plików CSV

struct ThermalPlant {
    double temperature  = 20.0;
    double ambient      = 20.0;
    double heater_power = 5.0;
    double cooling_coeff= 0.3;

    void step(double u, double dt) {
        double heating = heater_power * u;
        double cooling = cooling_coeff * (temperature - ambient);
        temperature += dt * (heating - cooling);
    }
};

static void print_header(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(70, '=') << "\n";
}

static void print_row_header() {
    std::cout << std::left
              << std::setw(6)  << "t[s]"
              << std::setw(10) << "Setpoint"
              << std::setw(10) << "Temp"
              << std::setw(10) << "Error"
              << std::setw(8)  << "Output"
              << std::setw(8)  << "P"
              << std::setw(8)  << "I"
              << std::setw(8)  << "D"
              << "\n"
              << std::string(70, '-') << "\n";
}

static void print_row(double t, double sp, double temp,
                       const pid::PIDController<double, pid::ManualClock>::DebugInfo& dbg) {
    std::cout << std::fixed << std::setprecision(2) << std::left
              << std::setw(6)  << t
              << std::setw(10) << sp
              << std::setw(10) << temp
              << std::setw(10) << dbg.error
              << std::setw(8)  << dbg.output
              << std::setw(8)  << dbg.p_term
              << std::setw(8)  << dbg.i_term
              << std::setw(8)  << dbg.d_term
              << "\n";
}

void demo_basic_pid() {
    print_header("DEMO 1: Basic Parallel PID — Step Setpoint Response");

    pid::PIDController<double, pid::ManualClock>::Config cfg{
        .gains          = {.kp = 0.8, .ki = 0.15, .kd = 0.1},
        .output_limits  = {0.0, 1.0},
        .form           = pid::PIDForm::Parallel,
        .enable_anti_windup = true
    };

    pid::ManualClock clock;
    pid::PIDController<double, pid::ManualClock> pid(cfg, clock);
    ThermalPlant plant;

    const double setpoint = 35.0;
    const double dt_sec   = 0.5;
    const int    steps    = 30;

    // Przygotowanie pliku CSV
    std::ofstream csv("demo1_basic.csv");
    csv << "time,setpoint,temperature,output\n";

    print_row_header();

    for (int i = 0; i < steps; ++i) {
        clock.advance(std::chrono::duration<double>(dt_sec));
        // pid.get_clock().advance(std::chrono::duration<double>(dt_sec));
        double u = pid.compute(setpoint, plant.temperature);
        plant.step(u, dt_sec);
        
        auto dbg = pid.get_debug_info();
        double t = i * dt_sec;
        print_row(t, setpoint, plant.temperature, dbg);
        
        // Zapis do CSV
        csv << t << "," << setpoint << "," << plant.temperature << "," << dbg.output << "\n";
    }
}

void demo_anti_windup() {
    print_header("DEMO 2: Anti-Windup — Preventing Integrator Saturation");

    auto make_pid = [](bool anti_windup) {
        pid::PIDController<double, pid::ManualClock>::Config cfg{
            .gains          = {.kp = 1.0, .ki = 0.5, .kd = 0.0},
            .output_limits  = {0.0, 1.0},
            .form           = pid::PIDForm::Parallel,
            .enable_anti_windup = anti_windup
        };
        pid::ManualClock clk;
        return std::make_pair(pid::PIDController<double, pid::ManualClock>(cfg, clk), clk);
    };

    auto [pid_aw, clk_aw]   = make_pid(true);
    auto [pid_now, clk_now] = make_pid(false);

    ThermalPlant plant_aw, plant_now;
    const double sp = 50.0;
    const double dt = 0.5;

    std::ofstream csv("demo2_anti_windup.csv");
    csv << "time,setpoint,temp_aw,temp_now\n";

    std::cout << std::left << std::setw(6) << "t[s]"
              << std::setw(20) << "Temp (anti-windup)"
              << std::setw(20) << "Temp (no anti-windup)" << "\n"
              << std::string(50, '-') << "\n";

    for (int i = 0; i < 25; ++i) {
        clk_aw.advance(std::chrono::duration<double>(dt));
        clk_now.advance(std::chrono::duration<double>(dt));

        double u_aw  = pid_aw.compute(sp, plant_aw.temperature);
        double u_now = pid_now.compute(sp, plant_now.temperature);

        plant_aw.step(u_aw, dt);
        plant_now.step(u_now, dt);

        double t = i * dt;
        std::cout << std::fixed << std::setprecision(2) << std::left
                  << std::setw(6)  << t
                  << std::setw(20) << plant_aw.temperature
                  << std::setw(20) << plant_now.temperature << "\n";

        csv << t << "," << sp << "," << plant_aw.temperature << "," << plant_now.temperature << "\n";
    }
}

void demo_gain_tuning() {
    print_header("DEMO 3: Runtime Gain Tuning");

    pid::PIDController<double, pid::ManualClock>::Config cfg{
        .gains         = {.kp = 0.3, .ki = 0.05, .kd = 0.0},
        .output_limits = {0.0, 1.0},
        .form          = pid::PIDForm::Parallel
    };
    pid::ManualClock clock;
    pid::PIDController<double, pid::ManualClock> pid(cfg, clock);

    ThermalPlant plant;
    const double sp = 30.0;
    const double dt = 0.5;

    std::ofstream csv("demo3_gain_tuning.csv");
    csv << "time,setpoint,temperature\n";

    std::cout << "Steps 0-14: conservative gains\nSteps 15+:  aggressive gains\n\n";

    for (int i = 0; i < 25; ++i) {
        if (i == 15) {
            pid.set_gains({.kp = 1.2, .ki = 0.30, .kd = 0.05});
            std::cout << "  <<< Gains updated >>>\n";
        }

        clock.advance(std::chrono::duration<double>(dt));
        double u = pid.compute(sp, plant.temperature);
        plant.step(u, dt);

        double t = i * dt;
        std::cout << std::fixed << std::setprecision(2) << std::left
                  << std::setw(6)  << t << std::setw(12) << plant.temperature << "\n";

        csv << t << "," << sp << "," << plant.temperature << "\n";
    }
}

void demo_series_vs_parallel() {
    print_header("DEMO 4: Parallel vs Series (Interactive) Form");

    pid::PIDController<double, pid::ManualClock>::Config cfg{
        .gains         = {.kp = 0.8, .ki = 0.15, .kd = 0.1},
        .output_limits = {0.0, 100.0}, // Zwiększyłem limity, żeby było coś widać
        .form          = pid::PIDForm::Parallel
    };

    // Zegary i kontrolery tworzymy bezpośrednio w funkcji demo
    pid::ManualClock clk_par;
    pid::PIDController<double, pid::ManualClock> pid_par(cfg, clk_par);

    cfg.form = pid::PIDForm::Series;
    pid::ManualClock clk_ser;
    pid::PIDController<double, pid::ManualClock> pid_ser(cfg, clk_ser);

    ThermalPlant p_par, p_ser;
    const double sp = 35.0;
    const double dt = 0.5;

    std::ofstream csv("demo4_series_vs_parallel.csv");
    csv << "time,setpoint,temp_par,temp_ser\n";

    std::cout << "Simulating... (check demo4_series_vs_parallel.csv for results)\n";

    for (int i = 0; i < 40; ++i) { // Więcej kroków
        clk_par.advance(std::chrono::duration<double>(dt));
        clk_ser.advance(std::chrono::duration<double>(dt));

        p_par.step(pid_par.compute(sp, p_par.temperature), dt);
        p_ser.step(pid_ser.compute(sp, p_ser.temperature), dt);

        double t = i * dt;
        csv << t << "," << sp << "," << p_par.temperature << "," << p_ser.temperature << "\n";
        
        // Opcjonalnie: dodaj print, żeby widzieć, że coś się dzieje
        if (i % 10 == 0) std::cout << "T=" << t << "s | Par: " << p_par.temperature << " | Ser: " << p_ser.temperature << "\n";
    }
}

void demo_derivative_filter() {
    print_header("DEMO 5: Derivative Filter — Reducing Noise Sensitivity");

    pid::PIDController<double, pid::ManualClock>::Config cfg{
        .gains                   = {.kp = 0.8, .ki = 0.1, .kd = 0.3},
        .output_limits           = {-50.0, 50.0},
        .derivative_filter_coeff = 0.0 // Dla pierwszego kontrolera
    };

    pid::ManualClock clk_nf;
    pid::PIDController<double, pid::ManualClock> pid_nf(cfg, clk_nf);

    cfg.derivative_filter_coeff = 0.8; // Dla drugiego kontrolera
    pid::ManualClock clk_f;
    pid::PIDController<double, pid::ManualClock> pid_f(cfg, clk_f);

    ThermalPlant plant_nf, plant_f;
    const double sp = 30.0;
    const double dt = 0.5;
    unsigned seed = 42;

    auto noise = [&]() -> double {
        seed = seed * 1664525u + 1013904223u;
        return ((seed >> 16) & 0xFFFF) / 65535.0 * 0.5 - 0.25;
    };

    std::ofstream csv("demo5_derivative_filter.csv");
    csv << "time,d_term_raw,d_term_filtered\n";

    std::cout << "Simulating noise... (check demo5_derivative_filter.csv)\n";

    for (int i = 0; i < 30; ++i) {
        clk_nf.advance(std::chrono::duration<double>(dt));
        clk_f.advance(std::chrono::duration<double>(dt));

        double n = noise();
        pid_nf.compute(sp, plant_nf.temperature + n);
        pid_f.compute(sp, plant_f.temperature + n);

        plant_nf.step(pid_nf.get_debug_info().output, dt);
        plant_f.step(pid_f.get_debug_info().output, dt);

        double t = i * dt;
        csv << t << "," << pid_nf.get_debug_info().d_term << "," << pid_f.get_debug_info().d_term << "\n";
    }
}

int main() {
    demo_basic_pid();
    demo_anti_windup();
    demo_gain_tuning();
    demo_series_vs_parallel();
    demo_derivative_filter();
    std::cout << "\nWszystkie dema zakończone. Wygenerowano pliki CSV do wykresów.\n";
    return 0;
}
````

## File: examples/CMakeLists.txt
````
# Examples for PID Controller library

add_executable(basic_usage basic_usage.cpp)
target_link_libraries(basic_usage PRIVATE PIDController::pid_controller)

message(STATUS "Examples directory configured (no examples built yet)")
````

## File: include/pid_controller/clock_interface.hpp
````cpp
/**
 * @file clock_interface.hpp
 * @author Bartłomiej Kuś
 * @brief Clock abstractions for time management and dependency injection
 * @date 2026-05-05
 */

#pragma once

#include <chrono>

namespace pid {

/**
 * @brief System clock using std::chrono::steady_clock
 * 
 * This is the default clock for production use. It provides monotonic
 * time measurements that are not affected by system clock adjustments.
 */
class SystemClock {
public:
    using time_point = std::chrono::steady_clock::time_point;
    
    /**
     * @brief Get the current time point
     * @return Current steady clock time point
     */
    [[nodiscard]] auto now() const noexcept -> time_point {
        return std::chrono::steady_clock::now();
    }
};

/**
 * @brief Manual clock for testing and simulation
 * 
 * This clock allows manual control of time progression, making it ideal
 * for unit tests and deterministic simulations.
 * 
 * Example:
 * @code
 * ManualClock clock;
 * clock.advance(std::chrono::milliseconds(100));
 * auto t = clock.now(); // Returns time advanced by 100ms
 * @endcode
 */
class ManualClock {
public:
    using time_point = std::chrono::steady_clock::time_point;
    
    /**
     * @brief Construct a manual clock starting at epoch
     */
    constexpr ManualClock() noexcept = default;
    
    /**
     * @brief Construct a manual clock with a specific starting time
     * @param start Initial time point
     */
    explicit constexpr ManualClock(time_point start) noexcept 
        : current_time_(start) {}
    
    /**
     * @brief Get the current simulated time
     * @return Current time point
     */
    [[nodiscard]] constexpr auto now() const noexcept -> time_point {
        return current_time_;
    }
    
    /**
     * @brief Advance the clock by a duration
     * @tparam Rep Duration representation type
     * @tparam Period Duration period type
     * @param duration Amount to advance the clock
     */
    template<typename Rep, typename Period>
    constexpr void advance(std::chrono::duration<Rep, Period> duration) noexcept {
        current_time_ += std::chrono::duration_cast<time_point::duration>(duration);
    }
    
    /**
     * @brief Set the clock to a specific time point
     * @param tp New time point
     */
    constexpr void set_time(time_point tp) noexcept {
        current_time_ = tp;
    }
    
    /**
     * @brief Reset the clock to epoch (zero)
     */
    constexpr void reset() noexcept {
        current_time_ = time_point{};
    }
    
private:
    time_point current_time_{};
};

} // namespace pid
````

## File: include/pid_controller/concepts.hpp
````cpp
/**
 * @file concepts.hpp
 * @author Bartłomiej Kuś 
 * @brief C++20 concepts for type constraints in PID controller library
 * @date 2026-05-05
 */

#pragma once

#include <concepts>
#include <chrono>
#include <type_traits>

namespace pid {

/**
 * @brief Concept requiring a numeric type (integral or floating-point)
 * @tparam T Type to check
 */
template<typename T>
concept Numeric = std::is_arithmetic_v<T>;

/**
 * @brief Concept requiring a floating-point type
 * @tparam T Type to check
 */
template<typename T>
concept FloatingPoint = std::floating_point<T>;

/**
 * @brief Concept for clock types providing time measurements
 * @tparam Clock Type to check
 */
template<typename Clock>
concept ClockInterface = requires(const Clock& c) {
    typename Clock::time_point;
    { c.now() } -> std::same_as<typename Clock::time_point>;
};

} // namespace pid
````

## File: include/pid_controller/pid_controller.hpp
````cpp
/**
 * @file pid_controller.hpp
 * @author Bartłomiej Kuś
 * @brief Modern C++20 PID controller implementation
 * @date 2026-05-05
 * 
 * @details
 * This file implements a generic, header-only PID controller with the following features:
 * - Zero heap allocation (embedded-friendly)
 * - Configurable parallel or series form
 * - Anti-windup protection
 * - Derivative filtering
 * - Dependency injection for time source (testable)
 * 
 * Mathematical Background:
 * 
 * Parallel Form:
 * u(t) = Kp·e(t) + Ki·∫e(t)dt + Kd·de(t)/dt
 * 
 * Series (Interactive) Form:
 * u(t) = Kp·[e(t) + (1/Ti)·∫e(t)dt + Td·de(t)/dt]
 * where Ti = Kp/Ki, Td = Kd/Kp
 */

#pragma once

#include "concepts.hpp"
#include "types.hpp"
#include "clock_interface.hpp"

#include <chrono>
#include <limits>
#include <cmath>
#include <algorithm>

namespace pid {

/**
 * @brief PID controller algorithm form
 */
enum class PIDForm {
    Parallel,  ///< Independent P, I, D terms: u = Kp·e + Ki·∫e + Kd·de/dt
    Series     ///< Interactive form: u = Kp·(e + 1/Ti·∫e + Td·de/dt)
};

/**
 * @brief Generic PID controller implementation
 * 
 * @tparam T Floating-point type for calculations (float, double, long double)
 * @tparam Clock Clock type for time measurements (default: SystemClock)
 * 
 * Example usage:
 * ```cpp
 * using namespace pid;
 * 
 * PIDController<double>::Config config{
 *     .gains = {.kp = 1.0, .ki = 0.1, .kd = 0.05},
 *     .output_limits = {-100.0, 100.0}
 * };
 * 
 * PIDController<double> controller(config);
 * 
 * double setpoint = 100.0;
 * double measurement = 95.0;
 * double control_output = controller.compute(setpoint, measurement);
 * ```
 * 
 * @example
 * @code {.cpp}
 * using namespace pid;
 * 
 * PIDController<double>::Config config{
 *     .gains = {.kp = 1.0, .ki = 0.1, .kd = 0.05},
 *     .output_limits = {-100.0, 100.0}
 * };
 * 
 * PIDController<double> controller(config);
 * 
 * double setpoint = 100.0;
 * double measurement = 95.0;
 * double control_output = controller.compute(setpoint, measurement);
 * @endcode
 */
template<FloatingPoint T = double, ClockInterface Clock = SystemClock>
class PIDController {
public:
    /**
     * @brief PID gain parameters
     */
    struct Gains {
        T kp{0};  ///< Proportional gain
        T ki{0};  ///< Integral gain
        T kd{0};  ///< Derivative gain
        
        /**
         * @brief Validate gains
         * @return true if all gains are non-negative and finite
         */
        [[nodiscard]] constexpr bool is_valid() const noexcept {
            return kp >= 0 && ki >= 0 && kd >= 0 
                && std::isfinite(kp) && std::isfinite(ki) && std::isfinite(kd);
        }
    };

    /**
     * @brief Output saturation limits
     */
    struct Limits {
        T min_output;  ///< Minimum output value
        T max_output;  ///< Maximum output value
        
        /**
         * @brief Validate limits
         * @return true if min < max and both are finite
         */
        [[nodiscard]] constexpr bool is_valid() const noexcept {
            return min_output < max_output 
                && std::isfinite(min_output) && std::isfinite(max_output);
        }
    };

    /**
     * @brief Complete PID controller configuration
     */
    struct Config {
        Gains gains;                              ///< PID gains
        Limits output_limits{-1.0, 1.0};         ///< Output saturation limits
        PIDForm form{PIDForm::Parallel};          ///< Algorithm form
        T derivative_filter_coeff{0.0};           ///< Low-pass filter coefficient (0 = no filter, range: [0,1])
        bool enable_anti_windup{true};            ///< Enable integral anti-windup
        
        /**
         * @brief Validate complete configuration
         * @return true if configuration is valid
         */
        [[nodiscard]] constexpr bool is_valid() const noexcept {
            return gains.is_valid() 
                && output_limits.is_valid()
                && derivative_filter_coeff >= 0 
                && derivative_filter_coeff <= 1;
        }
    };

    /**
     * @brief Diagnostic information for debugging
     */
    struct DebugInfo {
        T p_term{0};      ///< Proportional term value
        T i_term{0};      ///< Integral term value
        T d_term{0};      ///< Derivative term value
        T error{0};       ///< Current error
        T output{0};      ///< Controller output
        Duration dt{0};   ///< Time step
    };

    /**
     * @brief Construct a PID controller
     * @param config Controller configuration
     * @param clock Clock instance for time measurements
     * @throws std::invalid_argument if configuration is invalid
     */
    explicit PIDController(const Config& config, Clock& clock)
        : config_(config)
        , clock_(clock)
    {
        if (!config_.is_valid()) {
            throw std::invalid_argument("Invalid PID configuration");
        }
        reset();
    }

    template<typename C = Clock>
    requires std::is_same_v<C, SystemClock>
    explicit PIDController(const Config& config)
        : config_(config)
        , clock_(default_system_clock()) // Pobieramy statyczną instancję
    {
        if (!config_.is_valid()) throw std::invalid_argument("Invalid PID configuration");
        reset();
    }

    /**
     * @brief Compute control output
     * 
     * @param setpoint Desired value (reference)
     * @param process_variable Current measured value
     * @return Control output (clamped to output_limits)
     * 
     * This is the main function called in each control loop iteration.
     * It computes the error, updates internal state, and returns the control signal.
     */
    [[nodiscard]] T compute(T setpoint, T process_variable) noexcept {
        // Calculate error: e(t) = setpoint - measurement
        const T error = setpoint - process_variable;
        
        // Get time delta
        const auto current_time = clock_.now();
        Duration dt{0};
        
        if (first_run_) {
            first_run_ = false;
            last_time_ = current_time;
            previous_error_ = error;
            debug_info_.error = error;
            debug_info_.dt = dt;
            return 0; // First run, no valid dt yet
        }
        
        dt = std::chrono::duration_cast<Duration>(current_time - last_time_);
        last_time_ = current_time;
        
        // Guard against zero or negative time steps
        if (dt.count() <= 0) {
            return previous_output_;
        }
        
        // Compute PID based on selected form
        T output;
        if (config_.form == PIDForm::Parallel) {
            output = compute_parallel(error, dt);
        } else {
            output = compute_series(error, dt);
        }
        
        // Clamp output to limits
        output = std::clamp(output, config_.output_limits.min_output, 
                                    config_.output_limits.max_output);
        
        // Anti-windup: back-calculate integral if output is saturated
        if (config_.enable_anti_windup) {
            apply_anti_windup(output);
        }
        
        // Update state for next iteration
        previous_error_ = error;
        previous_output_ = output;
        
        // Update debug info
        debug_info_.error = error;
        debug_info_.output = output;
        debug_info_.dt = dt;
        
        return output;
    }

    /**
     * @brief Reset controller to initial state
     * 
     * Clears integral accumulator, previous error, and resets timing.
     * Call this when changing setpoint significantly or restarting control.
     */
    void reset() noexcept {
        integral_ = 0;
        previous_error_ = 0;
        previous_derivative_ = 0;
        previous_output_ = 0;
        first_run_ = true;
        debug_info_ = {};
    }

    /**
     * @brief Update PID gains during runtime
     * @param gains New gain values
     */
    void set_gains(const Gains& gains) noexcept {
        if (gains.is_valid()) {
            config_.gains = gains;
        }
    }

    /**
     * @brief Get current PID gains
     * @return Current gains
     */
    [[nodiscard]] Gains get_gains() const noexcept {
        return config_.gains;
    }

    /**
     * @brief Update output limits during runtime
     * @param limits New limit values
     */
    void set_output_limits(const Limits& limits) noexcept {
        if (limits.is_valid()) {
            config_.output_limits = limits;
        }
    }

    /**
     * @brief Get current output limits
     * @return Current limits
     */
    [[nodiscard]] Limits get_output_limits() const noexcept {
        return config_.output_limits;
    }

    /**
     * @brief Get diagnostic information from last compute() call
     * @return Debug information structure
     */
    [[nodiscard]] DebugInfo get_debug_info() const noexcept {
        return debug_info_;
    }

    /**
     * @brief Get the clock object
     * 
     * @return Clock& 
     */
    [[nodiscard]] Clock& get_clock() noexcept { return clock_; }

private:
    Config config_;
    Clock& clock_;
    
    // Internal state
    T integral_{0};
    T previous_error_{0};
    T previous_derivative_{0};
    T previous_output_{0};
    typename Clock::time_point last_time_;
    bool first_run_{true};
    
    // Debug information
    mutable DebugInfo debug_info_;

    /**
     * @brief Compute PID in parallel form
     * 
     * u = Kp·e + Ki·∫e·dt + Kd·de/dt
     */
    [[nodiscard]] T compute_parallel(T error, Duration dt) noexcept {
        const T dt_sec = dt.count();
        
        // Proportional term
        const T p_term = config_.gains.kp * error;
        
        // Integral term (trapezoidal integration)
        integral_ += error * dt_sec;
        const T i_term = config_.gains.ki * integral_;
        
        // Derivative term (with optional filtering)
        T derivative = (error - previous_error_) / dt_sec;
        derivative = apply_derivative_filter(derivative, dt);
        const T d_term = config_.gains.kd * derivative;
        
        // Update debug info
        debug_info_.p_term = p_term;
        debug_info_.i_term = i_term;
        debug_info_.d_term = d_term;
        
        return p_term + i_term + d_term;
    }

    /**
     * @brief Compute PID in series (interactive) form
     * 
     * u = Kp·(e + 1/Ti·∫e·dt + Td·de/dt)
     * where Ti = Kp/Ki, Td = Kd/Kp
     */
    [[nodiscard]] T compute_series(T error, Duration dt) noexcept {
        const T dt_sec = dt.count();
        
        // Integral term
        integral_ += error * dt_sec;
        T i_contribution = 0;
        if (config_.gains.ki > 0 && config_.gains.kp > 0) {
            const T Ti = config_.gains.kp / config_.gains.ki;
            i_contribution = integral_ / Ti;
        }
        
        // Derivative term
        T derivative = (error - previous_error_) / dt_sec;
        derivative = apply_derivative_filter(derivative, dt);
        T d_contribution = 0;
        if (config_.gains.kp > 0) {
            const T Td = config_.gains.kd / config_.gains.kp;
            d_contribution = Td * derivative;
        }
        
        const T output = config_.gains.kp * (error + i_contribution + d_contribution);
        
        // Update debug info
        debug_info_.p_term = config_.gains.kp * error;
        debug_info_.i_term = config_.gains.kp * i_contribution;
        debug_info_.d_term = config_.gains.kp * d_contribution;
        
        return output;
    }

    /**
     * @brief Apply low-pass filter to derivative term
     * 
     * Filtered derivative = α·previous + (1-α)·current
     * where α = τ/(τ+dt), τ is filter time constant
     */
    [[nodiscard]] T apply_derivative_filter(T raw_derivative, Duration dt) noexcept {
        if (config_.derivative_filter_coeff <= 0) {
            return raw_derivative; // No filtering
        }
        
        const T alpha = config_.derivative_filter_coeff;
        const T filtered = alpha * previous_derivative_ + (1 - alpha) * raw_derivative;
        previous_derivative_ = filtered;
        
        return filtered;
    }

    /**
     * @brief Apply anti-windup mechanism (clamping method)
     * 
     * If output is saturated, stop integral accumulation in that direction
     */
    void apply_anti_windup(T clamped_output) noexcept {
        // If output was clamped, back-calculate integral
        const T unclamped_i_term = debug_info_.i_term;
        const T actual_i_term = clamped_output - debug_info_.p_term - debug_info_.d_term;
        
        // Update integral to prevent further windup
        if (config_.gains.ki > 0) {
            integral_ = actual_i_term / config_.gains.ki;
        }
    }

    static SystemClock& default_system_clock() {
        static SystemClock instance;
        return instance;
    }
};

} // namespace pid
````

## File: include/pid_controller/types.hpp
````cpp
/**
 * @file types.hpp
 * @author Bartłomiej Kuś
 * @brief Common types and error handling for PID controller
 * @date 2026-05-05
 */

#pragma once

#include <system_error>
#include <string>
#include <chrono>

// Check for std::expected availability (C++23)
#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
    #include <expected>
    namespace pid {
        template<typename T, typename E>
        using Expected = std::expected<T, E>;
    }
#else
    // Fallback: simple Result type for C++20
    #include <variant>
    #include <type_traits>
    
    namespace pid {
        template<typename T, typename E>
        class Expected {
        private:
            std::variant<T, E> data_;
            
        public:
            constexpr Expected(const T& value) : data_(value) {}
            constexpr Expected(T&& value) : data_(std::move(value)) {}
            constexpr Expected(const E& error) : data_(error) {}
            constexpr Expected(E&& error) : data_(std::move(error)) {}
            
            constexpr bool has_value() const noexcept {
                return std::holds_alternative<T>(data_);
            }
            
            constexpr explicit operator bool() const noexcept {
                return has_value();
            }
            
            constexpr T& value() & {
                return std::get<T>(data_);
            }
            
            constexpr const T& value() const& {
                return std::get<T>(data_);
            }
            
            constexpr T&& value() && {
                return std::get<T>(std::move(data_));
            }
            
            constexpr const E& error() const& {
                return std::get<E>(data_);
            }
            
            constexpr T value_or(T&& default_value) const& {
                return has_value() ? value() : std::forward<T>(default_value);
            }
        };
    }
#endif

namespace pid {

/**
 * @brief Error codes for PID controller operations
 */
enum class PIDError {
    None = 0,
    InvalidGains,           ///< One or more PID gains are invalid (negative or NaN)
    InvalidOutputLimits,    ///< Output limits are invalid (min >= max)
    InvalidTimeStep,        ///< Time step is zero or negative
    InvalidFilterCoeff,     ///< Derivative filter coefficient is out of range [0, 1]
    NotInitialized,         ///< Controller not properly initialized
    NumericOverflow         ///< Numeric overflow detected in calculations
};

/**
 * @brief Error category for PID errors
 */
class PIDErrorCategory : public std::error_category {
public:
    const char* name() const noexcept override {
        return "pid_controller";
    }
    
    std::string message(int ev) const override {
        switch (static_cast<PIDError>(ev)) {
            case PIDError::None:
                return "No error";
            case PIDError::InvalidGains:
                return "Invalid PID gains: must be non-negative and finite";
            case PIDError::InvalidOutputLimits:
                return "Invalid output limits: min must be less than max";
            case PIDError::InvalidTimeStep:
                return "Invalid time step: must be positive";
            case PIDError::InvalidFilterCoeff:
                return "Invalid derivative filter coefficient: must be in range [0, 1]";
            case PIDError::NotInitialized:
                return "PID controller not initialized";
            case PIDError::NumericOverflow:
                return "Numeric overflow in PID calculation";
            default:
                return "Unknown PID error";
        }
    }
};

/**
 * @brief Get the global PID error category instance
 */
inline const PIDErrorCategory& pid_error_category() {
    static PIDErrorCategory instance;
    return instance;
}

/**
 * @brief Create an error_code from a PIDError
 */
inline std::error_code make_error_code(PIDError e) {
    return {static_cast<int>(e), pid_error_category()};
}

/**
 * @brief Result type for operations that may fail
 * @tparam T Success value type
 */
template<typename T>
using Result = Expected<T, std::error_code>;

/**
 * @brief Duration type using double precision seconds
 */
using Duration = std::chrono::duration<double>;

} // namespace pid

// Enable error_code creation from PIDError
namespace std {
    template<>
    struct is_error_code_enum<pid::PIDError> : true_type {};
}
````

## File: tests/CMakeLists.txt
````
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../googletest googletest_build)

add_executable(compile_test compile_test.cpp)
target_link_libraries(compile_test PRIVATE PIDController::pid_controller)
add_test(NAME CompileTest COMMAND compile_test)


add_executable(pid_tests
    test_concepts.cpp
    test_clock.cpp
    test_types.cpp
    test_pid_basic.cpp
)

target_link_libraries(pid_tests 
    PRIVATE 
    PIDController::pid_controller
    gtest_main
    gtest
)

add_test(NAME PIDControllerTests COMMAND pid_tests)
````

## File: tests/compile_test.cpp
````cpp
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
    return 0; // TODO: add unit tests for parallel and series PID
}
````

## File: tests/test_clock.cpp
````cpp
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
````

## File: tests/test_concepts.cpp
````cpp
#include <gtest/gtest.h>
#include "pid_controller/concepts.hpp"

TEST(ConceptsTest, FloatingPoint) {
    EXPECT_TRUE(pid::FloatingPoint<double>);
    EXPECT_TRUE(pid::FloatingPoint<float>);
    EXPECT_FALSE(pid::FloatingPoint<int>);
}

TEST(ConceptsTest, Numeric) {
    EXPECT_TRUE(pid::Numeric<int>);
    EXPECT_TRUE(pid::Numeric<double>);
    EXPECT_TRUE(pid::Numeric<float>);
}

// TEST(ConceptsTest, ClockInterface) {
//     EXPECT_TRUE(pid::ClockInterface<pid::SystemClock>);
//     EXPECT_TRUE(pid::ClockInterface<pid::ManualClock>);
// }
````

## File: tests/test_pid_basic.cpp
````cpp
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
````

## File: tests/test_types.cpp
````cpp
#include <gtest/gtest.h>
#include "pid_controller/types.hpp"

TEST(TypesTest, ErrorCodeMessage) {
    auto error = pid::make_error_code(pid::PIDError::InvalidGains);
    EXPECT_FALSE(error.message().empty());
}

TEST(TypesTest, ExpectedValue) {
    pid::Expected<int, std::error_code> value(102);
    EXPECT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), 102);
}

TEST(TypesTest, ExpectedError) {
    auto error = pid::make_error_code(pid::PIDError::InvalidGains);
    pid::Expected<int, std::error_code> value(error);

    EXPECT_FALSE(value.has_value());
    EXPECT_EQ(value.error(), error);
}

TEST(TypesTest, ValueOr) {
    auto error = pid::make_error_code(pid::PIDError::InvalidGains);
    pid::Expected<int, std::error_code> value(error);

    EXPECT_EQ(value.value_or(10), 10);
}
````

## File: .clang-format
````
---
Language: Cpp
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 100
AccessModifierOffset: -4
NamespaceIndentation: None
PointerAlignment: Left
AllowShortFunctionsOnASingleLine: Inline
AllowShortIfStatementsOnASingleLine: Never
AlwaysBreakTemplateDeclarations: Yes
BreakBeforeBraces: Attach
Standard: c++20
````

## File: .gitignore
````
# Build directories
build/
cmake-build-*/
out/

# IDE
.vscode/
.idea/
*.swp
*.swo
*~

# Compiled files
*.o
*.so
*.a
*.exe

# CMake
CMakeCache.txt
CMakeFiles/
cmake_install.cmake
install_manifest.txt

# Documentation
docs/


# Coverage
*.gcov
*.gcda
*.gcno
coverage/

# OS
.DS_Store
Thumbs.db
````

## File: .gitmodules
````
[submodule "googletest"]
	path = googletest
	url = https://github.com/google/googletest.git
````

## File: CMakeLists.txt
````
cmake_minimum_required(VERSION 3.20)

project(PIDController
    VERSION 1.0.0
    DESCRIPTION "Modern C++20 header-only PID controller library"
    LANGUAGES CXX
)

# C++20 wymagane
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Opcje projektu
option(PID_BUILD_EXAMPLES "Build example programs" ON)
option(PID_BUILD_TESTS "Build unit tests" ON)
option(PID_BUILD_DOCS "Build documentation" OFF)
option(PID_ENABLE_WARNINGS "Enable compiler warnings" ON)

# Header-only library target
add_library(pid_controller INTERFACE)
add_library(PIDController::pid_controller ALIAS pid_controller)

target_include_directories(pid_controller
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(pid_controller INTERFACE cxx_std_20)

# Compiler warnings
if(PID_ENABLE_WARNINGS)
    # Compiler warnings
    if(MSVC)
        target_compile_options(pid_controller INTERFACE /W4)
    else()
        target_compile_options(pid_controller INTERFACE 
            -Wall -Wextra -Wpedantic
        )
    endif()
endif()

# Examples
if(PID_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(PID_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Documentation
if(PID_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    else()
        message(WARNING "Doxygen not found, documentation will not be built")
    endif()
endif()

# Installation
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS pid_controller
    EXPORT PIDControllerTargets
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT PIDControllerTargets
    FILE PIDControllerTargets.cmake
    NAMESPACE PIDController::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PIDController
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PIDControllerConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/PIDControllerConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PIDController
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/PIDControllerConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/PIDControllerConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/PIDControllerConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PIDController
)
````

## File: LICENSE
````
MIT License

Copyright (c) 2026 [Bartłomiej Kuś]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
````

## File: README.md
````markdown
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
````
