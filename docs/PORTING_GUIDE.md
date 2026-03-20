# Porting Guide

microfsm is designed to compile and run on any platform with a C99 compiler.
This guide covers integration into specific environments.

---

## General integration

### Files to include

You need exactly two files:

```
include/mfsm.h    →  add to your include path
src/mfsm.c        →  add to your build
```

No build system is required. No configuration files, no code generation, no
pre-processing steps. Add the two files to your project and compile.

### Compiler requirements

- **C99** or later (for `stdint.h`, `stdbool.h`, designated initialisers).
- Tested with: GCC 7+, Clang 6+, ARM Compiler 6 (armclang), IAR EWARM 8+,
  MSVC 2019+ (C11 mode).

### Configuration

Override macros **before** including `mfsm.h`:

```c
/* Option A: in a project-wide header */
#define MFSM_MAX_STATES     16
#define MFSM_MAX_TRANSITIONS 32
#define MFSM_ENABLE_TRACE    0       /* strip trace code */
#define MFSM_ENABLE_NAMES    0       /* strip name strings */
#define MFSM_ASSERT(x)      MY_ASSERT(x)
#include "mfsm.h"

/* Option B: compiler flags */
/* gcc -DMFSM_MAX_STATES=16 -DMFSM_ENABLE_TRACE=0 ... */
```

---

## Platform-specific notes

### ESP32 (ESP-IDF)

```
components/
└── microfsm/
    ├── include/
    │   └── mfsm.h
    ├── mfsm.c
    └── CMakeLists.txt
```

**CMakeLists.txt:**

```cmake
idf_component_register(
    SRCS "mfsm.c"
    INCLUDE_DIRS "include"
)
```

**Usage with FreeRTOS tasks:**

```c
/* Each task owns its own mfsm_t — no synchronisation needed */
void sensor_task(void *param) {
    sensor_ctx_t ctx = { .sensor_id = (int)param };
    mfsm_t fsm;
    mfsm_init(&fsm, &sensor_def, &ctx);

    while (1) {
        int event = xQueueReceive(event_queue, &event, portMAX_DELAY);
        mfsm_dispatch(&fsm, event);
    }
}
```

**Placing definitions in flash:**

```c
/* ESP-IDF: DRAM_ATTR for RAM, no attribute needed for const (flash) */
static const mfsm_state_t states[] = { ... };        /* flash */
static const mfsm_transition_t transitions[] = { ... }; /* flash */
static const mfsm_def_t def = { ... };                /* flash */
static mfsm_t fsm;                                     /* RAM  */
```

**Recommended MFSM_ASSERT for ESP-IDF:**

```c
#define MFSM_ASSERT(x) configASSERT(x)
```

---

### STM32 (bare metal / HAL)

**With STM32CubeIDE / Makefile:**

Add to your source list:

```makefile
C_SOURCES += lib/microfsm/src/mfsm.c
C_INCLUDES += -Ilib/microfsm/include
```

**Interrupt-safe dispatch:**

If events come from ISRs (GPIO EXTI, UART, Timer), do **not** call
`mfsm_dispatch()` directly from the ISR. Instead, post the event to a
buffer and process it in the main loop:

```c
/* ISR: just record the event */
volatile uint8_t pending_event = EV_NONE;

void EXTI0_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
    pending_event = EV_BUTTON_PRESS;
}

/* Main loop: process events */
int main(void) {
    HAL_Init();
    SystemClock_Config();

    mfsm_t fsm;
    mfsm_init(&fsm, &machine_def, NULL);

    while (1) {
        if (pending_event != EV_NONE) {
            uint8_t ev = pending_event;
            pending_event = EV_NONE;
            mfsm_dispatch(&fsm, ev);
        }
        HAL_Delay(10);
    }
}
```

For a more robust approach, use a ring buffer for events:

```c
#define EVENT_BUF_SIZE 8
static volatile uint8_t event_buf[EVENT_BUF_SIZE];
static volatile uint8_t ev_head = 0, ev_tail = 0;

static inline void event_push(uint8_t ev) {
    uint8_t next = (ev_head + 1) % EVENT_BUF_SIZE;
    if (next != ev_tail) {
        event_buf[ev_head] = ev;
        ev_head = next;
    }
}

static inline bool event_pop(uint8_t *ev) {
    if (ev_tail == ev_head) return false;
    *ev = event_buf[ev_tail];
    ev_tail = (ev_tail + 1) % EVENT_BUF_SIZE;
    return true;
}
```

