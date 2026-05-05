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