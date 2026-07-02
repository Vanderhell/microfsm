cmake_minimum_required(VERSION 3.21)

file(MAKE_DIRECTORY "${BINARY_DIR}")

try_compile(
    COMPILE_OK
    "${BINARY_DIR}"
    "${SOURCE_FILE}"
    CMAKE_FLAGS
        "-DCMAKE_C_STANDARD=99"
        "-DINCLUDE_DIRECTORIES=${INCLUDE_DIR};${GENERATED_INCLUDE_DIR}"
    OUTPUT_VARIABLE BUILD_OUTPUT
)

if(COMPILE_OK)
    message(FATAL_ERROR "compile-fail test unexpectedly succeeded:\n${BUILD_OUTPUT}")
endif()

if(NOT BUILD_OUTPUT MATCHES "${EXPECTED_REGEX}")
    message(FATAL_ERROR "compile-fail test failed for the wrong reason:\n${BUILD_OUTPUT}")
endif()
