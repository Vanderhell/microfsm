# Design Rationale

This document explains **why** microfsm is built the way it is. Every
design decision involves tradeoffs; this is the record of which tradeoffs
were chosen and why.

---

## 1. Table-driven vs code-driven

**Decision:** Machine behaviour is defined entirely by const data tables
(arrays of `mfsm_state_t` and `mfsm_transition_t`), not by code structure.

**Alternatives considered:**

- **Switch/case state machine** — the classic approach. Fast dispatch, but
  the machine logic is scattered across switch cases and hard to visualise.
  Adding a state means touching multiple places. No separation between
  definition and execution.

- **State pattern (OOP)** — each state is an object with virtual methods.
  Clean, but requires dynamic dispatch, vtables, and heap allocation. Not
  practical on Cortex-M0 with 2 KB RAM.

- **Code generation from UML/graphviz** — powerful, but adds a build step,
  requires external tools, and the generated code is hard to debug.

**Why tables win for embedded:**

- The definition is `const` and can live entirely in flash/ROM. On a
  microcontroller with 32 KB flash and 4 KB RAM, this matters enormously.
- The definition is **inspectable at runtime** — you can iterate states
  and transitions for validation, visualisation, or auto-generated docs.
- Adding a state or transition is a one-line change in one place.
- The engine is generic — the same ~300 lines of C work for any machine.

**Tradeoff accepted:** Linear scan of the transition table makes dispatch
O(n) where n is the number of transitions. For the typical IoT device with
10–50 transitions, this is irrelevant (< 1 µs on a 48 MHz Cortex-M0).
If someone has 1000+ transitions, they should use a lookup table or hash
map — but that is outside the scope of this library.

---

## 2. Flat transitions vs hierarchical states (HSM)

**Decision:** microfsm implements flat finite state machines (FSM), not
hierarchical state machines (HSM).

**Why:**

- HSMs add significant complexity: state nesting, history pseudostates,
  event bubbling, entry/exit ordering for nested states. This roughly
  triples the engine size and makes behaviour harder to reason about.
- In practice, 90% of IoT state machines are flat. A device with 5–15
  states and 20–40 transitions does not need hierarchy.
- Flat FSMs are trivially testable — every path is a simple sequence of
  events. HSMs require testing at multiple nesting levels.

**Tradeoff accepted:** Users who genuinely need HSMs should use a different
library (e.g., QP/C). microfsm does not try to be everything.

**Mitigation:** The `user_data` pointer can carry a secondary FSM instance,
allowing manual composition. A "connecting" state can internally run a
sub-FSM for the connection handshake. This is explicit and debuggable.

---

## 3. Linear scan vs lookup table for dispatch

**Decision:** `mfsm_dispatch()` scans the transition array linearly.

**Alternatives considered:**

- **2D lookup table** `[state][event] → transition index`. O(1) dispatch,
  but wastes memory: a 16-state, 16-event machine needs a 256-entry table
  (256 bytes minimum), most of which are empty.

- **Hash map.** Overhead of hashing and collision handling is not worth it
  for < 100 entries.

- **Sorted array + binary search.** O(log n), but complicates the
  definition syntax and saves microseconds at best.

**Why linear scan:**

- Transition arrays are typically 10–50 entries. Linear scan of 50
  entries takes ~50 comparisons of two `uint8_t` values. On a 48 MHz MCU,
  this is well under 5 µs.
- Linear scan preserves **insertion order priority** — when two transitions
  match (because they have different guards), the first one wins. This is
  intuitive and useful.
- Zero additional memory beyond the transition array itself.

**Tradeoff accepted:** If someone builds a 500-transition machine,
dispatch will take ~50 µs. This is acceptable for IoT workloads (typical
event rates: 1–100 Hz). If sub-microsecond dispatch is needed, the user
can build a lookup table externally and use microfsm only for the callback
orchestration.

---

## 4. uint8_t IDs vs enums vs strings

**Decision:** State and event identifiers are `uint8_t` (via typedef
`mfsm_state_id`, `mfsm_event_id`). The user defines enums for readability.

**Why:**

- `uint8_t` is the smallest practical integer type and keeps structures
  compact. On a Cortex-M0, struct packing with 32-bit pointers and 8-bit
  IDs keeps `mfsm_transition_t` at 16 bytes.
- Using a typedef means the width can be widened to `uint16_t` by changing
  one line if someone needs > 255 states (unlikely in practice).
- String-based IDs (like `"IDLE"`, `"CONNECTING"`) are flexible but require
  `strcmp` on every dispatch — unacceptable on MCUs.
- The user's enums provide compile-time type safety without any runtime cost.

