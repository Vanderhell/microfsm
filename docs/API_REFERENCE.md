# API Reference

Header: `#include "mfsm.h"`

## ABI types

```c
typedef int32_t mfsm_err_t;
typedef uint8_t mfsm_state_id;
typedef uint8_t mfsm_event_id;
typedef uint8_t mfsm_trace_kind_t;
```

These widths are fixed and do not vary with `-fshort-enums`.

## Status codes

| Name | Value |
| --- | --- |
| `MFSM_OK` | `0` |
| `MFSM_ERR_NULL` | `-1` |
| `MFSM_ERR_INVALID` | `-2` |
| `MFSM_ERR_INVALID_STATE` | `-3` |
| `MFSM_ERR_NO_TRANSITION` | `-4` |
| `MFSM_ERR_GUARD_REJECT` | `-5` |
| `MFSM_ERR_DUPLICATE` | `-6` |
| `MFSM_ERR_SHADOWED` | `-7` |
| `MFSM_ERR_UNREACHABLE` | `-8` |
| `MFSM_ERR_BUSY` | `-9` |
| `MFSM_ERR_UNINITIALIZED` | `-10` |
| `MFSM_ERR_UNSUPPORTED` | `-11` |
| `MFSM_ERR_LIMIT` | `-12` |

## Trace kinds

| Name | Value |
| --- | --- |
| `MFSM_TRACE_KIND_TRANSITION` | `0` |
| `MFSM_TRACE_KIND_RESET` | `1` |

## Callback types

```c
typedef void (*mfsm_action_fn)(void *user_data);
typedef bool (*mfsm_guard_fn)(void *user_data);
typedef void (*mfsm_trace_fn)(const mfsm_trace_record_t *record, void *user_data);
```

- Action callbacks are synchronous and `void`. Application failures must be recorded in caller-owned state.
- Guards are evaluated in transition-table order and may reject without consuming the event.
- `mfsm_trace_fn` runs only after the normal callback sequence completes.

## Public structs

```c
typedef struct {
    mfsm_trace_kind_t kind;
    mfsm_state_id from;
    mfsm_event_id event;
    mfsm_state_id to;
} mfsm_trace_record_t;

typedef struct {
    mfsm_action_fn on_enter;
    mfsm_action_fn on_exit;
    const char *name;
} mfsm_state_t;

typedef struct {
    mfsm_state_id from;
    mfsm_event_id event;
    mfsm_state_id to;
    mfsm_guard_fn guard;
    mfsm_action_fn action;
} mfsm_transition_t;

typedef struct {
    const mfsm_state_t *states;
    const mfsm_transition_t *transitions;
    uint16_t num_states;
    uint16_t num_transitions;
    mfsm_state_id initial;
} mfsm_def_t;

typedef struct {
    const mfsm_def_t *def;
    void *user_data;
    mfsm_trace_fn trace;
    mfsm_state_id current;
    uint8_t initialized;
    uint8_t busy;
    uint8_t reserved[2];
} mfsm_t;
```

`MFSM_ENABLE_NAMES` and `MFSM_ENABLE_TRACE` do not change these layouts.

## Helper macros

```c
#define MFSM_STATE(on_enter_, on_exit_, name_) ...
#define MFSM_TRANSITION(from_, event_, to_, guard_, action_) ...
#define MFSM_DEF(states_, transitions_, num_states_, num_transitions_, initial_) ...
```

These helpers keep examples portable across C99 and C++ aggregate initialization.

## Core API

### `mfsm_validate`

```c
mfsm_err_t mfsm_validate(const mfsm_def_t *def);
```

Checks:

- non-NULL definition and state array
- `1 <= num_states <= MFSM_MAX_STATES`
- `0 <= num_transitions <= MFSM_MAX_TRANSITIONS`
- `transitions != NULL` when `num_transitions > 0`
- `initial < num_states`
- every transition source and destination is in range
- exact duplicates are rejected
- only one unguarded `(from, event)` fallback may exist
- an unguarded fallback must be the final transition in its `(from, event)` group
- every state is reachable from `initial` by graph traversal that ignores runtime guard outcomes

### `mfsm_init`

```c
mfsm_err_t mfsm_init(mfsm_t *fsm, const mfsm_def_t *def, void *user_data);
```

- Validates `def` on every successful initialization.
- On failure, `fsm` is left fully zeroed and uninitialized.
- On success, `fsm` becomes initialized before the initial `on_enter` callback runs.
- Reinitializing the same instance while it is busy returns `MFSM_ERR_BUSY`.

### `mfsm_dispatch`

```c
mfsm_err_t mfsm_dispatch(mfsm_t *fsm, mfsm_event_id event);
```

Normal transition order:

1. choose one transition
2. call source `on_exit`
3. call transition action
4. commit `current = to`
5. call destination `on_enter`
6. call trace hook

Visibility:

- guard, source `on_exit`, and transition action observe the source state
- destination `on_enter` and trace observe the destination state

### `mfsm_reset`

```c
mfsm_err_t mfsm_reset(mfsm_t *fsm);
```

- Rejects busy and uninitialized instances.
- Validates current and initial states before callbacks.
- Runs exit, commits `current = initial`, then runs enter.
- Emits a reset trace record with `kind = MFSM_TRACE_KIND_RESET`.
- Resetting while already in the initial state still performs exit and enter.

## Query API

```c
mfsm_err_t mfsm_current(const mfsm_t *fsm, mfsm_state_id *state);
mfsm_err_t mfsm_state_name(const mfsm_t *fsm, const char **name);
mfsm_err_t mfsm_has_transition(const mfsm_t *fsm, mfsm_event_id event, bool *exists);
```

Rules:

- output parameters remain unchanged on error
- `mfsm_state_name()` returns `MFSM_ERR_UNSUPPORTED` when names are disabled
- `mfsm_has_transition()` reports table presence only and never evaluates guards
- invalid current state returns `MFSM_ERR_INVALID_STATE`

## Trace registration

```c
mfsm_err_t mfsm_set_trace(mfsm_t *fsm, mfsm_trace_fn fn);
```

- Returns `MFSM_ERR_UNINITIALIZED` if `fsm` has not been initialized.
- Returns `MFSM_ERR_BUSY` if called on the same instance from callbacks.
- Returns `MFSM_ERR_UNSUPPORTED` when trace support is disabled.

## Error strings

```c
const char *mfsm_err_str(mfsm_err_t err);
```

Returns a short diagnostic string for logs and tests.
