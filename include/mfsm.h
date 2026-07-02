/*
 * microfsm - Deterministic, table-driven finite state machine engine.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MFSM_H
#define MFSM_H

#include "mfsm_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t mfsm_err_t;
typedef uint8_t mfsm_state_id;
typedef uint8_t mfsm_event_id;
typedef uint8_t mfsm_trace_kind_t;

#define MFSM_OK ((mfsm_err_t)0)
#define MFSM_ERR_NULL ((mfsm_err_t)-1)
#define MFSM_ERR_INVALID ((mfsm_err_t)-2)
#define MFSM_ERR_INVALID_STATE ((mfsm_err_t)-3)
#define MFSM_ERR_NO_TRANSITION ((mfsm_err_t)-4)
#define MFSM_ERR_GUARD_REJECT ((mfsm_err_t)-5)
#define MFSM_ERR_DUPLICATE ((mfsm_err_t)-6)
#define MFSM_ERR_SHADOWED ((mfsm_err_t)-7)
#define MFSM_ERR_UNREACHABLE ((mfsm_err_t)-8)
#define MFSM_ERR_BUSY ((mfsm_err_t)-9)
#define MFSM_ERR_UNINITIALIZED ((mfsm_err_t)-10)
#define MFSM_ERR_UNSUPPORTED ((mfsm_err_t)-11)
#define MFSM_ERR_LIMIT ((mfsm_err_t)-12)

#define MFSM_TRACE_KIND_TRANSITION ((mfsm_trace_kind_t)0u)
#define MFSM_TRACE_KIND_RESET ((mfsm_trace_kind_t)1u)

typedef void (*mfsm_action_fn)(void *user_data);
typedef bool (*mfsm_guard_fn)(void *user_data);

typedef struct {
    mfsm_trace_kind_t kind;
    mfsm_state_id from;
    mfsm_event_id event;
    mfsm_state_id to;
} mfsm_trace_record_t;

typedef void (*mfsm_trace_fn)(
    const mfsm_trace_record_t *record,
    void *user_data
);

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

#define MFSM_STATE(on_enter_, on_exit_, name_) \
    { (on_enter_), (on_exit_), (name_) }

#define MFSM_TRANSITION(from_, event_, to_, guard_, action_) \
    { (from_), (event_), (to_), (guard_), (action_) }

#define MFSM_DEF(states_, transitions_, num_states_, num_transitions_, initial_) \
    { (states_), (transitions_), (num_states_), (num_transitions_), (initial_) }

mfsm_err_t mfsm_init(mfsm_t *fsm, const mfsm_def_t *def, void *user_data);
mfsm_err_t mfsm_dispatch(mfsm_t *fsm, mfsm_event_id event);
mfsm_err_t mfsm_reset(mfsm_t *fsm);

mfsm_err_t mfsm_current(const mfsm_t *fsm, mfsm_state_id *state);
mfsm_err_t mfsm_state_name(const mfsm_t *fsm, const char **name);
mfsm_err_t mfsm_has_transition(const mfsm_t *fsm, mfsm_event_id event, bool *exists);

mfsm_err_t mfsm_set_trace(mfsm_t *fsm, mfsm_trace_fn fn);
mfsm_err_t mfsm_validate(const mfsm_def_t *def);
const char *mfsm_err_str(mfsm_err_t err);

#ifdef __cplusplus
}
#endif

#endif /* MFSM_H */
