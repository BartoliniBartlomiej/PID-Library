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