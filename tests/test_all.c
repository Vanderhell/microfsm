/*
 * microfsm test suite — minimal test framework + all test cases.
 *
 * Build: gcc -std=c99 -Wall -Wextra -I../include ../src/mfsm.c test_all.c -o test_all
 * Run:   ./test_all
 */

#include "mfsm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ── Minimal test framework ────────────────────────────────────────────── */

static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void name(void)

#define RUN_TEST(name) do {                                     \
    tests_run++;                                                \
    printf("  %-50s ", #name);                                  \
    name();                                                     \
    printf("PASS\n");                                           \
    tests_passed++;                                             \
} while (0)

#define ASSERT_EQ(expected, actual) do {                        \
    if ((expected) != (actual)) {                               \
        printf("FAIL\n    %s:%d: expected %d, got %d\n",       \
               __FILE__, __LINE__, (int)(expected), (int)(actual)); \
        tests_failed++;                                         \
        return;                                                 \
    }                                                           \
} while (0)

#define ASSERT_TRUE(expr) do {                                  \
    if (!(expr)) {                                              \
        printf("FAIL\n    %s:%d: expected true\n",              \
               __FILE__, __LINE__);                             \
        tests_failed++;                                         \
        return;                                                 \
    }                                                           \
} while (0)

#define ASSERT_FALSE(expr) do {                                 \
    if ((expr)) {                                               \
        printf("FAIL\n    %s:%d: expected false\n",             \
               __FILE__, __LINE__);                             \
        tests_failed++;                                         \
        return;                                                 \
    }                                                           \
} while (0)

#define ASSERT_STR_EQ(expected, actual) do {                    \
    if (strcmp((expected), (actual)) != 0) {                     \
        printf("FAIL\n    %s:%d: expected \"%s\", got \"%s\"\n",\
               __FILE__, __LINE__, (expected), (actual));       \
        tests_failed++;                                         \
        return;                                                 \
    }                                                           \
} while (0)

/* ── Shared test fixtures ──────────────────────────────────────────────── */

enum { ST_A, ST_B, ST_C, ST_TEST_COUNT };
enum { EV_GO, EV_BACK, EV_FAIL, EV_GUARD };

/* Tracking context for action callbacks */
typedef struct {
    int enter_count[3];
    int exit_count[3];
    int action_count;
    int guard_calls;
    bool guard_result;
} test_ctx_t;

static void reset_ctx(test_ctx_t *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->guard_result = true;
}

static void on_enter_a(void *ud) { ((test_ctx_t *)ud)->enter_count[ST_A]++; }
static void on_enter_b(void *ud) { ((test_ctx_t *)ud)->enter_count[ST_B]++; }
static void on_enter_c(void *ud) { ((test_ctx_t *)ud)->enter_count[ST_C]++; }
static void on_exit_a(void *ud)  { ((test_ctx_t *)ud)->exit_count[ST_A]++; }
static void on_exit_b(void *ud)  { ((test_ctx_t *)ud)->exit_count[ST_B]++; }

static void on_transition(void *ud) { ((test_ctx_t *)ud)->action_count++; }

static bool guard_check(void *ud) {
    test_ctx_t *ctx = (test_ctx_t *)ud;
    ctx->guard_calls++;
    return ctx->guard_result;
}

static const mfsm_state_t test_states[] = {
    [ST_A] = { .on_enter = on_enter_a, .on_exit = on_exit_a, .name = "A" },
    [ST_B] = { .on_enter = on_enter_b, .on_exit = on_exit_b, .name = "B" },
    [ST_C] = { .on_enter = on_enter_c, .on_exit = NULL,      .name = "C" },
};

static const mfsm_transition_t test_transitions[] = {
    { ST_A, EV_GO,    ST_B, NULL,        on_transition },
    { ST_B, EV_GO,    ST_C, NULL,        NULL          },
    { ST_B, EV_BACK,  ST_A, NULL,        NULL          },
    { ST_C, EV_BACK,  ST_A, NULL,        NULL          },
    { ST_A, EV_GUARD, ST_C, guard_check, NULL          },
};

static const mfsm_def_t test_def = {
    .states          = test_states,
    .num_states      = ST_TEST_COUNT,
    .transitions     = test_transitions,
    .num_transitions = sizeof(test_transitions) / sizeof(test_transitions[0]),
    .initial         = ST_A,
};

/* ══════════════════════════════════════════════════════════════════════════
 * Test cases: Core
 * ══════════════════════════════════════════════════════════════════════════ */

