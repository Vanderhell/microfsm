cmake_minimum_required(VERSION 3.21)

project(microfsm VERSION 0.1.0 LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(MFSM_BUILD_TESTS "Build microfsm tests and consumer fixtures" OFF)
option(MFSM_STRICT_WARNINGS "Enable strict warnings for project-owned targets" ON)
option(MFSM_ENABLE_NAMES "Enable state-name query support" ON)
option(MFSM_ENABLE_TRACE "Enable trace registration support" ON)

set(MFSM_MAX_STATES "32" CACHE STRING "Maximum number of states in one definition")
set(MFSM_MAX_TRANSITIONS "64" CACHE STRING "Maximum number of transitions in one definition")

if(NOT (MFSM_ENABLE_NAMES STREQUAL "ON" OR MFSM_ENABLE_NAMES STREQUAL "OFF"))
    message(FATAL_ERROR "MFSM_ENABLE_NAMES must be ON or OFF")
endif()

if(NOT (MFSM_ENABLE_TRACE STREQUAL "ON" OR MFSM_ENABLE_TRACE STREQUAL "OFF"))
    message(FATAL_ERROR "MFSM_ENABLE_TRACE must be ON or OFF")
endif()

math(EXPR MFSM_MAX_STATES_VALUE "${MFSM_MAX_STATES}")
math(EXPR MFSM_MAX_TRANSITIONS_VALUE "${MFSM_MAX_TRANSITIONS}")

if(MFSM_MAX_STATES_VALUE LESS 1 OR MFSM_MAX_STATES_VALUE GREATER 255)
    message(FATAL_ERROR "MFSM_MAX_STATES must be in the range 1..255")
endif()

if(MFSM_MAX_TRANSITIONS_VALUE LESS 0 OR MFSM_MAX_TRANSITIONS_VALUE GREATER 65535)
    message(FATAL_ERROR "MFSM_MAX_TRANSITIONS must be in the range 0..65535")
endif()

if(MFSM_ENABLE_NAMES)
    set(MFSM_ENABLE_NAMES_VALUE 1)
else()
    set(MFSM_ENABLE_NAMES_VALUE 0)
endif()

if(MFSM_ENABLE_TRACE)
    set(MFSM_ENABLE_TRACE_VALUE 1)
else()
    set(MFSM_ENABLE_TRACE_VALUE 0)
endif()

set(MFSM_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
file(MAKE_DIRECTORY "${MFSM_GENERATED_INCLUDE_DIR}")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/mfsm_config.h.in"
    "${MFSM_GENERATED_INCLUDE_DIR}/mfsm_config.h"
    @ONLY
)

add_library(microfsm STATIC src/mfsm.c)
add_library(microfsm::microfsm ALIAS microfsm)

target_compile_features(microfsm PUBLIC c_std_99)
target_include_directories(microfsm
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${MFSM_GENERATED_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_target_properties(microfsm PROPERTIES
    EXPORT_NAME microfsm
    OUTPUT_NAME microfsm
)

if(MFSM_STRICT_WARNINGS)
    if(MSVC)
        target_compile_options(microfsm PRIVATE /W4 /WX)
    else()
        target_compile_options(microfsm PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
    endif()
endif()

install(
    TARGETS microfsm
    EXPORT microfsmTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/include/mfsm.h"
        "${MFSM_GENERATED_INCLUDE_DIR}/mfsm_config.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/microfsmConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/microfsmConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/microfsmConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/microfsm"
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/microfsmConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/microfsmConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/microfsm"
)

install(
    EXPORT microfsmTargets
    NAMESPACE microfsm::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/microfsm"
)

if(MFSM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
