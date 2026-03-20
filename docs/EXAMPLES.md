# Examples

Real-world usage patterns for microfsm. Each example is self-contained and
demonstrates a different aspect of the library.

---

## 1. MQTT IoT device with reconnect logic

The most common IoT pattern: connect to a broker, publish data, handle
disconnections with exponential backoff.

```
┌──────┐ BOOT_DONE ┌────────────┐ WIFI_OK  ┌────────────┐
│ INIT │──────────▶│ CONNECTING │────────▶│ SUBSCRIBING│
└──────┘           └────────────┘          └────────────┘
                        ▲                       │
                        │                  SUB_ACK
                        │                       ▼
                   ┌─────────┐  RETRY    ┌───────────┐
                   │ BACKOFF │◀──────────│  ONLINE   │
                   └─────────┘  DISC     └───────────┘
                        │                     │
                   MAX_RETRIES           PUBLISH
                        ▼                     ▼
                   ┌─────────┐          ┌───────────┐
                   │  FATAL  │          │PUBLISHING │
                   └─────────┘          └───────────┘
```

```c
#include "mfsm.h"
#include <stdio.h>

/* ── Application context ─────────────────────────────────── */

typedef struct {
    int  retry_count;
    int  backoff_ms;
    char topic[64];
    char payload[256];
} mqtt_ctx_t;

/* ── States ──────────────────────────────────────────────── */

enum {
    ST_INIT,
    ST_CONNECTING,
    ST_SUBSCRIBING,
    ST_ONLINE,
    ST_PUBLISHING,
    ST_BACKOFF,
    ST_FATAL,
    ST_COUNT
};

/* ── Events ──────────────────────────────────────────────── */

enum {
    EV_BOOT_DONE,
    EV_WIFI_OK,
    EV_SUB_ACK,
    EV_PUBLISH,
    EV_PUB_ACK,
    EV_DISCONNECT,
    EV_RETRY,
    EV_TIMEOUT,
};

/* ── Guards ──────────────────────────────────────────────── */

static bool guard_can_retry(void *ctx) {
    mqtt_ctx_t *m = (mqtt_ctx_t *)ctx;
    return m->retry_count < 5;
}

static bool guard_max_retries(void *ctx) {
    mqtt_ctx_t *m = (mqtt_ctx_t *)ctx;
    return m->retry_count >= 5;
}

/* ── Actions ─────────────────────────────────────────────── */

static void on_enter_connecting(void *ctx) {
    mqtt_ctx_t *m = (mqtt_ctx_t *)ctx;
    printf("[MQTT] Connecting (attempt %d)...\n", m->retry_count + 1);
    /* wifi_connect(); mqtt_connect(); */
}

static void on_enter_backoff(void *ctx) {
    mqtt_ctx_t *m = (mqtt_ctx_t *)ctx;
    m->retry_count++;
    m->backoff_ms = (m->backoff_ms == 0) ? 1000 : m->backoff_ms * 2;
    if (m->backoff_ms > 30000) m->backoff_ms = 30000;
    printf("[MQTT] Backing off %d ms (retry %d/5)\n",
           m->backoff_ms, m->retry_count);
    /* start_timer(m->backoff_ms); */
}

static void on_enter_online(void *ctx) {
    mqtt_ctx_t *m = (mqtt_ctx_t *)ctx;
    m->retry_count = 0;
    m->backoff_ms  = 0;
    printf("[MQTT] Online and ready.\n");
}

static void on_enter_fatal(void *ctx) {
    printf("[MQTT] FATAL: max retries exceeded. Rebooting.\n");
    /* esp_restart(); */
}

/* ── Machine definition ──────────────────────────────────── */

static const mfsm_state_t states[] = {
    [ST_INIT]        = { .name = "INIT"        },
    [ST_CONNECTING]  = { .name = "CONNECTING",  .on_enter = on_enter_connecting },
    [ST_SUBSCRIBING] = { .name = "SUBSCRIBING"  },
    [ST_ONLINE]      = { .name = "ONLINE",      .on_enter = on_enter_online     },
    [ST_PUBLISHING]  = { .name = "PUBLISHING"   },
    [ST_BACKOFF]     = { .name = "BACKOFF",     .on_enter = on_enter_backoff    },
    [ST_FATAL]       = { .name = "FATAL",       .on_enter = on_enter_fatal      },
};

static const mfsm_transition_t transitions[] = {
    /* from            event           to              guard              action */
    { ST_INIT,        EV_BOOT_DONE,   ST_CONNECTING,  NULL,              NULL },
    { ST_CONNECTING,  EV_WIFI_OK,     ST_SUBSCRIBING, NULL,              NULL },
    { ST_SUBSCRIBING, EV_SUB_ACK,     ST_ONLINE,      NULL,              NULL },
    { ST_ONLINE,      EV_PUBLISH,     ST_PUBLISHING,  NULL,              NULL },
    { ST_PUBLISHING,  EV_PUB_ACK,     ST_ONLINE,      NULL,              NULL },

    /* Disconnection from any active state → backoff */
    { ST_CONNECTING,  EV_DISCONNECT,  ST_BACKOFF,     NULL,              NULL },
    { ST_SUBSCRIBING, EV_DISCONNECT,  ST_BACKOFF,     NULL,              NULL },
    { ST_ONLINE,      EV_DISCONNECT,  ST_BACKOFF,     NULL,              NULL },
    { ST_PUBLISHING,  EV_DISCONNECT,  ST_BACKOFF,     NULL,              NULL },

    /* Retry with guard priority: can_retry first, then fatal fallback */
    { ST_BACKOFF,     EV_RETRY,       ST_CONNECTING,  guard_can_retry,   NULL },
    { ST_BACKOFF,     EV_RETRY,       ST_FATAL,       guard_max_retries, NULL },
};

static const mfsm_def_t mqtt_def = {
    .states          = states,
    .num_states      = ST_COUNT,
    .transitions     = transitions,
    .num_transitions = sizeof(transitions) / sizeof(transitions[0]),
    .initial         = ST_INIT,
};
```

