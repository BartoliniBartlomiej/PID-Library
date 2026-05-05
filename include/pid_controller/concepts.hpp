/**
 * @file concepts.hpp
 * @author Bartłomiej Kuś 
 * @brief C++20 concepts for type constraints in PID cosntroller library
 * @date 2026-05-05
 * 
 */

#pragma once

#include <concepts>
#include <chrono>

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
    * 
    * Used for PID calculations to ensure sufficient precision.
    */
    template<typename T>
    concept FloatingPoint = std::floating_point<T>;

    /**
     * @brief Concept for clock types providing time measurements
     * @tparam Clock Type to check
     *
     * A valid clock must provide a now() method returning a time point
     * that can be converted to nanoseconds for precise time delta calculations.
     */
    template<typename Clock>
    concept ClockInterface = requires(Clock c) {
        { c.now() } -> std::same_as<std::chrono::steady_clock::time_point>; 
    }
}; // namespace pid