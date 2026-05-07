add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../googletest googletest_build)

add_executable(compile_test compile_test.cpp)
target_link_libraries(compile_test PRIVATE PIDController::pid_controller)
add_test(NAME CompileTest COMMAND compile_test)


add_executable(pid_tests
    test_concepts.cpp

)

target_link_libraries(pid_tests 
    PRIVATE 
    PIDController::pid_controller
    gtest_main
    gtest
)

add_test(NAME PIDControllerTests COMMAND pid_tests)