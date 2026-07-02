cmake_minimum_required(VERSION 3.21)

project(microfsm_add_subdirectory_consumer LANGUAGES C)

set(MICROFSM_SOURCE_DIR "" CACHE PATH "Path to the microfsm source tree")
if(MICROFSM_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "MICROFSM_SOURCE_DIR must point at the microfsm source tree")
endif()

add_subdirectory("${MICROFSM_SOURCE_DIR}" microfsm-build)

add_executable(microfsm_add_subdirectory_consumer main.c)
target_link_libraries(microfsm_add_subdirectory_consumer PRIVATE microfsm::microfsm)
target_compile_features(microfsm_add_subdirectory_consumer PRIVATE c_std_99)