---

## 2. LED blink patterns (bare metal, minimal)

A tiny example showing how a state machine can replace boolean flags for
managing LED behaviour. Useful on any MCU.

```c
#include "mfsm.h"

enum { ST_OFF, ST_ON, ST_FAST_BLINK, ST_COUNT };
enum { EV_TOGGLE, EV_ALERT, EV_CLEAR };

static void led_on(void *ctx)  { /* HAL_GPIO_WritePin(LED, SET);   */ }
static void led_off(void *ctx) { /* HAL_GPIO_WritePin(LED, RESET); */ }

static const mfsm_state_t states[] = {
    [ST_OFF]        = { .name = "OFF",   .on_enter = led_off },
    [ST_ON]         = { .name = "ON",    .on_enter = led_on  },
    [ST_FAST_BLINK] = { .name = "BLINK", .on_enter = led_on  },
};

static const mfsm_transition_t transitions[] = {
    { ST_OFF,        EV_TOGGLE, ST_ON,         NULL, NULL },
    { ST_ON,         EV_TOGGLE, ST_OFF,        NULL, NULL },
    { ST_OFF,        EV_ALERT,  ST_FAST_BLINK, NULL, NULL },
    { ST_ON,         EV_ALERT,  ST_FAST_BLINK, NULL, NULL },
    { ST_FAST_BLINK, EV_TOGGLE, ST_FAST_BLINK, NULL, NULL }, /* self-transition */
    { ST_FAST_BLINK, EV_CLEAR,  ST_OFF,        NULL, NULL },
};

static const mfsm_def_t led_def = {
    .states          = states,
    .num_states      = ST_COUNT,
    .transitions     = transitions,
    .num_transitions = sizeof(transitions) / sizeof(transitions[0]),
    .initial         = ST_OFF,
};
```

---

## 3. Traffic light (timed transitions)

Classic FSM example showing how to combine microfsm with a timer. The
state machine does not own the timer — the application drives events.

```c
#include "mfsm.h"

enum { ST_RED, ST_GREEN, ST_YELLOW, ST_COUNT };
enum { EV_TIMER };

typedef struct {
    uint32_t duration_ms;  /* how long to stay in current state */
} light_ctx_t;

static void on_enter_red(void *ctx) {
    light_ctx_t *l = (light_ctx_t *)ctx;
    l->duration_ms = 5000;
    /* set_outputs(RED_ON, YELLOW_OFF, GREEN_OFF); */
}

static void on_enter_green(void *ctx) {
    light_ctx_t *l = (light_ctx_t *)ctx;
    l->duration_ms = 4000;
    /* set_outputs(RED_OFF, YELLOW_OFF, GREEN_ON); */
}

static void on_enter_yellow(void *ctx) {
    light_ctx_t *l = (light_ctx_t *)ctx;
    l->duration_ms = 1500;
    /* set_outputs(RED_OFF, YELLOW_ON, GREEN_OFF); */
}

static const mfsm_state_t states[] = {
    [ST_RED]    = { .name = "RED",    .on_enter = on_enter_red    },
    [ST_GREEN]  = { .name = "GREEN",  .on_enter = on_enter_green  },
    [ST_YELLOW] = { .name = "YELLOW", .on_enter = on_enter_yellow },
};

static const mfsm_transition_t transitions[] = {
    { ST_RED,    EV_TIMER, ST_GREEN,  NULL, NULL },
    { ST_GREEN,  EV_TIMER, ST_YELLOW, NULL, NULL },
    { ST_YELLOW, EV_TIMER, ST_RED,    NULL, NULL },
};

static const mfsm_def_t light_def = {
    .states          = states,
    .num_states      = ST_COUNT,
    .transitions     = transitions,
    .num_transitions = sizeof(transitions) / sizeof(transitions[0]),
    .initial         = ST_RED,
};

/* Application loop (bare metal): */
/*
    light_ctx_t ctx;
    mfsm_t fsm;
    mfsm_init(&fsm, &light_def, &ctx);

    uint32_t last_tick = HAL_GetTick();
    while (1) {
        if (HAL_GetTick() - last_tick >= ctx.duration_ms) {
            mfsm_dispatch(&fsm, EV_TIMER);
            last_tick = HAL_GetTick();
        }
    }
*/
```