TEST(test_init_sets_initial_state) {
    test_ctx_t ctx; reset_ctx(&ctx);
    mfsm_t fsm;
    ASSERT_EQ(MFSM_OK, mfsm_init(&fsm, &test_def, &ctx));
    ASSERT_EQ(ST_A, mfsm_current(&fsm));
}

TEST(test_init_calls_on_enter) {
    test_ctx_t ctx; reset_ctx(&ctx);
    mfsm_t fsm;
    mfsm_init(&fsm, &test_def, &ctx);
    ASSERT_EQ(1, ctx.enter_count[ST_A]);
}

TEST(test_init_null_fsm) {
    ASSERT_EQ(MFSM_ERR_NULL, mfsm_init(NULL, &test_def, NULL));
}

TEST(test_init_null_def) {
    mfsm_t fsm;
    ASSERT_EQ(MFSM_ERR_NULL, mfsm_init(&fsm, NULL, NULL));
}

TEST(test_init_invalid_initial) {
    mfsm_def_t bad_def = test_def;
    bad_def.initial = 99;
    mfsm_t fsm;
    ASSERT_EQ(MFSM_ERR_INVALID_STATE, mfsm_init(&fsm, &bad_def, NULL));
}

TEST(test_dispatch_basic_transition) {
    test_ctx_t ctx; reset_ctx(&ctx);
    mfsm_t fsm;
    mfsm_init(&fsm, &test_def, &ctx);
    ASSERT_EQ(MFSM_OK, mfsm_dispatch(&fsm, EV_GO));
    ASSERT_EQ(ST_B, mfsm_current(&fsm));
}

TEST(test_dispatch_chain) {
    test_ctx_t ctx; reset_ctx(&ctx);
    mfsm_t fsm;
    mfsm_init(&fsm, &test_def, &ctx);
    mfsm_dispatch(&fsm, EV_GO);   /* A → B */
    mfsm_dispatch(&fsm, EV_GO);   /* B → C */
    ASSERT_EQ(ST_C, mfsm_current(&fsm));
}

TEST(test_dispatch_no_transition) {
    test_ctx_t ctx; reset_ctx(&ctx);
    mfsm_t fsm;
    mfsm_init(&fsm, &test_def, &ctx);
    ASSERT_EQ(MFSM_ERR_NO_TRANSITION, mfsm_dispatch(&fsm, EV_FAIL));
    ASSERT_EQ(ST_A, mfsm_current(&fsm));  /* stays in A */
}

TEST(test_dispatch_null_fsm) {
    ASSERT_EQ(MFSM_ERR_NULL, mfsm_dispatch(NULL, EV_GO));
}

TEST(test_reset) {
    test_ctx_t ctx; reset_ctx(&ctx);
    mfsm_t fsm;
    mfsm_init(&fsm, &test_def, &ctx);
    mfsm_dispatch(&fsm, EV_GO);   /* A → B */
    ASSERT_EQ(ST_B, mfsm_current(&fsm));
    ASSERT_EQ(MFSM_OK, mfsm_reset(&fsm));
    ASSERT_EQ(ST_A, mfsm_current(&fsm));
}

