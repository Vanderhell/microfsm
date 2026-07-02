cmake_minimum_required(VERSION 3.21)

project(microfsm_find_package_cpp_consumer LANGUAGES CXX)

find_package(microfsm CONFIG REQUIRED)

foreach(standard IN ITEMS 11 17 20)
    add_executable("microfsm_find_package_cpp${standard}" main.cpp)
    target_link_libraries("microfsm_find_package_cpp${standard}" PRIVATE microfsm::microfsm)
    set_target_properties("microfsm_find_package_cpp${standard}" PROPERTIES
        CXX_STANDARD ${standard}
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )
endforeach()
