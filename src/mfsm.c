/*
 * microfsm — Implementation.
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/microfsm
 */

#include "mfsm.h"

/* ── Internal helpers ──────────────────────────────────────────────────── */

#ifdef MFSM_ASSERT
#define MFSM_CHECK_NULL(ptr)  do { MFSM_ASSERT((ptr) != NULL); } while (0)
#else
#define MFSM_CHECK_NULL(ptr)  do { if ((ptr) == NULL) return MFSM_ERR_NULL; } while (0)
#endif

/**
 * Find the first transition matching (from, event) whose guard passes.
 * Returns pointer to the matching transition, or NULL.
 * Sets *guard_failed to true if a match was found but guard rejected.
 */
static const mfsm_transition_t *find_transition(
    const mfsm_def_t *def,
    mfsm_state_id     from,
    mfsm_event_id     event,
    void              *user_data,
    bool              *guard_failed)
{
    *guard_failed = false;

    for (uint8_t i = 0; i < def->num_transitions; i++) {
        const mfsm_transition_t *t = &def->transitions[i];

        if (t->from != from || t->event != event) {
            continue;
        }

        /* Match found — check guard */
        if (t->guard != NULL && !t->guard(user_data)) {
            *guard_failed = true;
            continue;   /* try next matching transition (priority ordering) */
        }

        return t;   /* guard passed (or no guard) */
    }

    return NULL;
}

/** Call an action callback if non-NULL. */
static inline void call_action(mfsm_action_fn fn, void *user_data)
{
    if (fn != NULL) {
        fn(user_data);
    }
}

/* ── Core API ──────────────────────────────────────────────────────────── */

mfsm_err_t mfsm_init(mfsm_t *fsm, const mfsm_def_t *def, void *user_data)
{
    MFSM_CHECK_NULL(fsm);
    MFSM_CHECK_NULL(def);
    MFSM_CHECK_NULL(def->states);
    MFSM_CHECK_NULL(def->transitions);

    if (def->initial >= def->num_states) {
        return MFSM_ERR_INVALID_STATE;
    }

    fsm->def       = def;
    fsm->current   = def->initial;
    fsm->user_data = user_data;

#if MFSM_ENABLE_TRACE
    fsm->trace = NULL;
#endif

    /* Enter the initial state */
    call_action(def->states[def->initial].on_enter, user_data);

    return MFSM_OK;
}

mfsm_err_t mfsm_dispatch(mfsm_t *fsm, mfsm_event_id event)
{
    MFSM_CHECK_NULL(fsm);
    MFSM_CHECK_NULL(fsm->def);

    bool guard_failed = false;
    const mfsm_transition_t *t = find_transition(
        fsm->def, fsm->current, event, fsm->user_data, &guard_failed
    );

    if (t == NULL) {
        return guard_failed ? MFSM_ERR_GUARD_REJECT : MFSM_ERR_NO_TRANSITION;
    }

    const mfsm_def_t *def = fsm->def;
    mfsm_state_id from    = fsm->current;
    mfsm_state_id to      = t->to;

    /* 1. Exit current state */
    call_action(def->states[from].on_exit, fsm->user_data);

    /* 2. Transition action */
    call_action(t->action, fsm->user_data);

    /* 3. Update state */
    fsm->current = to;

    /* 4. Enter new state */
    call_action(def->states[to].on_enter, fsm->user_data);

    /* 5. Trace */
#if MFSM_ENABLE_TRACE
    if (fsm->trace != NULL) {
        fsm->trace(from, event, to, fsm->user_data);
    }
#endif

    return MFSM_OK;
}

mfsm_err_t mfsm_reset(mfsm_t *fsm)
{
    MFSM_CHECK_NULL(fsm);
    MFSM_CHECK_NULL(fsm->def);

    const mfsm_def_t *def = fsm->def;
    mfsm_state_id from    = fsm->current;
    mfsm_state_id to      = def->initial;

    /* Exit current state */
    call_action(def->states[from].on_exit, fsm->user_data);

    /* Update state */
    fsm->current = to;

    /* Enter initial state */
    call_action(def->states[to].on_enter, fsm->user_data);

#if MFSM_ENABLE_TRACE
    if (fsm->trace != NULL) {
        /* Use 0xFF as a synthetic "reset" event in trace */
        fsm->trace(from, 0xFF, to, fsm->user_data);
    }
#endif

    return MFSM_OK;
}