TEST(test_reset_calls_exit_and_enter) {
    test_ctx_t ctx; reset_ctx(&ctx);
    mfsm_t fsm;
    mfsm_init(&fsm, &test_def, &ctx);
    mfsm_dispatch(&fsm, EV_GO);   /* A → B */

    /* Reset counters */
    memset(ctx.enter_count, 0, sizeof(ctx.enter_count));
    memset(ctx.exit_count, 0, sizeof(ctx.exit_count));

    mfsm_reset(&fsm);  /* B → A */
    ASSERT_EQ(1, ctx.exit_count[ST_B]);
    ASSERT_EQ(1, ctx.enter_count[ST_A]);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Test cases: Actions
 * ══════════════════════════════════════════════════════════════════════════ */

TEST(test_exit_action_called_on_transition) {
    test_ctx_t ctx; reset_ctx(&ctx);
    mfsm_t fsm;
    mfsm_init(&fsm, &test_def, &ctx);
    mfsm_dispatch(&fsm, EV_GO);   /* A → B */
    ASSERT_EQ(1, ctx.exit_count[ST_A]);
}

TEST(test_enter_action_called_on_transition) {
    test_ctx_t ctx; reset_ctx(&ctx);
    mfsm_t fsm;
    mfsm_init(&fsm, &test_def, &ctx);
    mfsm_dispatch(&fsm, EV_GO);   /* A → B */
    ASSERT_EQ(1, ctx.enter_count[ST_B]);
}

TEST(test_transition_action_called) {
    test_ctx_t ctx; reset_ctx(&ctx);
    mfsm_t fsm;
    mfsm_init(&fsm, &test_def, &ctx);
    mfsm_dispatch(&fsm, EV_GO);   /* A → B, has transition action */
    ASSERT_EQ(1, ctx.action_count);
}

TEST(test_no_actions_on_unhandled_event) {
    test_ctx_t ctx; reset_ctx(&ctx);
    mfsm_t fsm;
    mfsm_init(&fsm, &test_def, &ctx);

    /* Reset after init */
    memset(ctx.enter_count, 0, sizeof(ctx.enter_count));
    memset(ctx.exit_count, 0, sizeof(ctx.exit_count));
    ctx.action_count = 0;

    mfsm_dispatch(&fsm, EV_FAIL);  /* no transition from A on EV_FAIL */
    ASSERT_EQ(0, ctx.exit_count[ST_A]);
    ASSERT_EQ(0, ctx.enter_count[ST_A]);
    ASSERT_EQ(0, ctx.action_count);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Test cases: Guards
 * ══════════════════════════════════════════════════════════════════════════ */

TEST(test_guard_allows_transition) {
    test_ctx_t ctx; reset_ctx(&ctx);
    ctx.guard_result = true;
    mfsm_t fsm;
    mfsm_init(&fsm, &test_def, &ctx);
    ASSERT_EQ(MFSM_OK, mfsm_dispatch(&fsm, EV_GUARD));
    ASSERT_EQ(ST_C, mfsm_current(&fsm));
    ASSERT_EQ(1, ctx.guard_calls);
}

TEST(test_guard_rejects_transition) {
    test_ctx_t ctx; reset_ctx(&ctx);
    ctx.guard_result = false;
    mfsm_t fsm;
    mfsm_init(&fsm, &test_def, &ctx);
    ASSERT_EQ(MFSM_ERR_GUARD_REJECT, mfsm_dispatch(&fsm, EV_GUARD));
    ASSERT_EQ(ST_A, mfsm_current(&fsm));  /* stays in A */
    ASSERT_EQ(1, ctx.guard_calls);
}

TEST(test_guard_no_side_effects_on_reject) {
    test_ctx_t ctx; reset_ctx(&ctx);
    ctx.guard_result = false;
    mfsm_t fsm;
    mfsm_init(&fsm, &test_def, &ctx);

    /* Reset after init */
    memset(ctx.exit_count, 0, sizeof(ctx.exit_count));
    ctx.action_count = 0;

    mfsm_dispatch(&fsm, EV_GUARD);
    ASSERT_EQ(0, ctx.exit_count[ST_A]);   /* on_exit NOT called */
    ASSERT_EQ(0, ctx.enter_count[ST_C]);  /* on_enter NOT called (except init) */
    ASSERT_EQ(0, ctx.action_count);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Test cases: Query functions
 * ══════════════════════════════════════════════════════════════════════════ */

TEST(test_state_name) {
    test_ctx_t ctx; reset_ctx(&ctx);
    mfsm_t fsm;
    mfsm_init(&fsm, &test_def, &ctx);
    ASSERT_STR_EQ("A", mfsm_state_name(&fsm));
    mfsm_dispatch(&fsm, EV_GO);
    ASSERT_STR_EQ("B", mfsm_state_name(&fsm));
}

TEST(test_state_name_null) {
    ASSERT_STR_EQ("?", mfsm_state_name(NULL));
}

TEST(test_can_handle_true) {
    test_ctx_t ctx; reset_ctx(&ctx);
    mfsm_t fsm;
    mfsm_init(&fsm, &test_def, &ctx);
    ASSERT_TRUE(mfsm_can_handle(&fsm, EV_GO));
}

TEST(test_can_handle_false) {
    test_ctx_t ctx; reset_ctx(&ctx);
    mfsm_t fsm;
    mfsm_init(&fsm, &test_def, &ctx);
    ASSERT_FALSE(mfsm_can_handle(&fsm, EV_FAIL));
}

TEST(test_can_handle_null) {
    ASSERT_FALSE(mfsm_can_handle(NULL, EV_GO));
}

TEST(test_current_null) {
    ASSERT_EQ(0, mfsm_current(NULL));
}

/* ══════════════════════════════════════════════════════════════════════════
 * Test cases: Validation
 * ══════════════════════════════════════════════════════════════════════════ */

TEST(test_validate_ok) {
    ASSERT_EQ(MFSM_OK, mfsm_validate(&test_def));
}

TEST(test_validate_null) {
    ASSERT_EQ(MFSM_ERR_NULL, mfsm_validate(NULL));
}

TEST(test_validate_invalid_initial) {
    mfsm_def_t bad = test_def;
    bad.initial = 99;
    ASSERT_EQ(MFSM_ERR_INVALID_STATE, mfsm_validate(&bad));
}

TEST(test_validate_invalid_transition_state) {
    static const mfsm_transition_t bad_trans[] = {
        { 0, 0, 99, NULL, NULL },  /* to = 99, out of range */
    };
    static const mfsm_state_t one_state[] = {
        { .name = "ONLY" },
    };
    mfsm_def_t bad = {
        .states = one_state, .num_states = 1,
        .transitions = bad_trans, .num_transitions = 1,
        .initial = 0,
    };
    ASSERT_EQ(MFSM_ERR_INVALID_STATE, mfsm_validate(&bad));
}

TEST(test_validate_duplicate_unguarded) {
    static const mfsm_transition_t dup_trans[] = {
        { 0, 0, 1, NULL, NULL },
        { 0, 0, 1, NULL, NULL },  /* same (from, event), no guards */
    };
    static const mfsm_state_t two_states[] = {
        { .name = "A" }, { .name = "B" },
    };
    mfsm_def_t bad = {
        .states = two_states, .num_states = 2,
        .transitions = dup_trans, .num_transitions = 2,
        .initial = 0,
    };
    ASSERT_EQ(MFSM_ERR_DUPLICATE, mfsm_validate(&bad));
}

TEST(test_validate_duplicate_with_guards_ok) {
    /* Same (from, event) but both have guards — this is intentional priority */
    static const mfsm_transition_t guarded_trans[] = {
        { 0, 0, 1, guard_check, NULL },
        { 0, 0, 1, guard_check, NULL },
    };
    static const mfsm_state_t two_states[] = {
        { .name = "A" }, { .name = "B" },
    };
    mfsm_def_t ok = {
        .states = two_states, .num_states = 2,
        .transitions = guarded_trans, .num_transitions = 2,
        .initial = 0,
    };
    ASSERT_EQ(MFSM_OK, mfsm_validate(&ok));
}

TEST(test_validate_unreachable) {
    /* State 1 exists but no transition leads to it */
    static const mfsm_transition_t no_reach[] = {
        { 0, 0, 0, NULL, NULL },  /* self-transition, never goes to state 1 */
    };
    static const mfsm_state_t two_states[] = {
        { .name = "A" }, { .name = "B" },
    };
    mfsm_def_t bad = {
        .states = two_states, .num_states = 2,
        .transitions = no_reach, .num_transitions = 1,
        .initial = 0,
    };
    ASSERT_EQ(MFSM_ERR_UNREACHABLE, mfsm_validate(&bad));
}

/* ══════════════════════════════════════════════════════════════════════════
 * Test cases: Trace
 * ══════════════════════════════════════════════════════════════════════════ */

static int trace_call_count = 0;
static mfsm_state_id trace_from, trace_to;
static mfsm_event_id trace_event;

static void test_trace_fn(mfsm_state_id from, mfsm_event_id event,
                          mfsm_state_id to, void *ud) {
    (void)ud;
    trace_call_count++;
    trace_from  = from;
    trace_event = event;
    trace_to    = to;
}

TEST(test_trace_called) {
    trace_call_count = 0;
    test_ctx_t ctx; reset_ctx(&ctx);
    mfsm_t fsm;
    mfsm_init(&fsm, &test_def, &ctx);
    mfsm_set_trace(&fsm, test_trace_fn);
    mfsm_dispatch(&fsm, EV_GO);
    ASSERT_EQ(1, trace_call_count);
    ASSERT_EQ(ST_A, trace_from);
    ASSERT_EQ(EV_GO, trace_event);
    ASSERT_EQ(ST_B, trace_to);
}

TEST(test_trace_not_called_on_failure) {
    trace_call_count = 0;
    test_ctx_t ctx; reset_ctx(&ctx);
    mfsm_t fsm;
    mfsm_init(&fsm, &test_def, &ctx);
    mfsm_set_trace(&fsm, test_trace_fn);
    mfsm_dispatch(&fsm, EV_FAIL);  /* no transition */
    ASSERT_EQ(0, trace_call_count);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Test cases: Edge cases
 * ══════════════════════════════════════════════════════════════════════════ */

TEST(test_self_transition) {
    /* A → A self-transition */
    static const mfsm_state_t self_states[] = {
        { .on_enter = on_enter_a, .on_exit = on_exit_a, .name = "A" },
    };
    static const mfsm_transition_t self_trans[] = {
        { 0, 0, 0, NULL, NULL },  /* self-transition */
    };
    mfsm_def_t self_def = {
        .states = self_states, .num_states = 1,
        .transitions = self_trans, .num_transitions = 1,
        .initial = 0,
    };
    test_ctx_t ctx; reset_ctx(&ctx);
    mfsm_t fsm;
    mfsm_init(&fsm, &self_def, &ctx);
    /* After init: enter_count[A] = 1 */
    mfsm_dispatch(&fsm, 0);
    /* After self-transition: exit + enter = exit_count[A]=1, enter_count[A]=2 */
    ASSERT_EQ(0, mfsm_current(&fsm));
    ASSERT_EQ(1, ctx.exit_count[ST_A]);
    ASSERT_EQ(2, ctx.enter_count[ST_A]);
}

TEST(test_err_str) {
    ASSERT_STR_EQ("ok",            mfsm_err_str(MFSM_OK));
    ASSERT_STR_EQ("null pointer",  mfsm_err_str(MFSM_ERR_NULL));
    ASSERT_STR_EQ("no transition", mfsm_err_str(MFSM_ERR_NO_TRANSITION));
    ASSERT_STR_EQ("guard rejected", mfsm_err_str(MFSM_ERR_GUARD_REJECT));
    ASSERT_STR_EQ("unknown error", mfsm_err_str((mfsm_err_t)99));
}

TEST(test_multiple_instances_shared_def) {
    test_ctx_t ctx1, ctx2;
    reset_ctx(&ctx1);
    reset_ctx(&ctx2);
    mfsm_t fsm1, fsm2;
    mfsm_init(&fsm1, &test_def, &ctx1);
    mfsm_init(&fsm2, &test_def, &ctx2);

    mfsm_dispatch(&fsm1, EV_GO);  /* fsm1: A → B */
    /* fsm2 should still be in A */
    ASSERT_EQ(ST_B, mfsm_current(&fsm1));
    ASSERT_EQ(ST_A, mfsm_current(&fsm2));
}

/* ══════════════════════════════════════════════════════════════════════════
 * Main
 * ══════════════════════════════════════════════════════════════════════════ */

int main(void) {
    printf("\n=== microfsm test suite ===\n\n");

    printf("[Core]\n");
    RUN_TEST(test_init_sets_initial_state);
    RUN_TEST(test_init_calls_on_enter);
    RUN_TEST(test_init_null_fsm);
    RUN_TEST(test_init_null_def);
    RUN_TEST(test_init_invalid_initial);
    RUN_TEST(test_dispatch_basic_transition);
    RUN_TEST(test_dispatch_chain);
    RUN_TEST(test_dispatch_no_transition);
    RUN_TEST(test_dispatch_null_fsm);
    RUN_TEST(test_reset);
    RUN_TEST(test_reset_calls_exit_and_enter);

    printf("\n[Actions]\n");
    RUN_TEST(test_exit_action_called_on_transition);
    RUN_TEST(test_enter_action_called_on_transition);
    RUN_TEST(test_transition_action_called);
    RUN_TEST(test_no_actions_on_unhandled_event);

    printf("\n[Guards]\n");
    RUN_TEST(test_guard_allows_transition);
    RUN_TEST(test_guard_rejects_transition);
    RUN_TEST(test_guard_no_side_effects_on_reject);

    printf("\n[Query]\n");
    RUN_TEST(test_state_name);
    RUN_TEST(test_state_name_null);
    RUN_TEST(test_can_handle_true);
    RUN_TEST(test_can_handle_false);
    RUN_TEST(test_can_handle_null);
    RUN_TEST(test_current_null);

    printf("\n[Validation]\n");
    RUN_TEST(test_validate_ok);
    RUN_TEST(test_validate_null);
    RUN_TEST(test_validate_invalid_initial);
    RUN_TEST(test_validate_invalid_transition_state);
    RUN_TEST(test_validate_duplicate_unguarded);
    RUN_TEST(test_validate_duplicate_with_guards_ok);
    RUN_TEST(test_validate_unreachable);

    printf("\n[Trace]\n");
    RUN_TEST(test_trace_called);
    RUN_TEST(test_trace_not_called_on_failure);

    printf("\n[Edge Cases]\n");
    RUN_TEST(test_self_transition);
    RUN_TEST(test_err_str);
    RUN_TEST(test_multiple_instances_shared_def);

    printf("\n=== Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) {
        printf(", %d FAILED", tests_failed);
    }
    printf(" ===\n\n");

    return tests_failed > 0 ? 1 : 0;
}
