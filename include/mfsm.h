/*
 * microfsm — Deterministic, table-driven finite state machine engine.
 *
 * C99 · Zero dependencies · Zero allocations · ROM-friendly · Portable
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/microfsm
 */

#ifndef MFSM_H
#define MFSM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ── Configuration ─────────────────────────────────────────────────────────
 *
 * Override any of these before #include "mfsm.h" or via -D compiler flags.
 */

#ifndef MFSM_MAX_STATES
#define MFSM_MAX_STATES 32
#endif

#ifndef MFSM_MAX_TRANSITIONS
#define MFSM_MAX_TRANSITIONS 64
#endif

#ifndef MFSM_ENABLE_TRACE
#define MFSM_ENABLE_TRACE 1
#endif

#ifndef MFSM_ENABLE_NAMES
#define MFSM_ENABLE_NAMES 1
#endif

/* Compile-time sanity check */
#if MFSM_MAX_STATES > 255
#error "MFSM_MAX_STATES must be <= 255 (uint8_t range)"
#endif

/* ── Types ─────────────────────────────────────────────────────────────── */

/** State identifier (0 .. MFSM_MAX_STATES-1). */
typedef uint8_t mfsm_state_id;

/** Event identifier (0 .. 255). */
typedef uint8_t mfsm_event_id;

/** Error / status codes. */
typedef enum {
    MFSM_OK                =  0,  /**< Success.                               */
    MFSM_ERR_NULL          = -1,  /**< NULL pointer argument.                 */
    MFSM_ERR_NO_TRANSITION = -2,  /**< No matching transition found.          */
    MFSM_ERR_GUARD_REJECT  = -3,  /**< Transition found, guard returned false.*/
    MFSM_ERR_INVALID_STATE = -4,  /**< State ID out of range.                 */
    MFSM_ERR_INVALID_EVENT = -5,  /**< Event ID looks suspicious (validate).  */
    MFSM_ERR_DUPLICATE     = -6,  /**< Duplicate (from, event) without guards.*/
    MFSM_ERR_UNREACHABLE   = -7,  /**< State has no incoming transition.      */
} mfsm_err_t;

/* ── Callback signatures ───────────────────────────────────────────────── */

/**
 * Action callback — used for on_enter, on_exit, and transition actions.
 *
 * @param user_data  Application context from mfsm_t.user_data.
 *
 * Rules:
 *  - Must NOT call mfsm_dispatch() on the same instance (no re-entrancy).
 *  - Should be short and non-blocking on bare-metal targets.
 *  - May be NULL (the engine skips NULL callbacks).
 */
typedef void (*mfsm_action_fn)(void *user_data);

/**
 * Guard callback — determines whether a transition is allowed.
 *
 * @param user_data  Application context from mfsm_t.user_data.
 * @return true to allow the transition, false to reject.
 *
 * Rules:
 *  - Must be a pure query — no side effects.
 *  - Must NOT call mfsm_dispatch().
 *  - May be NULL (treated as always-true).
 */
typedef bool (*mfsm_guard_fn)(void *user_data);

/**
 * Trace callback — called after every successful transition.
 * Only available when MFSM_ENABLE_TRACE == 1.
 */
typedef void (*mfsm_trace_fn)(
    mfsm_state_id from,
    mfsm_event_id event,
    mfsm_state_id to,
    void         *user_data
);

/* ── Data structures ───────────────────────────────────────────────────── */

/** State descriptor. Typically stored as a const array indexed by state ID. */
typedef struct {
    mfsm_action_fn  on_enter;   /**< Called when entering this state (or NULL). */
    mfsm_action_fn  on_exit;    /**< Called when leaving this state (or NULL).  */
#if MFSM_ENABLE_NAMES
    const char     *name;       /**< Human-readable name (or NULL → "?").      */
#endif
} mfsm_state_t;

/** Transition descriptor. */
typedef struct {
    mfsm_state_id   from;       /**< Source state.                             */
    mfsm_event_id   event;      /**< Triggering event.                        */
    mfsm_state_id   to;         /**< Destination state.                       */
    mfsm_guard_fn   guard;      /**< Guard condition (NULL = always true).     */
    mfsm_action_fn  action;     /**< Transition action (NULL = none).          */
} mfsm_transition_t;

