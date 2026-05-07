# Examples for PID Controller library

add_executable(basic_usage basic_usage.cpp)
target_link_libraries(basic_usage PRIVATE PIDController::pid_controller)

message(STATUS "Examples directory configured (no examples built yet)")