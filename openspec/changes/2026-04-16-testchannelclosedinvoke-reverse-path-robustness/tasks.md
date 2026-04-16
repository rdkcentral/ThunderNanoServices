# Implementation Tasks: Reverse-Path Robustness for TestChannelClosedInvoke

## Phase 1: Baseline Preservation

### Task 1.1: Guard current behavior
- [ ] Confirm existing forward-path scenario remains unchanged in intent and behavior.
- [ ] Add/keep regression check for current forward-path robustness path.

### Task 1.2: Governance baseline
- [ ] Confirm architecture/SME constraints from Thunder docs are captured in implementation notes.
- [ ] If interface evolution is needed, implement via new versioned interface only (no signature mutation in published interface).

## Phase 2: Reverse-Path Endpoint Setup

### Task 2.1: Composite client service endpoint
- [ ] Add or wire client-hosted service endpoint that plugin can invoke.
- [ ] Ensure endpoint lifecycle is aligned with client test process lifecycle.

### Task 2.2: Composite client sink endpoint
- [ ] Add or wire client sink/callback endpoint that plugin can invoke.
- [ ] Ensure sink registration/unregistration is explicit and observable.
- [ ] Ensure registration/addref and unregister/release symmetry for service and sink endpoints.

## Phase 3: Reverse-Path Crash Scenario

### Task 3.1: Start reverse traffic
- [ ] Trigger plugin-to-client service calls.
- [ ] Trigger plugin-to-client sink callbacks.
- [ ] Add markers confirming both paths are active pre-crash.

### Task 3.2: Crash timing window
- [ ] Introduce deterministic synchronization for crash window.
- [ ] Trigger abrupt client termination during active reverse traffic.

## Phase 4: Post-Crash Robustness Checks

### Task 4.1: Liveness
- [ ] Verify framework host process remains alive.
- [ ] Verify plugin remains active/responding post-crash.

### Task 4.2: Communication and cleanup
- [ ] Verify no deadlock/stall in communication handling.
- [ ] Verify cleanup completes without fatal error.
- [ ] Emit explicit pass/fail output for each required check.

## Phase 5: Build-Mode and Observability Rules

### Task 5.1: Build-mode validation
- [ ] Run robustness validation in both debug and release builds.
- [ ] Record any build-mode-specific assertion behavior in test notes.

### Task 5.2: Logging/assertion discipline
- [ ] Use Thunder logging macros for new instrumentation (`SYSLOG()` / `TRACE()`).
- [ ] Keep `ASSERT()` usage limited to invariants; do not use it as runtime error handling.

## Phase 6: Reliability and Maintainability

### Task 6.1: Flake reduction
- [ ] Stabilize synchronization/timing to reduce race flakiness.

### Task 6.2: Future refinement hooks
- [ ] Keep scenario structure extensible for future timing and stress refinements.
- [ ] Keep strict forward/reverse symmetry out-of-scope for this change.
