# microfsm

Table-driven finite state machine engine for C99 applications that need a small, caller-owned runtime object and immutable machine definitions.

[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![CI](https://github.com/Vanderhell/microfsm/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/Vanderhell/microfsm/actions/workflows/ci.yml)

microfsm is not claimed production-ready in this repository snapshot. The API and ABI contract have been tightened, but release evidence, full CI proof, and platform verification still need a maintainer-run audit.

## Scope

- Flat FSMs only. No hierarchy, schedulers, timers, queues, reflection, code generation, or dynamic registration.
- Caller-owned memory only. The library does not allocate, free, close, unlock, or persist anything.
- Immutable definitions. State and transition arrays must outlive every `mfsm_t` instance that uses them.
- Explicit error reporting. Query APIs return status codes instead of ambiguous sentinels.

## Public contract summary

- Fixed-width public ABI types:
  - `mfsm_err_t` is `int32_t`
  - `mfsm_state_id`, `mfsm_event_id`, and `mfsm_trace_kind_t` are `uint8_t`
- `mfsm_state_t`, `mfsm_transition_t`, `mfsm_def_t`, and `mfsm_t` keep stable public layouts regardless of `MFSM_ENABLE_NAMES` and `MFSM_ENABLE_TRACE`.
- `mfsm_init()` validates every successful definition before the instance becomes active.
- Same-instance reentrancy from callbacks is rejected with `MFSM_ERR_BUSY`.
- `mfsm_has_transition()` reports table presence only. It does not evaluate guards.
- Reset tracing uses an explicit trace kind instead of a synthetic event value.

## Quick start

```c
#include "mfsm.h"

enum {
    ST_IDLE = 0,
    ST_BUSY = 1,
    ST_COUNT = 2
};

enum {
    EV_START = 1,
    EV_DONE = 2
};

typedef struct {
    unsigned int completed;
} app_ctx_t;

static void on_enter_busy(void *user_data)
{
    app_ctx_t *ctx = (app_ctx_t *)user_data;
    ctx->completed = 0u;
}

static void on_done(void *user_data)
{
    app_ctx_t *ctx = (app_ctx_t *)user_data;
    ctx->completed += 1u;
}

static const mfsm_state_t states[] = {
    MFSM_STATE(NULL, NULL, "IDLE"),
    MFSM_STATE(on_enter_busy, NULL, "BUSY")
};

static const mfsm_transition_t transitions[] = {
    MFSM_TRANSITION(ST_IDLE, EV_START, ST_BUSY, NULL, NULL),
    MFSM_TRANSITION(ST_BUSY, EV_DONE, ST_IDLE, NULL, on_done)
};

static const mfsm_def_t definition =
    MFSM_DEF(states, transitions, ST_COUNT, 2u, ST_IDLE);

int main(void)
{
    app_ctx_t ctx = {0u};
    mfsm_t fsm;
    mfsm_state_id current = 0u;
    const char *name = NULL;

    if (mfsm_validate(&definition) != MFSM_OK) {
        return 1;
    }

    if (mfsm_init(&fsm, &definition, &ctx) != MFSM_OK) {
        return 1;
    }

    if (mfsm_dispatch(&fsm, EV_START) != MFSM_OK) {
        return 1;
    }

    if (mfsm_current(&fsm, &current) != MFSM_OK || current != ST_BUSY) {
        return 1;
    }

    if (mfsm_state_name(&fsm, &name) != MFSM_OK || name == NULL) {
        return 1;
    }

    if (mfsm_dispatch(&fsm, EV_DONE) != MFSM_OK) {
        return 1;
    }

    return 0;
}
```

## Configuration

The installed package provides an authoritative generated `mfsm_config.h`. Consumers must not override it with conflicting values.

| Macro | Meaning | Source-tree default |
| --- | --- | --- |
| `MFSM_ENABLE_NAMES` | `0` disables state-name queries and returns `MFSM_ERR_UNSUPPORTED` | `1` |
| `MFSM_ENABLE_TRACE` | `0` disables trace registration and dispatch-time trace callbacks | `1` |
| `MFSM_MAX_STATES` | definition limit validated at init and validation time | `32` |
| `MFSM_MAX_TRANSITIONS` | definition limit validated at init and validation time | `64` |

## Support matrix

| Area | Status |
| --- | --- |
| C99 library build | Implemented |
| C11 library build | Intended through CMake and CI |
| C++ consumers | Implemented examples for C++11, C++17, and C++20 |
| GCC / Clang / MSVC | Intended through CI workflows |
| ARM cross compilation | Compile-only workflow coverage planned |
| Names-disabled build | Supported by generated config |
| Trace-disabled build | Supported by generated config |
| Independent-instance concurrency | Supported when shared definitions and callbacks are immutable and thread-safe |
| Shared-instance concurrency | Requires external serialization around the whole call, including callbacks |
| ISR dispatch/reset/init | Not supported; queue events and dispatch later |
| Real hardware execution | Not verified in this repository snapshot |

## Execution limits

- One execution context owns one `mfsm_t` at a time unless the application serializes all access externally.
- `mfsm_init()`, `mfsm_dispatch()`, and `mfsm_reset()` run callbacks synchronously.
- Same-instance nested `init`, `dispatch`, `reset`, or `mfsm_set_trace()` from callbacks returns `MFSM_ERR_BUSY`.
- `longjmp`, asynchronous escapes, process termination, hard faults, power loss, and similar events have no cleanup or rollback semantics.
- Action callbacks are `void`. The library does not roll back application side effects.

## Build and package

### CMake

```cmake
find_package(microfsm CONFIG REQUIRED)

add_executable(app main.c)
target_link_libraries(app PRIVATE microfsm::microfsm)
```

### add_subdirectory

```cmake
add_subdirectory(path/to/microfsm microfsm-build)
target_link_libraries(app PRIVATE microfsm::microfsm)
```

## Documentation

- [API reference](docs/API_REFERENCE.md)
- [Cookbook](docs/COOKBOOK.md)
- [Design notes](docs/DESIGN.md)
- [Porting guide](docs/PORTING_GUIDE.md)
- [Troubleshooting](docs/ISSUES.md)
- [Verification status](docs/VERIFICATION.md)
- [Release policy](docs/RELEASES.md)
- [Changelog](CHANGELOG.md)

## Release status

- Releases are tag-driven through [`.github/workflows/release.yml`](.github/workflows/release.yml).
- No local Git tags were present in this repository state before the audit work, so `CHANGELOG.md` keeps all corrective work under `Unreleased`.

## License

MIT. See [LICENSE](LICENSE).
