# Verification Status

This repository snapshot is not fully verified.

## Verified from repository evidence

- Public headers and implementation exist.
- C99 runtime tests exist in `tests/test_all.c`.
- Compile-fail fixtures exist in `tests/compile_fail/`.
- C, C++11, C++17, and C++20 consumer fixtures exist in `tests/consumers/`.
- CMake package and tag-only release workflow files exist.

## Not verified

- No maintainer-run build output is recorded here.
- No maintainer-run CTest or sanitizer output is recorded here.
- No real hardware execution evidence is recorded here.
- No tagged release evidence is recorded here.
- No formal ABI audit artifact is recorded here.

## Release blockers

- Run the runtime harness and compile-fail fixtures on supported toolchains.
- Run names-disabled and trace-disabled builds.
- Run ASan, UBSan, and any available TSan/static-analysis gates.
- Verify MSVC and ARM compile-only jobs from actual CI results.
- Confirm package install and standalone consumer builds from real build logs.

## Policy

Do not mark the project release-ready from documentation alone. A real audit must supply the missing evidence.
