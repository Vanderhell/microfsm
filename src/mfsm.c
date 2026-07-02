/*
 * microfsm - Implementation.
 *
 * SPDX-License-Identifier: MIT
 */

#include "mfsm.h"

#include <string.h>

#define MFSM_RESET_EVENT_SENTINEL ((mfsm_event_id)0u)

#ifdef MFSM_ASSERT
#define MFSM_DIAG_ASSERT(expr_) MFSM_ASSERT(expr_)
#else
#define MFSM_DIAG_ASSERT(expr_) ((void)0)
#endif

#define MFSM_RETURN_NULL(ptr_)                                                    \
    do {                                                                          \
        const void *mfsm_ptr__ = (const void *)(ptr_);                            \
        if (mfsm_ptr__ == NULL) {                                                 \
            MFSM_DIAG_ASSERT(mfsm_ptr__ != NULL);                                 \
            return MFSM_ERR_NULL;                                                 \
        }                                                                         \
    } while (0)

static bool mfsm_transition_equal(
    const mfsm_transition_t *lhs,
    const mfsm_transition_t *rhs)
{
    return lhs->from == rhs->from &&
           lhs->event == rhs->event &&
           lhs->to == rhs->to &&
           lhs->guard == rhs->guard &&
           lhs->action == rhs->action;
}

static void mfsm_call_action(mfsm_action_fn fn, void *user_data)
{
    if (fn != NULL) {
        fn(user_data);
    }
}

static bool mfsm_group_match(
    const mfsm_transition_t *transition,
    mfsm_state_id from,
    mfsm_event_id event)
{
    return transition->from == from && transition->event == event;
}

static mfsm_err_t mfsm_validate_definition_shape(const mfsm_def_t *def)
{
    MFSM_RETURN_NULL(def);
    MFSM_RETURN_NULL(def->states);

    if (def->num_states < 1u || def->num_states > (uint16_t)MFSM_MAX_STATES) {
        MFSM_DIAG_ASSERT(def->num_states >= 1u);
        return MFSM_ERR_LIMIT;
    }

    if (def->num_transitions > (uint16_t)MFSM_MAX_TRANSITIONS) {
        MFSM_DIAG_ASSERT(def->num_transitions <= (uint16_t)MFSM_MAX_TRANSITIONS);
        return MFSM_ERR_LIMIT;
    }

    if (def->num_transitions > 0u && def->transitions == NULL) {
        MFSM_DIAG_ASSERT(def->transitions != NULL);
        return MFSM_ERR_NULL;
    }

    if (def->initial >= def->num_states) {
        MFSM_DIAG_ASSERT(def->initial < def->num_states);
        return MFSM_ERR_INVALID_STATE;
    }

    return MFSM_OK;
}

static mfsm_err_t mfsm_validate_runtime_definition(const mfsm_def_t *def)
{
    mfsm_err_t err = mfsm_validate_definition_shape(def);
    uint16_t i;
    bool reachable[MFSM_MAX_STATES];
    mfsm_state_id queue[MFSM_MAX_STATES];
    uint16_t head = 0u;
    uint16_t tail = 0u;

    if (err != MFSM_OK) {
        return err;
    }

    for (i = 0u; i < def->num_transitions; ++i) {
        const mfsm_transition_t *transition = &def->transitions[i];
        bool saw_unguarded = false;
        uint16_t j;

        if (transition->from >= def->num_states || transition->to >= def->num_states) {
            MFSM_DIAG_ASSERT(transition->from < def->num_states);
            return MFSM_ERR_INVALID_STATE;
        }

        for (j = 0u; j < i; ++j) {
            const mfsm_transition_t *other = &def->transitions[j];

            if (!mfsm_group_match(other, transition->from, transition->event)) {
                continue;
            }

            if (mfsm_transition_equal(other, transition)) {
                return MFSM_ERR_DUPLICATE;
            }

            if (other->guard == NULL) {
                saw_unguarded = true;
                if (transition->guard == NULL) {
                    return MFSM_ERR_DUPLICATE;
                }
            }
        }

        if (saw_unguarded) {
            return MFSM_ERR_SHADOWED;
        }
    }

    memset(reachable, 0, sizeof(reachable));
    queue[tail++] = def->initial;
    reachable[def->initial] = true;

    while (head < tail) {
        const mfsm_state_id from = queue[head++];

        for (i = 0u; i < def->num_transitions; ++i) {
            const mfsm_transition_t *transition = &def->transitions[i];
            const mfsm_state_id to = transition->to;

            if (transition->from != from || reachable[to]) {
                continue;
            }

            reachable[to] = true;
            queue[tail++] = to;
        }
    }

    for (i = 0u; i < def->num_states; ++i) {
        if (!reachable[i]) {
            return MFSM_ERR_UNREACHABLE;
        }
    }

    return MFSM_OK;
}

