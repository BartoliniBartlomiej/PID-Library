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