---

## 4. Tracing and debugging

Using the trace hook to log every transition:

```c
static const char *state_names[] = {
    "INIT", "CONNECTING", "ONLINE", "ERROR"
};

static const char *event_names[] = {
    "START", "CONNECTED", "FAILURE", "RETRY"
};

static void trace_callback(mfsm_state_id from, mfsm_event_id event,
                           mfsm_state_id to, void *ctx) {
    printf("[FSM] %s --(%s)--> %s\n",
           state_names[from], event_names[event], state_names[to]);
}

/* Setup: */
mfsm_t fsm;
mfsm_init(&fsm, &def, &my_ctx);
mfsm_set_trace(&fsm, trace_callback);

/* Now every dispatch prints:
   [FSM] INIT --(START)--> CONNECTING
   [FSM] CONNECTING --(CONNECTED)--> ONLINE
   etc.
*/
```

---

## 5. Multiple instances sharing one definition

Monitoring 4 identical temperature sensors, each with its own state:

```c
typedef struct {
    int     sensor_id;
    float   last_temp;
    int     error_count;
} sensor_ctx_t;

/* One definition for all sensors */
static const mfsm_def_t sensor_def = { ... };

/* Four independent instances */
static sensor_ctx_t contexts[4] = {
    { .sensor_id = 0 }, { .sensor_id = 1 },
    { .sensor_id = 2 }, { .sensor_id = 3 },
};
static mfsm_t fsms[4];

void sensors_init(void) {
    for (int i = 0; i < 4; i++) {
        mfsm_init(&fsms[i], &sensor_def, &contexts[i]);
    }
}

void sensor_event(int sensor_id, mfsm_event_id event) {
    mfsm_dispatch(&fsms[sensor_id], event);
}
```

---

## 6. Composing sub-machines

Using `user_data` to carry a secondary FSM for a complex sub-process:

```c
typedef struct {
    mfsm_t  main_fsm;
    mfsm_t  connect_fsm;   /* sub-machine for connection handshake */
    /* ... */
} device_t;

static void on_enter_connecting(void *ctx) {
    device_t *dev = (device_t *)ctx;
    /* Start the sub-machine */
    mfsm_init(&dev->connect_fsm, &handshake_def, dev);
}

/* In the main loop, route events to the right machine: */
void device_handle_event(device_t *dev, uint8_t event) {
    mfsm_state_id main_state = mfsm_current(&dev->main_fsm);

    if (main_state == ST_CONNECTING) {
        /* Sub-machine handles connection events */
        mfsm_err_t err = mfsm_dispatch(&dev->connect_fsm, event);
        if (mfsm_current(&dev->connect_fsm) == HANDSHAKE_DONE) {
            mfsm_dispatch(&dev->main_fsm, EV_CONNECTED);
        }
    } else {
        mfsm_dispatch(&dev->main_fsm, event);
    }
}
```

---

## 7. Validation at startup

Catching definition errors early:

```c
int main(void) {
    mfsm_err_t err = mfsm_validate(&machine_def);
    if (err != MFSM_OK) {
        printf("FSM definition error: %s\n", mfsm_err_str(err));
        return 1;
    }

    mfsm_t fsm;
    mfsm_init(&fsm, &machine_def, NULL);
    /* ... */
}
```

In production, you can skip validation (it is only useful during development
and testing).

---

## Pattern summary

| Pattern | Key technique | When to use |
|---------|--------------|-------------|
| MQTT device | Guards + exponential backoff | Any network-connected device |
| LED blink | Self-transitions | Simple I/O control |
| Traffic light | Timer-driven events | Periodic state changes |
| Tracing | `mfsm_set_trace` | Development and debugging |
| Multi-instance | Shared `mfsm_def_t` | Identical devices/channels |
| Sub-machines | Nested `mfsm_t` in context | Complex sub-processes |
| Validation | `mfsm_validate` at startup | Catching definition bugs early |
