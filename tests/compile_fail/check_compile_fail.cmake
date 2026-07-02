cmake_minimum_required(VERSION 3.21)

file(MAKE_DIRECTORY "${BINARY_DIR}")
set(OBJECT_FILE "${BINARY_DIR}/compile_fail_test.obj")

if(COMPILER_ID STREQUAL "MSVC")
    execute_process(
        COMMAND
            "${COMPILER_PATH}"
            /nologo
            /c
            "/I${INCLUDE_DIR}"
            "/I${GENERATED_INCLUDE_DIR}"
            "/Fo${OBJECT_FILE}"
            "${SOURCE_FILE}"
        RESULT_VARIABLE COMPILE_RESULT
        OUTPUT_VARIABLE BUILD_STDOUT
        ERROR_VARIABLE BUILD_STDERR
    )
else()
    execute_process(
        COMMAND
            "${COMPILER_PATH}"
            -std=c99
            -c
            "-I${INCLUDE_DIR}"
            "-I${GENERATED_INCLUDE_DIR}"
            -o
            "${OBJECT_FILE}"
            "${SOURCE_FILE}"
        RESULT_VARIABLE COMPILE_RESULT
        OUTPUT_VARIABLE BUILD_STDOUT
        ERROR_VARIABLE BUILD_STDERR
    )
endif()

set(BUILD_OUTPUT "${BUILD_STDOUT}\n${BUILD_STDERR}")

if(COMPILE_RESULT EQUAL 0)
    message(FATAL_ERROR "compile-fail test unexpectedly succeeded:\n${BUILD_OUTPUT}")
endif()

if(NOT BUILD_OUTPUT MATCHES "${EXPECTED_REGEX}")
    message(FATAL_ERROR "compile-fail test failed for the wrong reason:\n${BUILD_OUTPUT}")
endif()