/**
 * Machine definition — a complete, immutable description of a state machine.
 *
 * This structure and everything it references should be const and may reside
 * in ROM/flash. Multiple mfsm_t instances can share one definition.
 */
typedef struct {
    const mfsm_state_t       *states;           /**< State array.             */
    uint8_t                    num_states;       /**< Length of states[].      */
    const mfsm_transition_t  *transitions;       /**< Transition array.        */
    uint8_t                    num_transitions;   /**< Length of transitions[]. */
    mfsm_state_id              initial;           /**< Starting state ID.       */
} mfsm_def_t;

/**
 * Machine instance — runtime state. This is the only struct that uses RAM.
 *
 * Typical size: 8–16 bytes depending on pointer width and config.
 */
typedef struct {
    const mfsm_def_t  *def;         /**< Pointer to definition (ROM-safe).   */
    mfsm_state_id      current;     /**< Current state ID.                   */
    void               *user_data;   /**< Application context.                */
#if MFSM_ENABLE_TRACE
    mfsm_trace_fn      trace;       /**< Trace callback (or NULL).           */
#endif
} mfsm_t;

/* ── Core API ──────────────────────────────────────────────────────────── */

/**
 * Initialise a state machine instance.
 *
 * Sets current state to def->initial and calls its on_enter (if any).
 *
 * @param fsm        Instance to initialise (caller-allocated).
 * @param def        Machine definition (const, ROM-safe).
 * @param user_data  Application context passed to all callbacks.
 * @return MFSM_OK on success.
 */
mfsm_err_t mfsm_init(mfsm_t *fsm, const mfsm_def_t *def, void *user_data);

/**
 * Dispatch an event to the state machine.
 *
 * Scans transitions for a match on (current, event). If found and guard
 * passes, executes: on_exit → transition action → state change → on_enter.
 *
 * @param fsm    Active instance.
 * @param event  Event to process.
 * @return MFSM_OK if transition executed, error code otherwise.
 */
mfsm_err_t mfsm_dispatch(mfsm_t *fsm, mfsm_event_id event);

/**
 * Reset to the initial state.
 *
 * Calls on_exit of current state, then on_enter of initial state.
 *
 * @param fsm  Active instance.
 * @return MFSM_OK on success.
 */
mfsm_err_t mfsm_reset(mfsm_t *fsm);

/* ── Query API ─────────────────────────────────────────────────────────── */

/**
 * Get the current state ID.
 * Returns 0 if fsm is NULL (defensive, should not happen).
 */
mfsm_state_id mfsm_current(const mfsm_t *fsm);

/**
 * Get the human-readable name of the current state.
 * Returns "?" if names are disabled or name is NULL.
 */
const char *mfsm_state_name(const mfsm_t *fsm);

/**
 * Check if an event has a matching transition from the current state.
 * Does NOT evaluate guards. Returns false if fsm is NULL.
 */
bool mfsm_can_handle(const mfsm_t *fsm, mfsm_event_id event);

/* ── Debug API ─────────────────────────────────────────────────────────── */

/**
 * Register a trace callback. Called after every successful transition.
 * Pass NULL to disable. No-op if MFSM_ENABLE_TRACE == 0.
 */
void mfsm_set_trace(mfsm_t *fsm, mfsm_trace_fn fn);

/**
 * Validate a machine definition for structural errors.
 *
 * Checks: non-NULL pointers, valid state IDs, no ambiguous transitions,
 * all states reachable. Call once at startup or in tests.
 *
 * @param def  Definition to validate.
 * @return MFSM_OK if valid, first error code otherwise.
 */
mfsm_err_t mfsm_validate(const mfsm_def_t *def);

/**
 * Convert an error code to a human-readable string.
 * Example: mfsm_err_str(MFSM_ERR_GUARD_REJECT) → "guard rejected"
 */
const char *mfsm_err_str(mfsm_err_t err);

#ifdef __cplusplus
}
#endif

#endif /* MFSM_H */