**Memory footprint on Cortex-M0 (typical):**

| Section | Size |
|---------|------|
| `.text` (engine code) | ~600–900 bytes |
| `.rodata` (10-state machine def) | ~200–400 bytes |
| `.bss` (one instance) | 12–16 bytes |

---

### Zephyr RTOS

```
modules/
└── microfsm/
    ├── zephyr/
    │   └── CMakeLists.txt
    ├── include/
    │   └── mfsm.h
    └── src/
        └── mfsm.c
```

**zephyr/CMakeLists.txt:**

```cmake
zephyr_library()
zephyr_library_sources(../src/mfsm.c)
zephyr_include_directories(../include)
```

**Integration with Zephyr work queues:**

```c
static void fsm_work_handler(struct k_work *work) {
    /* dequeue event and dispatch */
}
K_WORK_DEFINE(fsm_work, fsm_work_handler);

/* From ISR or any context: */
k_work_submit(&fsm_work);
```

---

### Arduino

Copy `mfsm.h` and `mfsm.c` into your sketch folder or a library folder.
Arduino IDE compiles `.c` files automatically.

```
MySketch/
├── MySketch.ino
├── mfsm.h
└── mfsm.c
```

**In MySketch.ino:**

```cpp
extern "C" {
    #include "mfsm.h"
}

// ... define states, transitions, def as usual ...

void setup() {
    mfsm_init(&fsm, &machine_def, NULL);
}

void loop() {
    // read sensors, determine event
    mfsm_dispatch(&fsm, event);
    delay(100);
}
```

Note the `extern "C"` wrapper — Arduino uses C++, and the header needs C
linkage.

---

### Linux / POSIX (testing, simulation, gateways)

```bash
gcc -std=c99 -Wall -Wextra -Iinclude src/mfsm.c your_app.c -o your_app
```

**Recommended MFSM_ASSERT:**

```c
#include <assert.h>
#define MFSM_ASSERT(x) assert(x)
```

**Thread-safe dispatch with pthreads:**

```c
pthread_mutex_t fsm_lock = PTHREAD_MUTEX_INITIALIZER;

void safe_dispatch(mfsm_t *fsm, mfsm_event_id event) {
    pthread_mutex_lock(&fsm_lock);
    mfsm_dispatch(fsm, event);
    pthread_mutex_unlock(&fsm_lock);
}
```

---

### Windows (MSVC)

```
# Developer Command Prompt
cl /std:c11 /W4 /Iinclude src\mfsm.c your_app.c
```

MSVC requires `/std:c11` for designated initialisers. If using an older MSVC
that does not support C11, use explicit struct initialisation:

```c
/* Instead of designated initialisers: */
static const mfsm_state_t states[] = {
    /* [ST_IDLE] = { .name = "IDLE" } */
    { NULL, NULL, "IDLE" },         /* on_enter, on_exit, name */
    { on_enter_conn, NULL, "CONN" },
};
```

---

## CMake integration (cross-platform)

If your project uses CMake:

```cmake
add_library(microfsm STATIC
    lib/microfsm/src/mfsm.c
)
target_include_directories(microfsm PUBLIC
    lib/microfsm/include
)

# Optional: compile-time config
target_compile_definitions(microfsm PUBLIC
    MFSM_MAX_STATES=16
    MFSM_ENABLE_TRACE=0
)

# Link to your target
target_link_libraries(my_app PRIVATE microfsm)
```

---

## Checklist for a new platform

1. **C99 compiler available?** → you are good to go.
2. **Do you have `stdint.h` and `stdbool.h`?** → if not (very old
   compilers), provide typedefs for `uint8_t`, `bool`, `true`, `false`.
3. **Do you want asserts?** → define `MFSM_ASSERT` to your platform's
   assert mechanism.
4. **Memory-constrained?** → set `MFSM_ENABLE_NAMES 0` and
   `MFSM_ENABLE_TRACE 0`.
5. **Events from ISR?** → buffer events, dispatch from main loop / task.
6. **Multiple threads?** → protect `mfsm_dispatch()` with a mutex.
7. **Multiple instances on shared def?** → just create more `mfsm_t`
   variables. The `const` definition is safe to share.
