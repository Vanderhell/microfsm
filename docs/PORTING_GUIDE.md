# Porting Guide

## General requirements

- C99 compiler for the library
- immutable state and transition arrays
- caller-owned storage for `mfsm_t`
- external serialization if more than one execution context touches the same instance

## Execution model

- Separate instances may run concurrently when shared definitions and callbacks are immutable and thread-safe.
- A shared instance requires serialization around the whole call, including callbacks.
- ISR code should queue events and dispatch later.

## Assertions

`MFSM_ASSERT` is diagnostic only. Runtime validation still returns status codes when assertions are disabled, compiled out with `NDEBUG`, or replaced with a logging macro.

## C++ consumers

The public header already provides `extern "C"` guards. C++ consumers do not need to wrap `#include "mfsm.h"` manually.

Use positional aggregate initialization or the helper macros:

```cpp
static const mfsm_state_t states[] = {
    MFSM_STATE(nullptr, nullptr, "IDLE"),
    MFSM_STATE(nullptr, nullptr, "DONE")
};
```

## Bare metal and RTOS targets

- Keep callbacks short.
- Queue ISR-originated events.
- Treat callback side effects as application behavior, not library-managed transactions.

## MSVC

Build through CMake or compile the C implementation as C. C++ consumers can link against the resulting library target.

## ARM cross compilation

Compile-only coverage is useful for type and header validation, but it is not real hardware proof.
