# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] — 2026-03-20

### Added

- Core state machine engine: `mfsm_init`, `mfsm_dispatch`, `mfsm_reset`.
- Query functions: `mfsm_current`, `mfsm_state_name`, `mfsm_can_handle`.
- Guard conditions on transitions.
- Entry / exit / transition action callbacks.
- Trace hook for debugging (`mfsm_set_trace`).
- Definition validation (`mfsm_validate`).
- Human-readable error strings (`mfsm_err_str`).
- Compile-time configuration macros.
- Optional state name strings (`MFSM_ENABLE_NAMES`).
- Optional trace support (`MFSM_ENABLE_TRACE`).
- Custom assert macro (`MFSM_ASSERT`).
- Full documentation: API reference, design rationale, porting guide, examples.
- Test suite: core, guards, actions, validation, edge cases.
- Examples: MQTT device, LED blink, traffic light.
