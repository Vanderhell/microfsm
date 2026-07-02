#include "mfsm.h"

enum {
    C_ST_IDLE = 0,
    C_ST_DONE = 1
};

enum {
    C_EV_GO = 1
};

static const mfsm_state_t states[] = {
    MFSM_STATE(NULL, NULL, "IDLE"),
    MFSM_STATE(NULL, NULL, "DONE")
};

static const mfsm_transition_t transitions[] = {
    MFSM_TRANSITION(C_ST_IDLE, C_EV_GO, C_ST_DONE, NULL, NULL)
};

static const mfsm_def_t definition = MFSM_DEF(states, transitions, 2u, 1u, C_ST_IDLE);

int main(void)
{
    mfsm_t fsm;
    bool exists = false;

    if (mfsm_validate(&definition) != MFSM_OK) {
        return 1;
    }

    if (mfsm_init(&fsm, &definition, NULL) != MFSM_OK) {
        return 1;
    }

    if (mfsm_has_transition(&fsm, C_EV_GO, &exists) != MFSM_OK || !exists) {
        return 1;
    }

    return mfsm_dispatch(&fsm, C_EV_GO) == MFSM_OK ? 0 : 1;
}