static mfsm_err_t mfsm_require_instance(
    const mfsm_t *fsm,
    bool require_not_busy,
    const mfsm_def_t **def_out)
{
    mfsm_err_t err;

    MFSM_RETURN_NULL(fsm);

    if (!fsm->initialized) {
        return MFSM_ERR_UNINITIALIZED;
    }

    if (require_not_busy && fsm->busy) {
        return MFSM_ERR_BUSY;
    }

    err = mfsm_validate_definition_shape(fsm->def);
    if (err != MFSM_OK) {
        return err;
    }

    if (fsm->current >= fsm->def->num_states) {
        return MFSM_ERR_INVALID_STATE;
    }

    if (def_out != NULL) {
        *def_out = fsm->def;
    }

    return MFSM_OK;
}

static const mfsm_transition_t *mfsm_find_transition(
    const mfsm_t *fsm,
    mfsm_event_id event,
    bool *saw_match,
    bool *saw_guard_reject)
{
    const mfsm_def_t *def = fsm->def;
    uint16_t i;

    *saw_match = false;
    *saw_guard_reject = false;

    for (i = 0u; i < def->num_transitions; ++i) {
        const mfsm_transition_t *transition = &def->transitions[i];

        if (!mfsm_group_match(transition, fsm->current, event)) {
            continue;
        }

        *saw_match = true;

        if (transition->guard != NULL && !transition->guard(fsm->user_data)) {
            *saw_guard_reject = true;
            continue;
        }

        return transition;
    }

    return NULL;
}

mfsm_err_t mfsm_init(mfsm_t *fsm, const mfsm_def_t *def, void *user_data)
{
    mfsm_err_t err;

    MFSM_RETURN_NULL(fsm);

    if (fsm->busy) {
        return MFSM_ERR_BUSY;
    }

    err = mfsm_validate_runtime_definition(def);
    if (err != MFSM_OK) {
        memset(fsm, 0, sizeof(*fsm));
        return err;
    }

    memset(fsm, 0, sizeof(*fsm));
    fsm->def = def;
    fsm->user_data = user_data;
    fsm->current = def->initial;
    fsm->initialized = 1u;
    fsm->busy = 1u;

    mfsm_call_action(def->states[def->initial].on_enter, user_data);

    fsm->busy = 0u;
    return MFSM_OK;
}

mfsm_err_t mfsm_dispatch(mfsm_t *fsm, mfsm_event_id event)
{
    const mfsm_def_t *def;
    const mfsm_transition_t *transition;
    bool saw_match;
    bool saw_guard_reject;
    mfsm_state_id from;
    mfsm_state_id to;
    mfsm_trace_record_t record;
    mfsm_err_t err = mfsm_require_instance(fsm, true, &def);

    if (err != MFSM_OK) {
        return err;
    }

    fsm->busy = 1u;
    transition = mfsm_find_transition(fsm, event, &saw_match, &saw_guard_reject);
    if (transition == NULL) {
        fsm->busy = 0u;
        return saw_guard_reject ? MFSM_ERR_GUARD_REJECT : MFSM_ERR_NO_TRANSITION;
    }

    to = transition->to;
    if (to >= def->num_states) {
        fsm->busy = 0u;
        return MFSM_ERR_INVALID_STATE;
    }

    from = fsm->current;
    mfsm_call_action(def->states[from].on_exit, fsm->user_data);
    mfsm_call_action(transition->action, fsm->user_data);
    fsm->current = to;
    mfsm_call_action(def->states[to].on_enter, fsm->user_data);

    if (MFSM_ENABLE_TRACE && fsm->trace != NULL) {
        record.kind = MFSM_TRACE_KIND_TRANSITION;
        record.from = from;
        record.event = event;
        record.to = to;
        fsm->trace(&record, fsm->user_data);
    }

    fsm->busy = 0u;
    return MFSM_OK;
}

