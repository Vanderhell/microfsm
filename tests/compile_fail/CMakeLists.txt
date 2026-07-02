function(mfsm_add_compile_fail_test name source expected_regex)
    add_test(
        NAME "${name}"
        COMMAND
            "${CMAKE_COMMAND}"
            "-DSOURCE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/${source}"
            "-DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/${name}"
            "-DINCLUDE_DIR=${PROJECT_SOURCE_DIR}/include"
            "-DGENERATED_INCLUDE_DIR=${MFSM_GENERATED_INCLUDE_DIR}"
            "-DEXPECTED_REGEX=${expected_regex}"
            "-DCOMPILER_PATH=${CMAKE_C_COMPILER}"
            "-DCOMPILER_ID=${CMAKE_C_COMPILER_ID}"
            "-P"
            "${CMAKE_CURRENT_SOURCE_DIR}/check_compile_fail.cmake"
    )
endfunction()

mfsm_add_compile_fail_test(microfsm.compile_fail.invalid_enable_names invalid_enable_names.c "MFSM_ENABLE_NAMES")
mfsm_add_compile_fail_test(microfsm.compile_fail.invalid_enable_trace invalid_enable_trace.c "MFSM_ENABLE_TRACE")
mfsm_add_compile_fail_test(microfsm.compile_fail.invalid_max_states invalid_max_states.c "MFSM_MAX_STATES")
mfsm_add_compile_fail_test(microfsm.compile_fail.invalid_max_transitions invalid_max_transitions.c "MFSM_MAX_TRANSITIONS")
mfsm_add_compile_fail_test(microfsm.compile_fail.conflicting_generated_configuration conflicting_generated_configuration.c "conflicts with the installed microfsm configuration")
mfsm_add_compile_fail_test(microfsm.compile_fail.unsupported_abi_override unsupported_abi_override.c "conflicts with the installed microfsm configuration")
