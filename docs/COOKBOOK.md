# Cookbook

Every example below checks return values explicitly.

## 1. Minimal definition

```c
static const mfsm_state_t states[] = {
    MFSM_STATE(NULL, NULL, "IDLE"),
    MFSM_STATE(NULL, NULL, "DONE")
};

static const mfsm_transition_t transitions[] = {
    MFSM_TRANSITION(0u, 1u, 1u, NULL, NULL)
};

static const mfsm_def_t definition = MFSM_DEF(states, transitions, 2u, 1u, 0u);
```

## 2. Validate before init

```c
if (mfsm_validate(&definition) != MFSM_OK) {
    return 1;
}
```

## 3. Init with user data

```c
mfsm_t fsm;
app_ctx_t ctx = {0};
if (mfsm_init(&fsm, &definition, &ctx) != MFSM_OK) {
    return 1;
}
```

## 4. Dispatch a normal transition

```c
if (mfsm_dispatch(&fsm, EV_START) != MFSM_OK) {
    return 1;
}
```

## 5. Guarded transition chain

```c
static const mfsm_transition_t transitions[] = {
    MFSM_TRANSITION(ST_ERROR, EV_RETRY, ST_CONNECTING, guard_can_retry, NULL),
    MFSM_TRANSITION(ST_ERROR, EV_RETRY, ST_FATAL, guard_must_fail, NULL)
};
```

## 6. Final unguarded fallback

```c
static const mfsm_transition_t transitions[] = {
    MFSM_TRANSITION(ST_ERROR, EV_RETRY, ST_CONNECTING, guard_can_retry, NULL),
    MFSM_TRANSITION(ST_ERROR, EV_RETRY, ST_FATAL, NULL, NULL)
};
```

## 7. Reset with explicit trace kind

```c
if (mfsm_reset(&fsm) != MFSM_OK) {
    return 1;
}
```

Trace callbacks inspect `record->kind` and do not infer reset from an event sentinel.

## 8. Transition-existence query

```c
bool exists = false;
if (mfsm_has_transition(&fsm, EV_START, &exists) != MFSM_OK) {
    return 1;
}
```

## 9. Current-state query

```c
mfsm_state_id current = 0u;
if (mfsm_current(&fsm, &current) != MFSM_OK) {
    return 1;
}
```

## 10. State-name query

```c
const char *name = NULL;
if (mfsm_state_name(&fsm, &name) != MFSM_OK) {
    return 1;
}
```

## 11. Trace registration

```c
if (mfsm_set_trace(&fsm, my_trace) != MFSM_OK) {
    return 1;
}
```

## 12. Names-disabled build

When `MFSM_ENABLE_NAMES=0`, `mfsm_state_name()` returns `MFSM_ERR_UNSUPPORTED`. Definitions may still store `NULL` names.

## 13. Trace-disabled build

When `MFSM_ENABLE_TRACE=0`, `mfsm_set_trace()` returns `MFSM_ERR_UNSUPPORTED` and dispatch performs no active trace callback.

## 14. Shared definition across instances

```c
mfsm_t left;
mfsm_t right;
if (mfsm_init(&left, &definition, &left_ctx) != MFSM_OK) {
    return 1;
}
if (mfsm_init(&right, &definition, &right_ctx) != MFSM_OK) {
    return 1;
}
```

## 15. Nested different-instance dispatch

One callback may dispatch a different `mfsm_t` instance if shared data is thread-safe and immutable where needed.

## 16. Set a flag, dispatch later

Use callbacks to record intent in caller-owned state, then dispatch follow-up events after the outer call returns.

## 17. C++11, C++17, and C++20 definition style

```cpp
static const mfsm_state_t states[] = {
    MFSM_STATE(nullptr, nullptr, "IDLE"),
    MFSM_STATE(nullptr, nullptr, "DONE")
};

static const mfsm_transition_t transitions[] = {
    MFSM_TRANSITION(0u, 1u, 1u, nullptr, nullptr)
};
```

## 18. install/find_package consumer

See `tests/consumers/find_package_c/` and `tests/consumers/find_package_cpp/`.

## 19. add_subdirectory consumer

See `tests/consumers/add_subdirectory_c/`.

## 20. ISR queues event and dispatches later

Do not call `mfsm_dispatch()` directly from an ISR when callbacks may run arbitrary code. Queue the event and dispatch from a thread or main loop.