mfsm_err_t mfsm_reset(mfsm_t *fsm)
{
    const mfsm_def_t *def;
    mfsm_state_id from;
    mfsm_state_id to;
    mfsm_trace_record_t record;
    mfsm_err_t err = mfsm_require_instance(fsm, true, &def);

    if (err != MFSM_OK) {
        return err;
    }

    to = def->initial;
    if (to >= def->num_states) {
        return MFSM_ERR_INVALID_STATE;
    }

    fsm->busy = 1u;
    from = fsm->current;
    mfsm_call_action(def->states[from].on_exit, fsm->user_data);
    fsm->current = to;
    mfsm_call_action(def->states[to].on_enter, fsm->user_data);

    if (MFSM_ENABLE_TRACE && fsm->trace != NULL) {
        record.kind = MFSM_TRACE_KIND_RESET;
        record.from = from;
        record.event = MFSM_RESET_EVENT_SENTINEL;
        record.to = to;
        fsm->trace(&record, fsm->user_data);
    }

    fsm->busy = 0u;
    return MFSM_OK;
}

mfsm_err_t mfsm_current(const mfsm_t *fsm, mfsm_state_id *state)
{
    mfsm_err_t err = mfsm_require_instance(fsm, false, NULL);

    MFSM_RETURN_NULL(state);
    if (err != MFSM_OK) {
        return err;
    }

    *state = fsm->current;
    return MFSM_OK;
}

mfsm_err_t mfsm_state_name(const mfsm_t *fsm, const char **name)
{
    const mfsm_def_t *def;
    mfsm_err_t err;

    MFSM_RETURN_NULL(name);

    if (!MFSM_ENABLE_NAMES) {
        return MFSM_ERR_UNSUPPORTED;
    }

    err = mfsm_require_instance(fsm, false, &def);
    if (err != MFSM_OK) {
        return err;
    }

    *name = def->states[fsm->current].name;
    return MFSM_OK;
}

mfsm_err_t mfsm_has_transition(const mfsm_t *fsm, mfsm_event_id event, bool *exists)
{
    const mfsm_def_t *def;
    uint16_t i;
    mfsm_err_t err;

    MFSM_RETURN_NULL(exists);

    err = mfsm_require_instance(fsm, false, &def);
    if (err != MFSM_OK) {
        return err;
    }

    for (i = 0u; i < def->num_transitions; ++i) {
        const mfsm_transition_t *transition = &def->transitions[i];

        if (mfsm_group_match(transition, fsm->current, event)) {
            *exists = true;
            return MFSM_OK;
        }
    }

    *exists = false;
    return MFSM_OK;
}

mfsm_err_t mfsm_set_trace(mfsm_t *fsm, mfsm_trace_fn fn)
{
    MFSM_RETURN_NULL(fsm);

    if (!fsm->initialized) {
        return MFSM_ERR_UNINITIALIZED;
    }

    if (fsm->busy) {
        return MFSM_ERR_BUSY;
    }

    if (!MFSM_ENABLE_TRACE) {
        return MFSM_ERR_UNSUPPORTED;
    }

    fsm->trace = fn;
    return MFSM_OK;
}

mfsm_err_t mfsm_validate(const mfsm_def_t *def)
{
    return mfsm_validate_runtime_definition(def);
}

const char *mfsm_err_str(mfsm_err_t err)
{
    switch (err) {
    case MFSM_OK:
        return "ok";
    case MFSM_ERR_NULL:
        return "null pointer";
    case MFSM_ERR_INVALID:
        return "invalid argument";
    case MFSM_ERR_INVALID_STATE:
        return "invalid state";
    case MFSM_ERR_NO_TRANSITION:
        return "no transition";
    case MFSM_ERR_GUARD_REJECT:
        return "guard rejected";
    case MFSM_ERR_DUPLICATE:
        return "duplicate transition";
    case MFSM_ERR_SHADOWED:
        return "shadowed transition";
    case MFSM_ERR_UNREACHABLE:
        return "unreachable state";
    case MFSM_ERR_BUSY:
        return "instance busy";
    case MFSM_ERR_UNINITIALIZED:
        return "uninitialized instance";
    case MFSM_ERR_UNSUPPORTED:
        return "unsupported";
    case MFSM_ERR_LIMIT:
        return "configured limit exceeded";
    default:
        return "unknown error";
    }
}