/* ── Query API ─────────────────────────────────────────────────────────── */

mfsm_state_id mfsm_current(const mfsm_t *fsm)
{
    if (fsm == NULL) {
        return 0;
    }
    return fsm->current;
}

const char *mfsm_state_name(const mfsm_t *fsm)
{
#if MFSM_ENABLE_NAMES
    if (fsm == NULL || fsm->def == NULL) {
        return "?";
    }
    const char *name = fsm->def->states[fsm->current].name;
    return (name != NULL) ? name : "?";
#else
    (void)fsm;
    return "?";
#endif
}

bool mfsm_can_handle(const mfsm_t *fsm, mfsm_event_id event)
{
    if (fsm == NULL || fsm->def == NULL) {
        return false;
    }

    const mfsm_def_t *def = fsm->def;

    for (uint8_t i = 0; i < def->num_transitions; i++) {
        if (def->transitions[i].from  == fsm->current &&
            def->transitions[i].event == event) {
            return true;
        }
    }

    return false;
}

/* ── Debug API ─────────────────────────────────────────────────────────── */

void mfsm_set_trace(mfsm_t *fsm, mfsm_trace_fn fn)
{
#if MFSM_ENABLE_TRACE
    if (fsm != NULL) {
        fsm->trace = fn;
    }
#else
    (void)fsm;
    (void)fn;
#endif
}

mfsm_err_t mfsm_validate(const mfsm_def_t *def)
{
    if (def == NULL || def->states == NULL || def->transitions == NULL) {
        return MFSM_ERR_NULL;
    }

    /* Check initial state is in range */
    if (def->initial >= def->num_states) {
        return MFSM_ERR_INVALID_STATE;
    }

    /* Check all transition state IDs are in range */
    for (uint8_t i = 0; i < def->num_transitions; i++) {
        const mfsm_transition_t *t = &def->transitions[i];
        if (t->from >= def->num_states || t->to >= def->num_states) {
            return MFSM_ERR_INVALID_STATE;
        }
    }

    /* Check for ambiguous transitions: same (from, event) without guards */
    for (uint8_t i = 0; i < def->num_transitions; i++) {
        for (uint8_t j = i + 1; j < def->num_transitions; j++) {
            const mfsm_transition_t *a = &def->transitions[i];
            const mfsm_transition_t *b = &def->transitions[j];

            if (a->from == b->from && a->event == b->event) {
                /* Ambiguous only if neither has a guard */
                if (a->guard == NULL && b->guard == NULL) {
                    return MFSM_ERR_DUPLICATE;
                }
            }
        }
    }

    /* Check all non-initial states are reachable (appear as 'to' somewhere) */
    for (uint8_t s = 0; s < def->num_states; s++) {
        if (s == def->initial) {
            continue;
        }

        bool reachable = false;
        for (uint8_t t = 0; t < def->num_transitions; t++) {
            if (def->transitions[t].to == s) {
                reachable = true;
                break;
            }
        }

        if (!reachable) {
            return MFSM_ERR_UNREACHABLE;
        }
    }

    return MFSM_OK;
}

const char *mfsm_err_str(mfsm_err_t err)
{
    switch (err) {
    case MFSM_OK:                return "ok";
    case MFSM_ERR_NULL:          return "null pointer";
    case MFSM_ERR_NO_TRANSITION: return "no transition";
    case MFSM_ERR_GUARD_REJECT:  return "guard rejected";
    case MFSM_ERR_INVALID_STATE: return "invalid state";
    case MFSM_ERR_INVALID_EVENT: return "invalid event";
    case MFSM_ERR_DUPLICATE:     return "duplicate transition";
    case MFSM_ERR_UNREACHABLE:   return "unreachable state";
    default:                     return "unknown error";
    }
}
