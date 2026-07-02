# Contributing to microfsm

microfsm accepts corrective changes that preserve its small, explicit contract.

## Project rules

- Minimum language level is C99.
- The library stays a flat FSM engine.
- Caller-owned memory only. No heap use.
- No dynamic registration, reflection, schedulers, timers, or hidden globals.
- Do not weaken negative tests, compile-fail fixtures, or consumer fixtures.
- Do not create tags or releases unless explicitly requested by the maintainer.

## Change expectations

- Keep the public ABI fixed-width and layout-stable.
- Preserve immutable definition semantics. Shared definitions may be reused across instances but must not be mutated while instances are active.
- Keep runtime validation active even when assertions compile out.
- Document thread, ISR, callback, and termination limits when behavior changes.
- Add or update tests for every contract change.

## Coding expectations

- Use standard C99 only for library code.
- Prefer straight-line code over clever abstractions.
- Keep callbacks synchronous and caller-owned.
- Avoid hidden state and side effects outside `mfsm_t` and caller data.

## Verification

- The repository includes a C99 runtime harness in `tests/test_all.c`.
- Compile-fail fixtures live under `tests/compile_fail/`.
- Consumer fixtures live under `tests/consumers/`.
- Maintainers should review [docs/VERIFICATION.md](docs/VERIFICATION.md) before claiming release readiness.

## Security reports

Use [SECURITY.md](SECURITY.md) for vulnerability reporting expectations.

## Release policy

Tag-based release behavior is documented in [docs/RELEASES.md](docs/RELEASES.md). Changelog updates belong in [CHANGELOG.md](CHANGELOG.md).
