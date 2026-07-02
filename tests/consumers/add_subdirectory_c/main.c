#include "mfsm.h"

enum {
    APP_ST_IDLE = 0,
    APP_ST_DONE = 1
};

enum {
    APP_EV_GO = 1
};

static const mfsm_state_t states[] = {
    MFSM_STATE(NULL, NULL, "IDLE"),
    MFSM_STATE(NULL, NULL, "DONE")
};

static const mfsm_transition_t transitions[] = {
    MFSM_TRANSITION(APP_ST_IDLE, APP_EV_GO, APP_ST_DONE, NULL, NULL)
};

static const mfsm_def_t definition = MFSM_DEF(states, transitions, 2u, 1u, APP_ST_IDLE);

int main(void)
{
    mfsm_t fsm;
    mfsm_state_id current = 0u;

    if (mfsm_validate(&definition) != MFSM_OK) {
        return 1;
    }

    if (mfsm_init(&fsm, &definition, NULL) != MFSM_OK) {
        return 1;
    }

    if (mfsm_dispatch(&fsm, APP_EV_GO) != MFSM_OK) {
        return 1;
    }

    if (mfsm_current(&fsm, &current) != MFSM_OK || current != APP_ST_DONE) {
        return 1;
    }

    return 0;
}
