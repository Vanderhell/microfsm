/*
 * This header is the source-tree equivalent of the generated installed config.
 * Installed consumers must use the generated copy emitted by the build system.
 */

#ifndef MFSM_CONFIG_H
#define MFSM_CONFIG_H

#define MFSM_CONFIG_ENABLE_NAMES 1
#define MFSM_CONFIG_ENABLE_TRACE 1
#define MFSM_CONFIG_MAX_STATES 32
#define MFSM_CONFIG_MAX_TRANSITIONS 64

#if defined(MFSM_ENABLE_NAMES) && MFSM_ENABLE_NAMES != MFSM_CONFIG_ENABLE_NAMES
#error "MFSM_ENABLE_NAMES conflicts with the installed microfsm configuration"
#endif

#if defined(MFSM_ENABLE_TRACE) && MFSM_ENABLE_TRACE != MFSM_CONFIG_ENABLE_TRACE
#error "MFSM_ENABLE_TRACE conflicts with the installed microfsm configuration"
#endif

#if defined(MFSM_MAX_STATES) && MFSM_MAX_STATES != MFSM_CONFIG_MAX_STATES
#error "MFSM_MAX_STATES conflicts with the installed microfsm configuration"
#endif

#if defined(MFSM_MAX_TRANSITIONS) && MFSM_MAX_TRANSITIONS != MFSM_CONFIG_MAX_TRANSITIONS
#error "MFSM_MAX_TRANSITIONS conflicts with the installed microfsm configuration"
#endif

#define MFSM_ENABLE_NAMES MFSM_CONFIG_ENABLE_NAMES
#define MFSM_ENABLE_TRACE MFSM_CONFIG_ENABLE_TRACE
#define MFSM_MAX_STATES MFSM_CONFIG_MAX_STATES
#define MFSM_MAX_TRANSITIONS MFSM_CONFIG_MAX_TRANSITIONS

#if (MFSM_ENABLE_NAMES != 0) && (MFSM_ENABLE_NAMES != 1)
#error "MFSM_ENABLE_NAMES must be exactly 0 or 1"
#endif

#if (MFSM_ENABLE_TRACE != 0) && (MFSM_ENABLE_TRACE != 1)
#error "MFSM_ENABLE_TRACE must be exactly 0 or 1"
#endif

#if (MFSM_MAX_STATES < 1) || (MFSM_MAX_STATES > 255)
#error "MFSM_MAX_STATES must be in the range 1..255"
#endif

#if (MFSM_MAX_TRANSITIONS < 0) || (MFSM_MAX_TRANSITIONS > 65535)
#error "MFSM_MAX_TRANSITIONS must be in the range 0..65535"
#endif

#endif /* MFSM_CONFIG_H */
