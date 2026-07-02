# Design Notes

## Chosen model

microfsm is a flat, table-driven FSM engine.

- Definitions are immutable caller-owned tables.
- Instances are caller-owned runtime objects.
- Dispatch is a linear scan in table order.
- Callbacks run synchronously.

## Why the layout is stable

The public structs keep the `name` pointer and trace callback field in every build so feature toggles do not silently alter ABI or installed-consumer expectations.

## Validation model

`mfsm_validate()` is structural, not heuristic.

- It checks limits and index ranges.
- It enforces ordered guarded priority groups.
- It rejects shadowed transitions after an unguarded fallback.
- It computes reachability from `initial` without recursion or heap allocation.

## Runtime model

- `mfsm_init()` validates the definition before activating the instance.
- `mfsm_dispatch()` and `mfsm_reset()` fail closed on invalid runtime state.
- Same-instance reentrancy is rejected with `MFSM_ERR_BUSY`.
- Different instances may call each other only if the application makes shared data safe.

## Non-goals

- hierarchical states
- built-in event queues
- timers or schedulers
- rollback or transactional callbacks
- persistence or crash recovery
- automatic synchronization
