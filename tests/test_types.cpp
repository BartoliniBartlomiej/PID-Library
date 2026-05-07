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