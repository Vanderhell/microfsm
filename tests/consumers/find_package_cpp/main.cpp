#include "mfsm.h"

enum AppState {
    APP_STATE_IDLE = 0,
    APP_STATE_DONE = 1
};

enum AppEvent {
    APP_EVENT_GO = 1
};

static const mfsm_state_t states[] = {
    MFSM_STATE(NULL, NULL, "IDLE"),
    MFSM_STATE(NULL, NULL, "DONE")
};

static const mfsm_transition_t transitions[] = {
    MFSM_TRANSITION(APP_STATE_IDLE, APP_EVENT_GO, APP_STATE_DONE, NULL, NULL)
};

static const mfsm_def_t definition = MFSM_DEF(states, transitions, 2u, 1u, APP_STATE_IDLE);

int main()
{
    mfsm_t fsm;
    mfsm_state_id current = 0u;

    if (mfsm_validate(&definition) != MFSM_OK) {
        return 1;
    }

    if (mfsm_init(&fsm, &definition, NULL) != MFSM_OK) {
        return 1;
    }

    if (mfsm_dispatch(&fsm, APP_EVENT_GO) != MFSM_OK) {
        return 1;
    }

    if (mfsm_current(&fsm, &current) != MFSM_OK || current != APP_STATE_DONE) {
        return 1;
    }

    return 0;
}
