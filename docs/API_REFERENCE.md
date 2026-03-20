# API Reference

> **Header:** `#include "mfsm.h"`
>
> **Version:** 1.0.0

---

## Table of contents

1. [Configuration macros](#configuration-macros)
2. [Error codes](#error-codes)
3. [Callback types](#callback-types)
4. [Data structures](#data-structures)
5. [Core functions](#core-functions)
6. [Query functions](#query-functions)
7. [Debug functions](#debug-functions)
8. [Thread safety](#thread-safety)

---

## Configuration macros

All macros can be overridden **before** including `mfsm.h` or via compiler
flags.

### MFSM_MAX_STATES

```c
#define MFSM_MAX_STATES 32
```

Upper limit on the number of states in a single machine definition. Determines
the width of `mfsm_state_id` internally. Keeping this ≤ 255 ensures IDs fit
in a `uint8_t`. Setting it higher than 255 is a compile-time error.

### MFSM_MAX_TRANSITIONS

```c
#define MFSM_MAX_TRANSITIONS 64
```

Upper limit on the number of transitions in a single definition. Used only by
`mfsm_validate()` for sanity checking. The engine itself iterates the
transition array linearly, so there is no hard compiled-in limit.

### MFSM_ENABLE_TRACE

```c
#define MFSM_ENABLE_TRACE 1
```

When set to `0`, all trace-related code is compiled out. `mfsm_set_trace()`
becomes a no-op, and no function pointer is stored in `mfsm_t`. Use this in
production builds where every byte counts.

### MFSM_ENABLE_NAMES

```c
#define MFSM_ENABLE_NAMES 1
```

When set to `0`, the `name` field is removed from `mfsm_state_t` and
`mfsm_state_name()` always returns `"?"`. This saves ROM when you have many
states with long names.

### MFSM_ASSERT

```c
/* Not defined by default */
#define MFSM_ASSERT(expr)  configASSERT(expr)   /* FreeRTOS example */
#define MFSM_ASSERT(expr)  assert(expr)          /* POSIX example */
```

If defined, the engine calls `MFSM_ASSERT` on precondition violations
(null pointers, invalid IDs). If not defined, the engine returns an error
code instead of asserting. Define this in debug/development builds for
fast-fail behaviour.

---

## Error codes

```c
typedef enum {
    MFSM_OK                = 0,   /* Success                              */
    MFSM_ERR_NULL          = -1,  /* NULL pointer argument                */
    MFSM_ERR_NO_TRANSITION = -2,  /* No matching transition found         */
    MFSM_ERR_GUARD_REJECT  = -3,  /* Transition found but guard returned false */
    MFSM_ERR_INVALID_STATE = -4,  /* State ID out of range                */
    MFSM_ERR_INVALID_EVENT = -5,  /* Event ID out of range (validate only)*/
    MFSM_ERR_DUPLICATE     = -6,  /* Duplicate from+event pair (validate) */
    MFSM_ERR_UNREACHABLE   = -7,  /* State has no incoming transition     */
} mfsm_err_t;
```

All functions return `mfsm_err_t`. Negative values are errors; zero is
success. The caller should **always** check the return value.

### Error code details

| Code | Returned by | Meaning |
|------|------------|---------|
| `MFSM_OK` | all | Operation succeeded |
| `MFSM_ERR_NULL` | all | A required pointer argument was NULL |
| `MFSM_ERR_NO_TRANSITION` | `dispatch`, `can_handle` | No transition matches (current state, event) |
| `MFSM_ERR_GUARD_REJECT` | `dispatch` | Transition exists but guard function returned `false` |
| `MFSM_ERR_INVALID_STATE` | `init`, `validate` | A state ID in the definition exceeds `num_states` |
| `MFSM_ERR_INVALID_EVENT` | `validate` | Informational — event ID looks suspicious |
| `MFSM_ERR_DUPLICATE` | `validate` | Two transitions share the same (from, event) pair |
| `MFSM_ERR_UNREACHABLE` | `validate` | A non-initial state has no incoming transitions |

---

## Callback types

### mfsm_action_fn

```c
typedef void (*mfsm_action_fn)(void *user_data);
```

General-purpose action callback. Used for `on_enter`, `on_exit`, and
transition actions. The `user_data` pointer comes from `mfsm_t.user_data` —
cast it to your application context type.

**Rules:**
- Must not call `mfsm_dispatch()` on the **same** FSM instance (re-entrancy
  is not supported). Dispatching to a **different** instance is fine.
- Should be short and non-blocking on bare-metal systems.
- May be NULL (the engine skips NULL callbacks).

### mfsm_guard_fn

```c
typedef bool (*mfsm_guard_fn)(void *user_data);
```

Guard callback. Determines whether a transition is allowed. Returns `true` to
permit the transition, `false` to reject it. When rejected, `mfsm_dispatch()`
returns `MFSM_ERR_GUARD_REJECT` and the machine stays in the current state.

**Rules:**
- Must be a **pure query** — no side effects, no state mutation.
- Must not call `mfsm_dispatch()`.
- May be NULL (treated as always-true).

### mfsm_trace_fn

```c
typedef void (*mfsm_trace_fn)(
    mfsm_state_id from,
    mfsm_event_id event,
    mfsm_state_id to,
    void *user_data
);
```

Trace callback, called **after** a successful transition completes (after
`on_enter` of the new state). Useful for logging, metrics, or debugging.
Only available when `MFSM_ENABLE_TRACE` is `1`.

---

## Data structures

### mfsm_state_t

```c
typedef struct {
    mfsm_action_fn on_enter;    /* called when entering this state (or NULL) */
    mfsm_action_fn on_exit;     /* called when leaving this state (or NULL)  */
#if MFSM_ENABLE_NAMES
    const char    *name;        /* human-readable name (or NULL → "?")       */
#endif
} mfsm_state_t;
```

Describes a single state. Typically stored as a `const` array indexed by state
ID enum values. All fields are optional (may be NULL / zeroed).

**Example:**

```c
static const mfsm_state_t states[] = {
    [ST_IDLE]    = { .name = "IDLE",    .on_enter = on_enter_idle    },
    [ST_ACTIVE]  = { .name = "ACTIVE",  .on_enter = on_enter_active,
                                         .on_exit  = on_exit_active  },
    [ST_ERROR]   = { .name = "ERROR"                                 },
};
```

### mfsm_transition_t

```c
typedef struct {
    mfsm_state_id  from;      /* source state                              */
    mfsm_event_id  event;     /* triggering event                          */
    mfsm_state_id  to;        /* destination state                         */
    mfsm_guard_fn  guard;     /* guard condition (NULL = always true)       */
    mfsm_action_fn action;    /* transition action (NULL = none)            */
} mfsm_transition_t;
```

Describes a single state transition. Transitions are stored as a flat `const`
array — the engine does a linear scan on each dispatch.

**Self-transitions** (`from == to`) are supported. Both `on_exit` and
`on_enter` will be called, in that order.

**Multiple transitions from the same (from, event)** — only the **first**
match whose guard passes is taken. This allows priority ordering:

```c
{ ST_ERROR, EV_RETRY, ST_CONNECTING, guard_can_retry,  NULL },
{ ST_ERROR, EV_RETRY, ST_FATAL,     NULL,              NULL },  /* fallback */
```

### mfsm_def_t

```c
typedef struct {
    const mfsm_state_t      *states;          /* state array               */
    uint8_t                   num_states;      /* length of states array    */
    const mfsm_transition_t  *transitions;     /* transition array          */
    uint8_t                   num_transitions;  /* length of transitions     */
    mfsm_state_id             initial;          /* starting state ID         */
} mfsm_def_t;
```

Machine definition — a complete description of the state machine. This
structure (and everything it points to) should be `const` and may reside in
ROM/flash. Multiple `mfsm_t` instances can share the same definition.

### mfsm_t

```c
typedef struct {
    const mfsm_def_t *def;          /* pointer to definition (ROM-safe)  */
    mfsm_state_id     current;      /* current state ID                  */
    void              *user_data;    /* application context               */
#if MFSM_ENABLE_TRACE
    mfsm_trace_fn     trace;        /* trace callback (or NULL)          */
#endif
} mfsm_t;
```

Runtime instance of a state machine. This is the only structure that requires
RAM. Typical size: 8–16 bytes depending on pointer width and config.

Multiple instances can run in parallel (each with its own `mfsm_t`), and they
can share the same `mfsm_def_t`.

---

## Core functions

### mfsm_init

```c
mfsm_err_t mfsm_init(mfsm_t *fsm, const mfsm_def_t *def, void *user_data);
```

Initialise a state machine instance. Sets the current state to `def->initial`
and calls the initial state's `on_enter` callback (if any).

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `fsm` | `mfsm_t *` | Instance to initialise (caller-allocated) |
| `def` | `const mfsm_def_t *` | Machine definition |
| `user_data` | `void *` | Application context passed to all callbacks |

**Returns:** `MFSM_OK` on success, `MFSM_ERR_NULL` if `fsm` or `def` is
NULL, `MFSM_ERR_INVALID_STATE` if `def->initial >= def->num_states`.

**Sequence:**
1. Validates parameters.
2. Stores `def`, `user_data`, clears `trace`.
3. Sets `current = def->initial`.
4. Calls `states[initial].on_enter(user_data)` if non-NULL.

---

### mfsm_dispatch

```c
mfsm_err_t mfsm_dispatch(mfsm_t *fsm, mfsm_event_id event);
```

Process an event. Scans the transition table for a matching `(current, event)`
pair. If found and the guard passes, executes the transition.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `fsm` | `mfsm_t *` | Active instance |
| `event` | `mfsm_event_id` | Event to process |

**Returns:**

| Code | Meaning |
|------|---------|
| `MFSM_OK` | Transition executed successfully |
| `MFSM_ERR_NULL` | `fsm` is NULL |
| `MFSM_ERR_NO_TRANSITION` | No transition matches `(current, event)` |
| `MFSM_ERR_GUARD_REJECT` | Transition found but guard returned `false` |

**Transition execution sequence:**
1. Scan transitions for first match where `from == current && event == event`.
2. If `guard != NULL`, call `guard(user_data)`. If `false`, continue scan or
   return `MFSM_ERR_GUARD_REJECT`.
3. Call `states[current].on_exit(user_data)` if non-NULL.
4. Call `transition.action(user_data)` if non-NULL.
5. Set `current = transition.to`.
6. Call `states[to].on_enter(user_data)` if non-NULL.
7. Call `trace(from, event, to, user_data)` if non-NULL and trace enabled.

**If the event is not handled**, the machine stays in the current state and
no callbacks fire. This is by design — unhandled events are silently ignored
in many state machine implementations, but microfsm explicitly returns an
error code so the caller can decide.

---

### mfsm_reset

```c
mfsm_err_t mfsm_reset(mfsm_t *fsm);
```

Return to the initial state. Calls `on_exit` of the current state, then
`on_enter` of the initial state.

**Returns:** `MFSM_OK` on success, `MFSM_ERR_NULL` if `fsm` is NULL.

**Sequence:**
1. Call `states[current].on_exit(user_data)`.
2. Set `current = def->initial`.
3. Call `states[initial].on_enter(user_data)`.
4. Fire trace callback if enabled.

---

## Query functions

### mfsm_current

```c
mfsm_state_id mfsm_current(const mfsm_t *fsm);
```

Returns the current state ID. If `fsm` is NULL, returns `0` (the caller
should never pass NULL, but this avoids undefined behaviour).

### mfsm_state_name

```c
const char *mfsm_state_name(const mfsm_t *fsm);
```

Returns the `name` string of the current state. If names are disabled
(`MFSM_ENABLE_NAMES == 0`), always returns `"?"`. If the name field is NULL,
returns `"?"`.

### mfsm_can_handle

```c
bool mfsm_can_handle(const mfsm_t *fsm, mfsm_event_id event);
```

Checks if an event has at least one matching transition from the current
state. Does **not** evaluate guards. Returns `true` if a transition exists,
`false` otherwise.

Use this for UI hints ("is the retry button available?") or to filter events
before dispatching.

---

## Debug functions

### mfsm_set_trace

```c
void mfsm_set_trace(mfsm_t *fsm, mfsm_trace_fn fn);
```

Register a trace callback. Called after every successful transition. Pass
NULL to disable tracing. No-op if `MFSM_ENABLE_TRACE == 0`.

**Example:**

```c
static void my_trace(mfsm_state_id from, mfsm_event_id ev,
                     mfsm_state_id to, void *ctx) {
    printf("[FSM] %s --(%d)--> %s\n",
           state_names[from], ev, state_names[to]);
}

mfsm_set_trace(&fsm, my_trace);
```

### mfsm_validate

```c
mfsm_err_t mfsm_validate(const mfsm_def_t *def);
```

Check a machine definition for structural errors. Call this once at startup
(or in tests) to catch mistakes early. Checks performed:

1. `def`, `def->states`, `def->transitions` are non-NULL.
2. `def->initial < def->num_states`.
3. All transition `from` and `to` fields are < `def->num_states`.
4. No two transitions share the same `(from, event)` pair when neither has a
   guard (ambiguous dispatch).
5. Every non-initial state is reachable (appears as `to` in at least one
   transition).

Returns the **first** error found or `MFSM_OK`.

### mfsm_err_str

```c
const char *mfsm_err_str(mfsm_err_t err);
```

Returns a human-readable string for an error code. Useful in logging.
Example: `mfsm_err_str(MFSM_ERR_GUARD_REJECT)` → `"guard rejected"`.

---

## Thread safety

microfsm is **not thread-safe by default**. A single `mfsm_t` instance must
be accessed from one thread (or task) at a time. If you need to dispatch
events from multiple threads or ISRs, protect the instance with a mutex or
critical section provided by your platform:

```c
/* FreeRTOS example */
xSemaphoreTake(fsm_mutex, portMAX_DELAY);
mfsm_dispatch(&fsm, event);
xSemaphoreGive(fsm_mutex);
```

Multiple `mfsm_t` instances (even sharing the same `mfsm_def_t`) can be
used concurrently from different threads without synchronisation, because
the definition is read-only.

---

## Memory layout summary

```
                   ROM / flash                           RAM
              ┌──────────────────┐              ┌──────────────────┐
              │  mfsm_def_t      │◀─────────────│  mfsm_t          │
              │   .states ───────│───┐          │   .def ──────────│──┐
              │   .transitions ──│─┐ │          │   .current       │  │
              │   .initial       │ │ │          │   .user_data     │  │
              └──────────────────┘ │ │          │   .trace         │  │
              ┌──────────────────┐ │ │          └──────────────────┘  │
              │  mfsm_state_t[]  │◀┘ │                               │
              │   [0] name,enter │   │               (8–16 bytes)    │
              │   [1] name,exit  │   │                               │
              └──────────────────┘   │          ┌────────────────────┘
              ┌──────────────────┐   │          │
              │ mfsm_transition_t│◀──┘          │  points to ROM
              │  [] from,ev,to,  │              │
              │     guard,action │
              └──────────────────┘
```