**Tradeoff accepted:** No compile-time enforcement that a `mfsm_state_id`
value is in range. This is caught at runtime by `mfsm_validate()` and by
`MFSM_ASSERT` in debug builds.

---

## 5. Callbacks vs event queues

**Decision:** microfsm calls callbacks synchronously during `mfsm_dispatch()`.
It does not provide a built-in event queue.

**Why:**

- An event queue requires memory (static buffer or heap), a policy for
  overflow (drop? block? overwrite?), and thread-safety (critical sections
  or atomic ops). These are platform-specific decisions that vary wildly
  between bare-metal, FreeRTOS, Zephyr, Linux, etc.
- Synchronous callbacks are the simplest correct model: the caller knows
  that when `mfsm_dispatch()` returns, all side effects have completed.
- Users who need queuing can wrap dispatch in their platform's queue
  mechanism (FreeRTOS xQueue, Zephyr k_msgq, POSIX pipe, etc.).

**Tradeoff accepted:** Actions cannot post events to the same machine
(re-entrancy is not supported). This is documented and enforced by
convention. If `on_enter` of state B needs to immediately trigger a
transition, the caller should dispatch the follow-up event after
`mfsm_dispatch()` returns.

---

## 6. No dynamic allocation

**Decision:** microfsm never calls `malloc`, `calloc`, `realloc`, or `free`.
All memory is caller-provided (stack or static).

**Why:**

- Many bare-metal projects disable the heap entirely. Some safety standards
  (MISRA, DO-178C) restrict or forbid dynamic allocation.
- Stack allocation is deterministic — you always know the worst-case memory
  usage at compile time.
- It eliminates an entire class of bugs (leaks, double-free, fragmentation).

**What this means for the user:**

- `mfsm_t` is allocated by the caller (on the stack or as a static global).
- `mfsm_def_t`, states, and transitions are `const` and should be static.
- There is no `mfsm_destroy()` function because there is nothing to free.

---

## 7. Separate definition and instance

**Decision:** The machine definition (`mfsm_def_t`) is separate from the
runtime instance (`mfsm_t`).

**Why:**

- The definition is `const` / ROM-safe. On a microcontroller, this means
  it lives in flash and does not consume RAM.
- Multiple instances can share one definition. Example: monitoring 4
  identical sensors, each with its own state machine, all using the same
  transition table but different `user_data`.
- It makes testing clean: you can validate the definition once, then
  create/destroy instances freely.

---

## 8. Guard priority ordering

**Decision:** When multiple transitions match `(from, event)`, the engine
takes the **first** one whose guard passes (or that has no guard).

**Why:**

- This gives the user explicit control over priority without a separate
  priority field.
- It matches how people naturally write rules: "if we can retry, go to
  CONNECTING; otherwise, go to FATAL."
- It's simple to implement (stop at first match) and simple to reason about.

**`mfsm_validate()` behaviour:** The validator warns about duplicate
`(from, event)` pairs **only if neither transition has a guard**. If both
have guards, it assumes the user intended priority ordering and stays silent.

---

## 9. Error codes vs assertions

**Decision:** microfsm uses return codes for all error reporting. Assertions
are opt-in via `MFSM_ASSERT`.

**Why:**

- On bare-metal, an unexpected `assert()` may reset the device or enter
  an infinite loop. Return codes let the caller decide how to handle errors.
- In production, you want the device to keep running even if a dispatch
  fails. In development, you want a hard crash on the first bug.
- `MFSM_ASSERT` bridges both worlds: define it in debug builds, leave it
  undefined in production.

---

## 10. Naming

**Why "microfsm"?**

- **micro** — small footprint, microcontroller-friendly.
- **fsm** — finite state machine, immediately recognisable.
- The `mfsm_` prefix is short enough for comfortable typing but unique
  enough to avoid collisions in large projects.
- Consistent with the embedded ecosystem naming convention (microjson,
  microhttpd, micropython, etc.).

---

## Summary of tradeoffs

| Design choice | Gains | Costs |
|--------------|-------|-------|
| Table-driven | ROM-safe, inspectable, easy to extend | O(n) dispatch |
| Flat FSM only | Simple, testable, small footprint | No hierarchy support |
| Linear scan | Zero extra memory, priority ordering | Slower for 500+ transitions |
| uint8_t IDs | Compact structures | Max 255 states/events |
| Sync callbacks | Simple, deterministic | No built-in event queue |
| No malloc | Deterministic, safe, portable | Caller manages memory |
| Separate def/instance | ROM-safe, shareable | Two structs to manage |
| Return codes | Resilient, production-safe | Caller must check returns |
