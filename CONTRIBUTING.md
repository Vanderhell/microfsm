# Contributing to microfsm

Thank you for considering a contribution. This document explains how to
contribute effectively.

## Scope

microfsm is intentionally small and focused. Contributions that align with
the project's philosophy are welcome:

- Bug fixes.
- Documentation improvements.
- New examples and usage patterns.
- Test coverage improvements.
- Platform-specific porting notes.
- Performance improvements that do not increase complexity.

Contributions that are **out of scope**:

- Hierarchical state machines (HSM). This is a flat FSM library by design.
- Built-in event queues. These are platform-specific.
- Dynamic allocation. The library is zero-alloc by design.
- External dependencies. The library has zero dependencies by design.

## How to contribute

1. **Open an issue first** — describe the bug or feature. Let's discuss
   before you write code.
2. **Fork and branch** — create a branch named `fix/description` or
   `feat/description`.
3. **Write tests** — every change should include tests. See `tests/`.
4. **Follow the style** — see below.
5. **Submit a PR** — reference the issue number.

## Code style

- **C99** — no compiler extensions, no C11+ features.
- **4-space indentation**, no tabs.
- **`mfsm_` prefix** on all public symbols.
- **`static`** on all internal functions.
- **`const` correctness** — use `const` wherever possible.
- **No dynamic allocation** — ever.
- **No global mutable state** — the engine is pure; all state is in `mfsm_t`.
- **Comments:** explain *why*, not *what*. The code should be self-explanatory.

## Testing

```bash
cd tests
make          # build and run
make coverage # with lcov
```

All tests must pass on both GCC and Clang. If you add a new test file,
add it to the Makefile.

## Commit messages

Use conventional format:

```
fix: handle NULL guard in priority scan
feat: add mfsm_transition_count() query
docs: add Zephyr RTOS example to porting guide
test: cover self-transition with guard
```

## License

By contributing, you agree that your contributions will be licensed under the
MIT License.
