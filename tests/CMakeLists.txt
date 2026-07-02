add_executable(microfsm_tests test_all.c)
target_link_libraries(microfsm_tests PRIVATE microfsm::microfsm)

if(MSVC)
    target_compile_options(microfsm_tests PRIVATE /W4 /WX)
else()
    target_compile_options(microfsm_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME microfsm.contract COMMAND microfsm_tests)

add_subdirectory(compile_fail)
