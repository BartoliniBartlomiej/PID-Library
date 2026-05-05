# Unit tests for PID Controller

add_executable(compile_test compile_test.cpp)
target_link_libraries(compile_test PRIVATE PIDController::pid_controller)

add_test(NAME CompileTest COMMAND compile_test)