cmake_minimum_required(VERSION 3.21)

project(microfsm_find_package_c_consumer LANGUAGES C)

find_package(microfsm CONFIG REQUIRED)

add_executable(microfsm_find_package_c_consumer main.c)
target_link_libraries(microfsm_find_package_c_consumer PRIVATE microfsm::microfsm)
target_compile_features(microfsm_find_package_c_consumer PRIVATE c_std_99)
