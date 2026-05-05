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