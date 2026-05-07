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
    explicit PIDController(const Config& config, Clock clock = Clock{})
        : config_(config)
        , clock_(std::move(clock))
    {
        if (!config_.is_valid()) {
            throw std::invalid_argument("Invalid PID configuration");
        }
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
    Clock clock_;
    
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
};

} // namespace pid