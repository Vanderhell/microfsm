# microfsm

**Deterministic, table-driven finite state machine engine for embedded systems.**

[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![CI](https://github.com/Vanderhell/microfsm/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/Vanderhell/microfsm/actions/workflows/ci.yml)
[![Language: C99](https://img.shields.io/badge/language-C99-blue.svg)](#)
[![Platform: Portable](https://img.shields.io/badge/platform-embedded%20%7C%20desktop-0a7ea4.svg)](#)
[![Status: Stable API](https://img.shields.io/badge/status-stable%20API-brightgreen.svg)](#)

C99 · Zero dependencies · Zero allocations · ROM-friendly · Portable

---

## Why microfsm?

Every IoT and embedded project reaches a point where device behavior becomes a
tangled mess of `if/else` chains and boolean flags. A sensor node that must
boot → connect → authenticate → work → handle errors → reconnect quickly turns
into unmaintainable spaghetti. **microfsm** replaces that with a clean,
data-driven state machine that is easy to read, test, and extend.

```
┌─────────┐   WIFI_OK   ┌───────────┐  AUTH_OK  ┌─────────┐
│  BOOT   │────────────▶│ CONNECTED │─────────▶│ WORKING │
└─────────┘              └───────────┘           └─────────┘
                              ▲         TIMEOUT       │
                              │    ┌──────────────────┘
                              │    ▼
                         ┌───────────┐
                         │   ERROR   │
                         └───────────┘
```

The entire machine above is described in ~20 lines of **const data** — no
dynamic allocation, no code generation, no external tools.

## Features

- **Table-driven** — states and transitions are const arrays; the engine is
  generic. Machine definitions can live in ROM/flash.
- **Guard conditions** — a transition only fires when its optional guard
  function returns `true`.
- **Entry / exit actions** — every state can have `on_enter` and `on_exit`
  callbacks, called automatically during transitions.
- **Transition actions** — optional one-shot callback fired during a specific
  transition, between exit and entry.
- **Trace / debug hooks** — plug in a single callback to log every transition.
  Disabled at compile time with `MFSM_ENABLE_TRACE 0`.
- **State names** — human-readable names for debugging. Disabled at compile
  time with `MFSM_ENABLE_NAMES 0` to save ROM.
- **Validation** — `mfsm_validate()` checks a definition for common mistakes
  (duplicate transitions, orphan states, unreachable states) at startup.
- **Zero dynamic allocation** — everything is stack or static. Safe for
  bare-metal, RTOS, or any environment without a heap.
- **C99, zero dependencies** — compiles with gcc, clang, armcc, iccarm, or
  any conforming C99 compiler.
- **Portable** — no platform-specific code. Works on Cortex-M0, ESP32,
  Linux, Windows, macOS.
- **Small footprint** — the engine itself is ~300 lines of C. Typical
  compiled size: < 1 KB text, < 100 B RAM per instance.

## Quick start

### 1. Copy into your project

```
your_project/
├── lib/
│   └── microfsm/
│       ├── include/
│       │   └── mfsm.h
│       └── src/
│           └── mfsm.c
```

Or add as a Git submodule:

```bash
git submodule add https://github.com/Vanderhell/microfsm.git lib/microfsm
```

### 2. Define states and events

```c
#include "mfsm.h"

/* States — use an enum for type safety */
enum {
    ST_IDLE,
    ST_CONNECTING,
    ST_WORKING,
    ST_ERROR,
    ST_COUNT          /* always last — gives you the count */
};

/* Events */
enum {
    EV_START,
    EV_CONNECTED,
    EV_DATA_READY,
    EV_FAILURE,
    EV_RETRY,
    EV_COUNT
};
```

### 3. Define callbacks (optional)

```c
static void on_enter_connecting(void *ctx) {
    printf("Connecting to broker...\n");
    /* start WiFi/MQTT connect */
}

static void on_exit_error(void *ctx) {
    printf("Leaving error state, resetting counters.\n");
}

static bool guard_can_retry(void *ctx) {
    my_device_t *dev = (my_device_t *)ctx;
    return dev->retry_count < 5;
}
```

### 4. Build the machine definition

```c
static const mfsm_state_t states[] = {
    [ST_IDLE]       = { .name = "IDLE"       },
    [ST_CONNECTING] = { .name = "CONNECTING", .on_enter = on_enter_connecting },
    [ST_WORKING]    = { .name = "WORKING"    },
    [ST_ERROR]      = { .name = "ERROR",     .on_exit  = on_exit_error       },
};

static const mfsm_transition_t transitions[] = {
    { ST_IDLE,       EV_START,      ST_CONNECTING, NULL,            NULL },
    { ST_CONNECTING, EV_CONNECTED,  ST_WORKING,    NULL,            NULL },
    { ST_WORKING,    EV_FAILURE,    ST_ERROR,      NULL,            NULL },
    { ST_ERROR,      EV_RETRY,      ST_CONNECTING, guard_can_retry, NULL },
};

static const mfsm_def_t machine_def = {
    .states          = states,
    .num_states      = ST_COUNT,
    .transitions     = transitions,
    .num_transitions = sizeof(transitions) / sizeof(transitions[0]),
    .initial         = ST_IDLE,
};
```

### 5. Run it

```c
int main(void) {
    my_device_t device = { .retry_count = 0 };

    mfsm_t fsm;
    mfsm_init(&fsm, &machine_def, &device);

    /* on_enter for IDLE fires here */
    printf("Current: %s\n", mfsm_state_name(&fsm));   /* "IDLE" */

    mfsm_dispatch(&fsm, EV_START);       /* IDLE → CONNECTING */
    mfsm_dispatch(&fsm, EV_CONNECTED);   /* CONNECTING → WORKING */
    mfsm_dispatch(&fsm, EV_FAILURE);     /* WORKING → ERROR */

    device.retry_count = 3;
    mfsm_dispatch(&fsm, EV_RETRY);       /* ERROR → CONNECTING (guard passes) */

    device.retry_count = 5;
    mfsm_err_t err = mfsm_dispatch(&fsm, EV_RETRY);
    /* err == MFSM_ERR_GUARD_REJECTED — guard_can_retry returned false */

    return 0;
}
```

## Configuration

All options are compile-time `#define`s. Override them before including the
header or via compiler flags (`-DMFSM_MAX_STATES=16`).

| Macro                | Default | Description                                 |
|----------------------|---------|---------------------------------------------|
| `MFSM_MAX_STATES`   | 32      | Maximum states per definition (uint8 range) |
| `MFSM_MAX_TRANSITIONS` | 64   | Maximum transitions per definition          |
| `MFSM_ENABLE_TRACE` | 1       | Enable trace hook (set 0 to strip code)     |
| `MFSM_ENABLE_NAMES` | 1       | Enable state name strings (set 0 for ROM)   |
| `MFSM_ASSERT(expr)` | (none)  | Custom assert macro, e.g. `configASSERT`    |

## API at a glance

| Function              | Description                                    |
|-----------------------|------------------------------------------------|
| `mfsm_init`          | Initialise instance, enter initial state       |
| `mfsm_dispatch`      | Send event — may trigger a transition          |
| `mfsm_current`       | Get current state ID                           |
| `mfsm_state_name`    | Get current state name (if names enabled)      |
| `mfsm_can_handle`    | Check if an event has a valid transition       |
| `mfsm_reset`         | Re-enter initial state (calls exit + enter)    |
| `mfsm_set_trace`     | Register trace callback                        |
| `mfsm_validate`      | Check definition for structural errors         |
| `mfsm_err_str`       | Error code to human-readable string            |

Full reference: **[docs/API_REFERENCE.md](docs/API_REFERENCE.md)**

## Documentation

| Document                                              | Content                            |
|-------------------------------------------------------|------------------------------------|
| [API Reference](docs/API_REFERENCE.md)                | Every function, type, and macro    |
| [Design Rationale](docs/DESIGN.md)                    | Architecture decisions and tradeoffs |
| [Porting Guide](docs/PORTING_GUIDE.md)                | How to integrate on any platform   |
| [Examples](docs/EXAMPLES.md)                          | Real-world usage patterns          |

## GitHub readiness

- Clear MIT licensing with copyright owned by Vanderhell
- Public API in `include/mfsm.h`
- Implementation isolated in `src/mfsm.c`
- Tests in `tests/`
- Extended documentation in `docs/`
- Contribution guide and changelog included
- Community guidelines in `CODE_OF_CONDUCT.md`
- GitHub Actions CI for GCC and Clang
- Repository-safe `.gitignore` for C, coverage, and editor artifacts

## Building the tests

```bash
cd tests
make            # builds and runs all tests
make coverage   # generates lcov report
```

Requires only a C99 compiler (gcc or clang) and make.

## Project structure

```
microfsm/
├── include/
│   └── mfsm.h              # public header (only file you include)
├── src/
│   └── mfsm.c              # implementation (~300 lines)
├── tests/
│   ├── test_all.c          # consolidated test suite
│   └── Makefile
├── examples/
│   └── README.md           # points to documented examples
├── docs/
│   ├── API_REFERENCE.md
│   ├── DESIGN.md
│   ├── PORTING_GUIDE.md
│   └── EXAMPLES.md
├── .gitignore
├── README.md
├── CHANGELOG.md
├── CODE_OF_CONDUCT.md
├── CONTRIBUTING.md
├── git.txt
└── LICENSE
```

## Repository metadata

- Suggested GitHub repository name: `microfsm`
- Short description: `Deterministic table-driven finite state machine engine for embedded systems in portable C99.`
- Suggested topics: `c`, `c99`, `embedded`, `embedded-systems`, `finite-state-machine`, `fsm`, `state-machine`, `library`, `zero-allocation`, `bare-metal`, `iot`, `microcontroller`

## License

MIT — see [LICENSE](LICENSE).
