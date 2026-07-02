# Troubleshooting

## Invalid initial, source, or destination state

`mfsm_validate()` or `mfsm_init()` returns `MFSM_ERR_INVALID_STATE` when any referenced state index is outside `num_states`.

## Unreachable state

`MFSM_ERR_UNREACHABLE` means graph traversal from `initial` could not reach every state. Merely appearing as a transition destination is not enough.

## Shadowed transition after fallback

Once an unguarded transition exists for a `(from, event)` group, every later transition in that group is shadowed and rejected.

## Duplicate unguarded transition

Two unguarded transitions for the same `(from, event)` group return `MFSM_ERR_DUPLICATE`.

## Transition existence versus guard outcome

`mfsm_has_transition()` reports whether a table entry exists. It does not predict whether guards will later accept the event.

## Callbacks cannot fail through microfsm

Action callbacks are `void`. Record failure in caller-owned state and react after the current library call returns.

## Same-instance busy from callback

Nested `mfsm_dispatch()`, `mfsm_reset()`, `mfsm_init()`, or `mfsm_set_trace()` on the same instance returns `MFSM_ERR_BUSY`.

## Names disabled

When `MFSM_ENABLE_NAMES=0`, `mfsm_state_name()` returns `MFSM_ERR_UNSUPPORTED`.

## Trace disabled

When `MFSM_ENABLE_TRACE=0`, `mfsm_set_trace()` returns `MFSM_ERR_UNSUPPORTED`.

## Event 255 versus reset trace

User event `255` remains a normal event. Reset traces are identified by `record.kind == MFSM_TRACE_KIND_RESET`.

## Uninitialized instance

Queries, dispatch, reset, and trace registration return `MFSM_ERR_UNINITIALIZED` until `mfsm_init()` succeeds.

## Invalid current state

If caller-owned memory is corrupted or a definition is mutated after initialization, runtime APIs fail closed with `MFSM_ERR_INVALID_STATE`.

## Definition lifetime or mutation

State arrays, transition arrays, callback pointers, and state-name strings must remain valid for the full lifetime of active instances.

## C++ aggregate initialization

Use the `MFSM_STATE`, `MFSM_TRANSITION`, and `MFSM_DEF` helper macros or positional initialization examples. Do not rely on C99 designated initializers in C++11 or C++17.

## CMake find_package prefix

`find_package(microfsm CONFIG REQUIRED)` needs `CMAKE_PREFIX_PATH` or `microfsm_DIR` to point at the installed package prefix.

## Makefile flags

`tests/Makefile` respects caller-supplied `CC`, `CPPFLAGS`, `CFLAGS`, and `LDFLAGS`.

## Compile-fail diagnostics differ

Compiler wording differs across GCC, Clang, and MSVC. The compile-fail fixtures should match broad diagnostics that mention the intended macro or conflict.

## Sanitizer or static-analysis tool unavailable

Mark the gate as unavailable in verification notes rather than claiming it passed.
