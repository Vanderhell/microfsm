/*
 * microfsm contract tests.
 *
 * Build example:
 *   cc -std=c99 -Wall -Wextra -Wpedantic -I../include ../src/mfsm.c test_all.c -o test_all
 */

#include "mfsm.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static unsigned int tests_run = 0u;
static unsigned int tests_passed = 0u;
static unsigned int tests_failed = 0u;
static const char *failure_message = NULL;
static int failure_line = 0;

static bool fail_at(const char *message, int line)
{
    failure_message = message;
    failure_line = line;
    return false;
}

#define TEST(name) static bool name(void)

#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            return fail_at("assert true failed: " #expr, __LINE__); \
        } \
    } while (0)

#define ASSERT_FALSE(expr) \
    do { \
        if (expr) { \
            return fail_at("assert false failed: " #expr, __LINE__); \
        } \
    } while (0)

#define ASSERT_STATUS(expected, actual) \
    do { \
        const mfsm_err_t actual_value__ = (actual); \
        if ((expected) != actual_value__) { \
            return fail_at("status mismatch", __LINE__); \
        } \
    } while (0)

#define ASSERT_U8(expected, actual) \
    do { \
        const uint8_t actual_value__ = (uint8_t)(actual); \
        if ((uint8_t)(expected) != actual_value__) { \
            return fail_at("u8 mismatch", __LINE__); \
        } \
    } while (0)

#define ASSERT_UINT(expected, actual) \
    do { \
        const unsigned int actual_value__ = (unsigned int)(actual); \
        if ((unsigned int)(expected) != actual_value__) { \
            return fail_at("unsigned mismatch", __LINE__); \
        } \
    } while (0)

#define ASSERT_PTR(expected, actual) \
    do { \
        const void *actual_value__ = (const void *)(actual); \
        if ((const void *)(expected) != actual_value__) { \
            return fail_at("pointer mismatch", __LINE__); \
        } \
    } while (0)

#define ASSERT_STR(expected, actual) \
    do { \
        const char *expected_value__ = (expected); \
        const char *actual_value__ = (actual); \
        if ((expected_value__ == NULL && actual_value__ != NULL) || \
            (expected_value__ != NULL && actual_value__ == NULL) || \
            (expected_value__ != NULL && strcmp(expected_value__, actual_value__) != 0)) { \
            return fail_at("string mismatch", __LINE__); \
        } \
    } while (0)

static void run_test(const char *name, bool (*fn)(void))
{
    const bool passed = fn();

    ++tests_run;
    if (passed) {
        ++tests_passed;
        printf("PASS %s\n", name);
        return;
    }

    ++tests_failed;
    printf("FAIL %s:%d %s\n", name, failure_line, failure_message);
}

enum {
    ST_A = 0,
    ST_B = 1,
    ST_C = 2,
    ST_D = 3
};

enum {
    EV_GO = 1,
    EV_ALT = 2,
    EV_RESET = 3,
    EV_MAX = 255
};

enum callback_phase {
    PHASE_NONE = 0,
    PHASE_INIT_ENTER,
    PHASE_GUARD,
    PHASE_EXIT,
    PHASE_ACTION,
    PHASE_ENTER,
    PHASE_TRACE,
    PHASE_RESET_EXIT,
    PHASE_RESET_ENTER
};

typedef struct {
    mfsm_t *self;
    mfsm_t *peer;
    enum callback_phase reenter_phase;
    enum callback_phase last_phase;
    mfsm_err_t nested_result;
    mfsm_err_t peer_result;
    mfsm_state_id observed_state;
    uint8_t sequence[16];
    uint8_t sequence_len;
    uint8_t enters[4];
    uint8_t exits[4];
    uint8_t actions;
    uint8_t guard_calls;
    uint8_t trace_calls;
    mfsm_trace_record_t last_trace;
} test_ctx_t;

static void ctx_reset(test_ctx_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->nested_result = MFSM_OK;
    ctx->peer_result = MFSM_OK;
}

static void log_phase(test_ctx_t *ctx, uint8_t marker)
{
    if (ctx->sequence_len < (uint8_t)sizeof(ctx->sequence)) {
        ctx->sequence[ctx->sequence_len++] = marker;
    }
}

static void capture_current(test_ctx_t *ctx)
{
    mfsm_state_id current = 0u;

    if (ctx->self != NULL && mfsm_current(ctx->self, &current) == MFSM_OK) {
        ctx->observed_state = current;
    }
}

static void maybe_reenter_dispatch(test_ctx_t *ctx, enum callback_phase phase)
{
    if (ctx->reenter_phase == phase) {
        ctx->nested_result = mfsm_dispatch(ctx->self, EV_ALT);
    }
}

static void maybe_reenter_reset(test_ctx_t *ctx, enum callback_phase phase)
{
    if (ctx->reenter_phase == phase) {
        ctx->nested_result = mfsm_reset(ctx->self);
    }
}

static void on_enter_a(void *user_data)
{
    test_ctx_t *ctx = (test_ctx_t *)user_data;
    ctx->last_phase = ctx->self != NULL && !ctx->self->busy ? PHASE_NONE : PHASE_INIT_ENTER;
    ++ctx->enters[ST_A];
    log_phase(ctx, 1u);
    capture_current(ctx);
    maybe_reenter_dispatch(ctx, PHASE_INIT_ENTER);
}

static void on_enter_b(void *user_data)
{
    test_ctx_t *ctx = (test_ctx_t *)user_data;
    ++ctx->enters[ST_B];
    log_phase(ctx, 4u);
    capture_current(ctx);
    maybe_reenter_dispatch(ctx, PHASE_ENTER);
    maybe_reenter_reset(ctx, PHASE_RESET_ENTER);
}

static void on_enter_c(void *user_data)
{
    test_ctx_t *ctx = (test_ctx_t *)user_data;
    ++ctx->enters[ST_C];
    log_phase(ctx, 9u);
}

static void on_exit_a(void *user_data)
{
    test_ctx_t *ctx = (test_ctx_t *)user_data;
    ++ctx->exits[ST_A];
    log_phase(ctx, 2u);
    capture_current(ctx);
    maybe_reenter_dispatch(ctx, PHASE_EXIT);
}

static void on_exit_b(void *user_data)
{
    test_ctx_t *ctx = (test_ctx_t *)user_data;
    ++ctx->exits[ST_B];
    log_phase(ctx, 7u);
    capture_current(ctx);
    maybe_reenter_reset(ctx, PHASE_RESET_EXIT);
}

static void action_go(void *user_data)
{
    test_ctx_t *ctx = (test_ctx_t *)user_data;
    ++ctx->actions;
    log_phase(ctx, 3u);
    capture_current(ctx);
    maybe_reenter_dispatch(ctx, PHASE_ACTION);
}

static bool guard_allow(void *user_data)
{
    test_ctx_t *ctx = (test_ctx_t *)user_data;
    ++ctx->guard_calls;
    log_phase(ctx, 5u);
    capture_current(ctx);
    maybe_reenter_dispatch(ctx, PHASE_GUARD);
    return true;
}

static bool guard_false(void *user_data)
{
    test_ctx_t *ctx = (test_ctx_t *)user_data;
    ++ctx->guard_calls;
    return false;
}

static void trace_capture(const mfsm_trace_record_t *record, void *user_data)
{
    test_ctx_t *ctx = (test_ctx_t *)user_data;
    ++ctx->trace_calls;
    ctx->last_trace = *record;
    log_phase(ctx, 6u);
    capture_current(ctx);
    maybe_reenter_dispatch(ctx, PHASE_TRACE);
}

static void dispatch_peer_action(void *user_data)
{
    test_ctx_t *ctx = (test_ctx_t *)user_data;
    if (ctx->peer != NULL) {
        ctx->peer_result = mfsm_dispatch(ctx->peer, EV_GO);
    }
}

static const mfsm_state_t core_states[] = {
    MFSM_STATE(on_enter_a, on_exit_a, "A"),
    MFSM_STATE(on_enter_b, on_exit_b, "B"),
    MFSM_STATE(on_enter_c, NULL, "C")
};

static const mfsm_transition_t core_transitions[] = {
    MFSM_TRANSITION(ST_A, EV_GO, ST_B, guard_allow, action_go),
    MFSM_TRANSITION(ST_A, EV_ALT, ST_C, NULL, NULL),
    MFSM_TRANSITION(ST_B, EV_GO, ST_A, NULL, NULL)
};

static const mfsm_def_t core_def = MFSM_DEF(
    core_states,
    core_transitions,
    3u,
    3u,
    ST_A
);

static mfsm_def_t make_def(
    const mfsm_state_t *states,
    uint16_t num_states,
    const mfsm_transition_t *transitions,
    uint16_t num_transitions,
    mfsm_state_id initial)
{
    const mfsm_def_t def = MFSM_DEF(states, transitions, num_states, num_transitions, initial);
    return def;
}

TEST(test_validate_zero_states)
{
    const mfsm_def_t def = make_def(core_states, 0u, NULL, 0u, 0u);
    ASSERT_STATUS(MFSM_ERR_LIMIT, mfsm_validate(&def));
    return true;
}

TEST(test_validate_max_states)
{
    mfsm_state_t states[MFSM_MAX_STATES];
    mfsm_transition_t transitions[MFSM_MAX_STATES - 1u];
    mfsm_def_t def;
    uint16_t i;

    memset(states, 0, sizeof(states));
    for (i = 0u; i < (uint16_t)MFSM_MAX_STATES; ++i) {
        states[i].name = "S";
    }

    for (i = 0u; i < (uint16_t)(MFSM_MAX_STATES - 1u); ++i) {
        transitions[i].from = (mfsm_state_id)i;
        transitions[i].event = EV_GO;
        transitions[i].to = (mfsm_state_id)(i + 1u);
        transitions[i].guard = NULL;
        transitions[i].action = NULL;
    }

    def = make_def(states, (uint16_t)MFSM_MAX_STATES, transitions, (uint16_t)(MFSM_MAX_STATES - 1u), 0u);
    ASSERT_STATUS(MFSM_OK, mfsm_validate(&def));
    return true;
}

TEST(test_validate_excessive_states)
{
    const mfsm_def_t def = make_def(core_states, (uint16_t)(MFSM_MAX_STATES + 1u), NULL, 0u, 0u);
    ASSERT_STATUS(MFSM_ERR_LIMIT, mfsm_validate(&def));
    return true;
}

TEST(test_validate_zero_transitions_with_null_pointer)
{
    const mfsm_def_t def = make_def(core_states, 1u, NULL, 0u, ST_A);
    ASSERT_STATUS(MFSM_OK, mfsm_validate(&def));
    return true;
}

TEST(test_validate_nonzero_transitions_with_null_pointer)
{
    const mfsm_def_t def = make_def(core_states, 3u, NULL, 1u, ST_A);
    ASSERT_STATUS(MFSM_ERR_NULL, mfsm_validate(&def));
    return true;
}

TEST(test_validate_excessive_transitions)
{
    const mfsm_def_t def = make_def(core_states, 3u, core_transitions, (uint16_t)(MFSM_MAX_TRANSITIONS + 1u), ST_A);
    ASSERT_STATUS(MFSM_ERR_LIMIT, mfsm_validate(&def));
    return true;
}

TEST(test_validate_invalid_initial)
{
    const mfsm_def_t def = make_def(core_states, 3u, core_transitions, 3u, 8u);
    ASSERT_STATUS(MFSM_ERR_INVALID_STATE, mfsm_validate(&def));
    return true;
}

TEST(test_validate_invalid_source)
{
    const mfsm_transition_t transitions[] = {
        MFSM_TRANSITION(8u, EV_GO, ST_B, NULL, NULL)
    };
    const mfsm_def_t def = make_def(core_states, 3u, transitions, 1u, ST_A);
    ASSERT_STATUS(MFSM_ERR_INVALID_STATE, mfsm_validate(&def));
    return true;
}

TEST(test_validate_invalid_destination)
{
    const mfsm_transition_t transitions[] = {
        MFSM_TRANSITION(ST_A, EV_GO, 8u, NULL, NULL)
    };
    const mfsm_def_t def = make_def(core_states, 3u, transitions, 1u, ST_A);
    ASSERT_STATUS(MFSM_ERR_INVALID_STATE, mfsm_validate(&def));
    return true;
}

TEST(test_validate_duplicate_unguarded)
{
    const mfsm_transition_t transitions[] = {
        MFSM_TRANSITION(ST_A, EV_GO, ST_B, NULL, NULL),
        MFSM_TRANSITION(ST_A, EV_GO, ST_C, NULL, NULL)
    };
    const mfsm_def_t def = make_def(core_states, 3u, transitions, 2u, ST_A);
    ASSERT_STATUS(MFSM_ERR_DUPLICATE, mfsm_validate(&def));
    return true;
}

TEST(test_validate_exact_guarded_duplicate)
{
    const mfsm_transition_t transitions[] = {
        MFSM_TRANSITION(ST_A, EV_GO, ST_B, guard_allow, NULL),
        MFSM_TRANSITION(ST_A, EV_GO, ST_B, guard_allow, NULL)
    };
    const mfsm_def_t def = make_def(core_states, 3u, transitions, 2u, ST_A);
    ASSERT_STATUS(MFSM_ERR_DUPLICATE, mfsm_validate(&def));
    return true;
}

TEST(test_validate_guarded_priority_chain)
{
    const mfsm_transition_t transitions[] = {
        MFSM_TRANSITION(ST_A, EV_GO, ST_B, guard_false, NULL),
        MFSM_TRANSITION(ST_A, EV_GO, ST_C, guard_allow, NULL),
        MFSM_TRANSITION(ST_C, EV_GO, ST_B, NULL, NULL)
    };
    const mfsm_def_t def = make_def(core_states, 3u, transitions, 3u, ST_A);
    ASSERT_STATUS(MFSM_OK, mfsm_validate(&def));
    return true;
}

TEST(test_validate_unguarded_fallback_final)
{
    const mfsm_transition_t transitions[] = {
        MFSM_TRANSITION(ST_A, EV_GO, ST_B, guard_false, NULL),
        MFSM_TRANSITION(ST_A, EV_GO, ST_C, NULL, NULL),
        MFSM_TRANSITION(ST_C, EV_GO, ST_B, NULL, NULL)
    };
    const mfsm_def_t def = make_def(core_states, 3u, transitions, 3u, ST_A);
    ASSERT_STATUS(MFSM_OK, mfsm_validate(&def));
    return true;
}

TEST(test_validate_shadowed_after_fallback)
{
    const mfsm_transition_t transitions[] = {
        MFSM_TRANSITION(ST_A, EV_GO, ST_B, NULL, NULL),
        MFSM_TRANSITION(ST_A, EV_GO, ST_C, guard_allow, NULL),
        MFSM_TRANSITION(ST_C, EV_GO, ST_B, NULL, NULL)
    };
    const mfsm_def_t def = make_def(core_states, 3u, transitions, 3u, ST_A);
    ASSERT_STATUS(MFSM_ERR_SHADOWED, mfsm_validate(&def));
    return true;
}

TEST(test_validate_disconnected_cycle)
{
    const mfsm_state_t states[] = {
        MFSM_STATE(NULL, NULL, "A"),
        MFSM_STATE(NULL, NULL, "B"),
        MFSM_STATE(NULL, NULL, "C")
    };
    const mfsm_transition_t transitions[] = {
        MFSM_TRANSITION(ST_A, EV_GO, ST_A, NULL, NULL),
        MFSM_TRANSITION(ST_B, EV_GO, ST_C, NULL, NULL),
        MFSM_TRANSITION(ST_C, EV_GO, ST_B, NULL, NULL)
    };
    const mfsm_def_t def = make_def(states, 3u, transitions, 3u, ST_A);
    ASSERT_STATUS(MFSM_ERR_UNREACHABLE, mfsm_validate(&def));
    return true;
}

TEST(test_validate_self_transition_with_unreachable_state)
{
    const mfsm_state_t states[] = {
        MFSM_STATE(NULL, NULL, "A"),
        MFSM_STATE(NULL, NULL, "B")
    };
    const mfsm_transition_t transitions[] = {
        MFSM_TRANSITION(ST_A, EV_GO, ST_A, NULL, NULL)
    };
    const mfsm_def_t def = make_def(states, 2u, transitions, 1u, ST_A);
    ASSERT_STATUS(MFSM_ERR_UNREACHABLE, mfsm_validate(&def));
    return true;
}

TEST(test_validate_reachable_graph)
{
    const mfsm_transition_t transitions[] = {
        MFSM_TRANSITION(ST_A, EV_GO, ST_B, NULL, NULL),
        MFSM_TRANSITION(ST_B, EV_GO, ST_C, NULL, NULL),
        MFSM_TRANSITION(ST_C, EV_GO, ST_A, NULL, NULL)
    };
    const mfsm_def_t def = make_def(core_states, 3u, transitions, 3u, ST_A);
    ASSERT_STATUS(MFSM_OK, mfsm_validate(&def));
    return true;
}

TEST(test_init_and_query)
{
    test_ctx_t ctx;
    mfsm_t fsm;
    mfsm_state_id current = 0u;
    const char *name = NULL;
    bool exists = false;

    ctx_reset(&ctx);
    ctx.self = &fsm;

    ASSERT_STATUS(MFSM_OK, mfsm_init(&fsm, &core_def, &ctx));
    ASSERT_STATUS(MFSM_OK, mfsm_current(&fsm, &current));
    ASSERT_STATUS(MFSM_OK, mfsm_state_name(&fsm, &name));
    ASSERT_STATUS(MFSM_OK, mfsm_has_transition(&fsm, EV_GO, &exists));
    ASSERT_U8(ST_A, current);
    ASSERT_STR("A", name);
    ASSERT_TRUE(exists);
    ASSERT_U8(1u, ctx.enters[ST_A]);
    return true;
}

TEST(test_uninitialized_instance)
{
    mfsm_t fsm;
    mfsm_state_id current = 99u;
    bool exists = true;
    memset(&fsm, 0, sizeof(fsm));
    ASSERT_STATUS(MFSM_ERR_UNINITIALIZED, mfsm_dispatch(&fsm, EV_GO));
    ASSERT_STATUS(MFSM_ERR_UNINITIALIZED, mfsm_reset(&fsm));
    ASSERT_STATUS(MFSM_ERR_UNINITIALIZED, mfsm_current(&fsm, &current));
    ASSERT_STATUS(MFSM_ERR_UNINITIALIZED, mfsm_has_transition(&fsm, EV_GO, &exists));
    ASSERT_U8(99u, current);
    ASSERT_TRUE(exists);
    return true;
}

TEST(test_dispatch_order_and_visibility)
{
    test_ctx_t ctx;
    mfsm_t fsm;
    mfsm_state_id current = 0u;

    ctx_reset(&ctx);
    ctx.self = &fsm;
    ASSERT_STATUS(MFSM_OK, mfsm_init(&fsm, &core_def, &ctx));
    ASSERT_STATUS(MFSM_OK, mfsm_set_trace(&fsm, trace_capture));
    memset(ctx.sequence, 0, sizeof(ctx.sequence));
    ctx.sequence_len = 0u;

    ASSERT_STATUS(MFSM_OK, mfsm_dispatch(&fsm, EV_GO));
    ASSERT_STATUS(MFSM_OK, mfsm_current(&fsm, &current));
    ASSERT_U8(ST_B, current);
    ASSERT_UINT(5u, ctx.sequence_len);
    ASSERT_U8(5u, ctx.sequence[0]);
    ASSERT_U8(2u, ctx.sequence[1]);
    ASSERT_U8(3u, ctx.sequence[2]);
    ASSERT_U8(4u, ctx.sequence[3]);
    ASSERT_U8(6u, ctx.sequence[4]);
    ASSERT_U8(ST_B, ctx.observed_state);
    ASSERT_U8(MFSM_TRACE_KIND_TRANSITION, ctx.last_trace.kind);
    ASSERT_U8(ST_A, ctx.last_trace.from);
    ASSERT_U8(EV_GO, ctx.last_trace.event);
    ASSERT_U8(ST_B, ctx.last_trace.to);
    return true;
}

TEST(test_guard_reject)
{
    const mfsm_state_t states[] = {
        MFSM_STATE(NULL, NULL, "A"),
        MFSM_STATE(NULL, NULL, "B")
    };
    const mfsm_transition_t transitions[] = {
        MFSM_TRANSITION(ST_A, EV_GO, ST_B, guard_false, NULL)
    };
    test_ctx_t ctx;
    mfsm_t fsm;
    mfsm_state_id current = 0u;

    ctx_reset(&ctx);
    ctx.self = &fsm;
    const mfsm_def_t def = make_def(states, 2u, transitions, 1u, ST_A);
    ASSERT_STATUS(MFSM_OK, mfsm_init(&fsm, &def, &ctx));
    ASSERT_STATUS(MFSM_ERR_GUARD_REJECT, mfsm_dispatch(&fsm, EV_GO));
    ASSERT_STATUS(MFSM_OK, mfsm_current(&fsm, &current));
    ASSERT_U8(ST_A, current);
    ASSERT_UINT(1u, ctx.guard_calls);
    return true;
}

TEST(test_invalid_current_state_fails_closed)
{
    test_ctx_t ctx;
    mfsm_t fsm;

    ctx_reset(&ctx);
    ctx.self = &fsm;
    ASSERT_STATUS(MFSM_OK, mfsm_init(&fsm, &core_def, &ctx));
    fsm.current = 99u;
    ASSERT_STATUS(MFSM_ERR_INVALID_STATE, mfsm_dispatch(&fsm, EV_GO));
    ASSERT_UINT(0u, ctx.exits[ST_A]);
    ASSERT_UINT(1u, ctx.enters[ST_A]);
    return true;
}

TEST(test_mutated_destination_fails_closed)
{
    mfsm_transition_t transitions[] = {
        MFSM_TRANSITION(ST_A, EV_GO, ST_B, NULL, NULL),
        MFSM_TRANSITION(ST_B, EV_GO, ST_C, NULL, NULL)
    };
    mfsm_def_t def = make_def(core_states, 3u, transitions, 2u, ST_A);
    test_ctx_t ctx;
    mfsm_t fsm;

    ctx_reset(&ctx);
    ctx.self = &fsm;
    ASSERT_STATUS(MFSM_OK, mfsm_init(&fsm, &def, &ctx));
    /* cppcheck-suppress unreadVariable */
    transitions[0].to = 99u;
    ASSERT_STATUS(MFSM_ERR_INVALID_STATE, mfsm_dispatch(&fsm, EV_GO));
    ASSERT_UINT(0u, ctx.exits[ST_A]);
    return true;
}

TEST(test_same_instance_reentrancy_from_init_enter)
{
    test_ctx_t ctx;
    mfsm_t fsm;

    ctx_reset(&ctx);
    ctx.self = &fsm;
    ctx.reenter_phase = PHASE_INIT_ENTER;
    ASSERT_STATUS(MFSM_OK, mfsm_init(&fsm, &core_def, &ctx));
    ASSERT_STATUS(MFSM_ERR_BUSY, ctx.nested_result);
    return true;
}

TEST(test_same_instance_reentrancy_from_guard)
{
    test_ctx_t ctx;
    mfsm_t fsm;

    ctx_reset(&ctx);
    ctx.self = &fsm;
    ctx.reenter_phase = PHASE_GUARD;
    ASSERT_STATUS(MFSM_OK, mfsm_init(&fsm, &core_def, &ctx));
    ASSERT_STATUS(MFSM_OK, mfsm_dispatch(&fsm, EV_GO));
    ASSERT_STATUS(MFSM_ERR_BUSY, ctx.nested_result);
    return true;
}

TEST(test_same_instance_reentrancy_from_exit)
{
    test_ctx_t ctx;
    mfsm_t fsm;
    ctx_reset(&ctx);
    ctx.self = &fsm;
    ctx.reenter_phase = PHASE_EXIT;
    ASSERT_STATUS(MFSM_OK, mfsm_init(&fsm, &core_def, &ctx));
    ASSERT_STATUS(MFSM_OK, mfsm_dispatch(&fsm, EV_GO));
    ASSERT_STATUS(MFSM_ERR_BUSY, ctx.nested_result);
    return true;
}

TEST(test_same_instance_reentrancy_from_action)
{
    test_ctx_t ctx;
    mfsm_t fsm;
    ctx_reset(&ctx);
    ctx.self = &fsm;
    ctx.reenter_phase = PHASE_ACTION;
    ASSERT_STATUS(MFSM_OK, mfsm_init(&fsm, &core_def, &ctx));
    ASSERT_STATUS(MFSM_OK, mfsm_dispatch(&fsm, EV_GO));
    ASSERT_STATUS(MFSM_ERR_BUSY, ctx.nested_result);
    return true;
}

TEST(test_same_instance_reentrancy_from_enter)
{
    test_ctx_t ctx;
    mfsm_t fsm;
    ctx_reset(&ctx);
    ctx.self = &fsm;
    ctx.reenter_phase = PHASE_ENTER;
    ASSERT_STATUS(MFSM_OK, mfsm_init(&fsm, &core_def, &ctx));
    ASSERT_STATUS(MFSM_OK, mfsm_dispatch(&fsm, EV_GO));
    ASSERT_STATUS(MFSM_ERR_BUSY, ctx.nested_result);
    return true;
}

TEST(test_same_instance_reentrancy_from_trace)
{
    test_ctx_t ctx;
    mfsm_t fsm;
    ctx_reset(&ctx);
    ctx.self = &fsm;
    ctx.reenter_phase = PHASE_TRACE;
    ASSERT_STATUS(MFSM_OK, mfsm_init(&fsm, &core_def, &ctx));
    ASSERT_STATUS(MFSM_OK, mfsm_set_trace(&fsm, trace_capture));
    ASSERT_STATUS(MFSM_OK, mfsm_dispatch(&fsm, EV_GO));
    ASSERT_STATUS(MFSM_ERR_BUSY, ctx.nested_result);
    return true;
}

TEST(test_same_instance_reentrancy_from_reset_exit_callback)
{
    const mfsm_state_t states[] = {
        MFSM_STATE(NULL, on_exit_a, "A"),
        MFSM_STATE(on_enter_b, on_exit_b, "B")
    };
    const mfsm_transition_t transitions[] = {
        MFSM_TRANSITION(ST_A, EV_GO, ST_B, NULL, NULL)
    };
    mfsm_def_t def = make_def(states, 2u, transitions, 1u, ST_A);
    test_ctx_t ctx;
    mfsm_t fsm;

    ctx_reset(&ctx);
    ctx.self = &fsm;
    ASSERT_STATUS(MFSM_OK, mfsm_init(&fsm, &def, &ctx));
    ASSERT_STATUS(MFSM_OK, mfsm_dispatch(&fsm, EV_GO));

    ctx.reenter_phase = PHASE_RESET_EXIT;
    ASSERT_STATUS(MFSM_OK, mfsm_reset(&fsm));
    ASSERT_STATUS(MFSM_ERR_BUSY, ctx.nested_result);
    return true;
}

TEST(test_same_instance_reentrancy_from_reset_enter_callback)
{
    const mfsm_state_t states[] = {
        MFSM_STATE(NULL, on_exit_a, "A"),
        MFSM_STATE(on_enter_b, on_exit_b, "B")
    };
    const mfsm_transition_t transitions[] = {
        MFSM_TRANSITION(ST_A, EV_GO, ST_B, NULL, NULL)
    };
    mfsm_def_t def = make_def(states, 2u, transitions, 1u, ST_A);
    test_ctx_t ctx;
    mfsm_t fsm;

    ctx_reset(&ctx);
    ctx.self = &fsm;
    ASSERT_STATUS(MFSM_OK, mfsm_init(&fsm, &def, &ctx));
    ASSERT_STATUS(MFSM_OK, mfsm_dispatch(&fsm, EV_GO));

    ctx.reenter_phase = PHASE_RESET_ENTER;
    ASSERT_STATUS(MFSM_OK, mfsm_reset(&fsm));
    ASSERT_STATUS(MFSM_ERR_BUSY, ctx.nested_result);
    return true;
}

TEST(test_nested_different_instance_dispatch)
{
    const mfsm_state_t states[] = {
        MFSM_STATE(NULL, NULL, "A"),
        MFSM_STATE(NULL, NULL, "B")
    };
    const mfsm_transition_t peer_transitions[] = {
        MFSM_TRANSITION(ST_A, EV_GO, ST_B, NULL, NULL)
    };
    const mfsm_transition_t outer_transitions[] = {
        MFSM_TRANSITION(ST_A, EV_GO, ST_B, NULL, dispatch_peer_action)
    };
    mfsm_def_t peer_def = make_def(states, 2u, peer_transitions, 1u, ST_A);
    mfsm_def_t outer_def = make_def(states, 2u, outer_transitions, 1u, ST_A);
    test_ctx_t ctx;
    mfsm_t outer_fsm;
    mfsm_t peer_fsm;
    mfsm_state_id current = 0u;

    ctx_reset(&ctx);
    ctx.self = &outer_fsm;
    ctx.peer = &peer_fsm;

    ASSERT_STATUS(MFSM_OK, mfsm_init(&peer_fsm, &peer_def, &ctx));
    ASSERT_STATUS(MFSM_OK, mfsm_init(&outer_fsm, &outer_def, &ctx));
    ASSERT_STATUS(MFSM_OK, mfsm_dispatch(&outer_fsm, EV_GO));
    ASSERT_STATUS(MFSM_OK, ctx.peer_result);
    ASSERT_STATUS(MFSM_OK, mfsm_current(&peer_fsm, &current));
    ASSERT_U8(ST_B, current);
    return true;
}

TEST(test_event_255_distinct_from_reset_trace)
{
    const mfsm_state_t states[] = {
        MFSM_STATE(NULL, NULL, "A"),
        MFSM_STATE(NULL, NULL, "B")
    };
    const mfsm_transition_t transitions[] = {
        MFSM_TRANSITION(ST_A, EV_MAX, ST_B, NULL, NULL),
        MFSM_TRANSITION(ST_B, EV_GO, ST_A, NULL, NULL)
    };
    mfsm_def_t def = make_def(states, 2u, transitions, 2u, ST_A);
    test_ctx_t ctx;
    mfsm_t fsm;

    ctx_reset(&ctx);
    ctx.self = &fsm;
    ASSERT_STATUS(MFSM_OK, mfsm_init(&fsm, &def, &ctx));
    ASSERT_STATUS(MFSM_OK, mfsm_set_trace(&fsm, trace_capture));
    ASSERT_STATUS(MFSM_OK, mfsm_dispatch(&fsm, EV_MAX));
    ASSERT_U8(MFSM_TRACE_KIND_TRANSITION, ctx.last_trace.kind);
    ASSERT_U8(EV_MAX, ctx.last_trace.event);
    ASSERT_STATUS(MFSM_OK, mfsm_reset(&fsm));
    ASSERT_U8(MFSM_TRACE_KIND_RESET, ctx.last_trace.kind);
    ASSERT_U8(0u, ctx.last_trace.event);
    return true;
}

TEST(test_reset_from_initial_runs_exit_and_enter)
{
    const mfsm_state_t states[] = {
        MFSM_STATE(on_enter_a, on_exit_a, "A")
    };
    mfsm_def_t def = make_def(states, 1u, NULL, 0u, ST_A);
    test_ctx_t ctx;
    mfsm_t fsm;

    ctx_reset(&ctx);
    ctx.self = &fsm;
    ASSERT_STATUS(MFSM_OK, mfsm_init(&fsm, &def, &ctx));
    ASSERT_STATUS(MFSM_OK, mfsm_reset(&fsm));
    ASSERT_UINT(2u, ctx.enters[ST_A]);
    ASSERT_UINT(1u, ctx.exits[ST_A]);
    return true;
}

TEST(test_multiple_instances_share_definition)
{
    test_ctx_t ctx_a;
    test_ctx_t ctx_b;
    mfsm_t fsm_a;
    mfsm_t fsm_b;
    mfsm_state_id current_a = 0u;
    mfsm_state_id current_b = 0u;

    ctx_reset(&ctx_a);
    ctx_reset(&ctx_b);
    ctx_a.self = &fsm_a;
    ctx_b.self = &fsm_b;

    ASSERT_STATUS(MFSM_OK, mfsm_init(&fsm_a, &core_def, &ctx_a));
    ASSERT_STATUS(MFSM_OK, mfsm_init(&fsm_b, &core_def, &ctx_b));
    ASSERT_STATUS(MFSM_OK, mfsm_dispatch(&fsm_a, EV_GO));
    ASSERT_STATUS(MFSM_OK, mfsm_current(&fsm_a, &current_a));
    ASSERT_STATUS(MFSM_OK, mfsm_current(&fsm_b, &current_b));
    ASSERT_U8(ST_B, current_a);
    ASSERT_U8(ST_A, current_b);
    return true;
}

TEST(test_reentrancy_regression_exit_nested_dispatch)
{
    test_ctx_t ctx;
    mfsm_t fsm;
    mfsm_state_id current = 0u;

    ctx_reset(&ctx);
    ctx.self = &fsm;
    ctx.reenter_phase = PHASE_EXIT;
    ASSERT_STATUS(MFSM_OK, mfsm_init(&fsm, &core_def, &ctx));
    ASSERT_STATUS(MFSM_OK, mfsm_dispatch(&fsm, EV_GO));
    ASSERT_STATUS(MFSM_ERR_BUSY, ctx.nested_result);
    ASSERT_STATUS(MFSM_OK, mfsm_current(&fsm, &current));
    ASSERT_U8(ST_B, current);
    ASSERT_UINT(0u, ctx.enters[ST_C]);
    return true;
}

TEST(test_harness_side_effect_assertion_once)
{
    unsigned int counter = 0u;
    ASSERT_TRUE((++counter) == 1u);
    ASSERT_UINT(1u, counter);
    return true;
}

int main(void)
{
    run_test("test_validate_zero_states", test_validate_zero_states);
    run_test("test_validate_max_states", test_validate_max_states);
    run_test("test_validate_excessive_states", test_validate_excessive_states);
    run_test("test_validate_zero_transitions_with_null_pointer", test_validate_zero_transitions_with_null_pointer);
    run_test("test_validate_nonzero_transitions_with_null_pointer", test_validate_nonzero_transitions_with_null_pointer);
    run_test("test_validate_excessive_transitions", test_validate_excessive_transitions);
    run_test("test_validate_invalid_initial", test_validate_invalid_initial);
    run_test("test_validate_invalid_source", test_validate_invalid_source);
    run_test("test_validate_invalid_destination", test_validate_invalid_destination);
    run_test("test_validate_duplicate_unguarded", test_validate_duplicate_unguarded);
    run_test("test_validate_exact_guarded_duplicate", test_validate_exact_guarded_duplicate);
    run_test("test_validate_guarded_priority_chain", test_validate_guarded_priority_chain);
    run_test("test_validate_unguarded_fallback_final", test_validate_unguarded_fallback_final);
    run_test("test_validate_shadowed_after_fallback", test_validate_shadowed_after_fallback);
    run_test("test_validate_disconnected_cycle", test_validate_disconnected_cycle);
    run_test("test_validate_self_transition_with_unreachable_state", test_validate_self_transition_with_unreachable_state);
    run_test("test_validate_reachable_graph", test_validate_reachable_graph);
    run_test("test_init_and_query", test_init_and_query);
    run_test("test_uninitialized_instance", test_uninitialized_instance);
    run_test("test_dispatch_order_and_visibility", test_dispatch_order_and_visibility);
    run_test("test_guard_reject", test_guard_reject);
    run_test("test_invalid_current_state_fails_closed", test_invalid_current_state_fails_closed);
    run_test("test_mutated_destination_fails_closed", test_mutated_destination_fails_closed);
    run_test("test_same_instance_reentrancy_from_init_enter", test_same_instance_reentrancy_from_init_enter);
    run_test("test_same_instance_reentrancy_from_guard", test_same_instance_reentrancy_from_guard);
    run_test("test_same_instance_reentrancy_from_exit", test_same_instance_reentrancy_from_exit);
    run_test("test_same_instance_reentrancy_from_action", test_same_instance_reentrancy_from_action);
    run_test("test_same_instance_reentrancy_from_enter", test_same_instance_reentrancy_from_enter);
    run_test("test_same_instance_reentrancy_from_trace", test_same_instance_reentrancy_from_trace);
    run_test("test_same_instance_reentrancy_from_reset_exit_callback", test_same_instance_reentrancy_from_reset_exit_callback);
    run_test("test_same_instance_reentrancy_from_reset_enter_callback", test_same_instance_reentrancy_from_reset_enter_callback);
    run_test("test_nested_different_instance_dispatch", test_nested_different_instance_dispatch);
    run_test("test_event_255_distinct_from_reset_trace", test_event_255_distinct_from_reset_trace);
    run_test("test_reset_from_initial_runs_exit_and_enter", test_reset_from_initial_runs_exit_and_enter);
    run_test("test_multiple_instances_share_definition", test_multiple_instances_share_definition);
    run_test("test_reentrancy_regression_exit_nested_dispatch", test_reentrancy_regression_exit_nested_dispatch);
    run_test("test_harness_side_effect_assertion_once", test_harness_side_effect_assertion_once);

    printf("RESULT %u/%u passed, %u failed\n", tests_passed, tests_run, tests_failed);
    return tests_failed == 0u ? 0 : 1;
}
