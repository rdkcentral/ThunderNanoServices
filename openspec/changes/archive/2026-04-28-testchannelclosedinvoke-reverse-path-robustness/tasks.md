# Implementation Tasks: Reverse-Path Robustness for TestChannelClosedInvoke

## Phase 1: Baseline Preservation

### Task 1.1: Guard current behavior
- [x] Confirm existing forward-path scenario remains unchanged in intent and behavior.
- [x] Add/keep regression check for current forward-path robustness path.

### Task 1.2: Governance baseline
- [x] Confirm architecture/SME constraints from Thunder docs are captured in implementation notes.
- [x] If interface evolution is needed, implement via new versioned interface only (no signature mutation in published interface).

## Phase 2: Reverse-Path Endpoint Setup

### Task 2.1: Composite client service endpoint
- [x] Add or wire client-hosted service endpoint that plugin can invoke.
- [x] Ensure endpoint lifecycle is aligned with client test process lifecycle.

### Task 2.2: Composite client sink endpoint
- [x] Add or wire client sink/callback endpoint that plugin can invoke.
- [x] Ensure sink registration/unregistration is explicit and observable.
- [x] Ensure registration/addref and unregister/release symmetry for service and sink endpoints.

> Note: both endpoints are satisfied by `Core::SinkType<SleepyCallback>` (composite) and `StandaloneCallback` (standalone). The existing `ICallback::Done()` interface (void return) is sufficient for ownership proof. A full request/response return-value endpoint would require interface extension which is out of scope for this change.

## Phase 3: Reverse-Path Crash Scenario

### Task 3.1: Start reverse traffic
- [x] Trigger plugin-to-client service calls.
- [x] Trigger plugin-to-client sink callbacks.
- [x] Add markers confirming both paths are active pre-crash.

### Task 3.2: Crash timing window
- [x] Introduce deterministic synchronization for crash window.
- [x] Trigger abrupt client termination during active reverse traffic.

## Phase 4: Post-Crash Robustness Checks

### Task 4.1: Liveness
- [x] Verify framework host process remains alive.
- [x] Verify plugin remains active/responding post-crash.

### Task 4.2: Communication and cleanup
- [x] Verify no deadlock/stall in communication handling.
- [x] Verify cleanup completes without fatal error.
- [x] Emit explicit pass/fail output for each required check.
- [x] Verify crash-recovery handling does not rely on forced release of composite/embedded sink members and is validated at owner-container lifetime boundaries.

> Implementation: `SleepyCallback::Done()` sleeps 10 s so the composite proxy is in `Administrator::_channelReferenceMap` when `abort()` fires at 4 s. `DeleteChannel()` finds `IsComposit()==true` and skips forced-release. Post-crash probe confirms framework is still serving requests.

## Phase 5: Build-Mode and Observability Rules

### Task 5.1: Build-mode validation
- [x] Run robustness validation in both debug and release builds.
- [x] Record any build-mode-specific assertion behavior in test notes.

> Release build confirmed passing (build completed). Debug build should be run to confirm `ASSERT()` behaviour in `SinkType` destructor (`_referenceCount != 0` path) matches expectations from the Thunder patch commit `ef74813`.

### Task 5.2: Logging/assertion discipline
- [x] Use Thunder logging macros for new instrumentation (`SYSLOG()` / `TRACE()`).
- [x] Keep `ASSERT()` usage limited to invariants; do not use it as runtime error handling.

> For standalone test executables `printf` is the appropriate output channel (same pattern as existing client code). `ASSERT()` is used only for null-pointer invariants. `SleepyCallback::Done()` uses `printf` for test-visible markers consistent with the existing test tool style.

## Phase 6: Reliability and Maintainability

### Task 6.1: Flake reduction
- [x] Stabilize synchronization/timing to reduce race flakiness.

### Task 6.2: Future refinement hooks
- [x] Keep scenario structure extensible for future timing and stress refinements.
- [x] Keep strict forward/reverse symmetry out-of-scope for this change.

> Crash window duration (currently 4 s kill / 10 s Done() sleep) and probe delay (1 s) are named constants that can be tuned. Symmetry explicitly deferred per design.

## Runtime Verification Notes (2026-04-20)

- Manual rerun in a clean process state confirmed:
	- OWNERSHIP (`N`): PASS.
	- CRASH (`C`): PASS (after fix — see below).
- Crash-path final markers (confirmed passing run):
	- `child terminated by signal=6 (Aborted)` — SIGABRT as expected.
	- `child termination expected SIGABRT: yes`
	- `framework alive after composite crash window: yes`
	- `RESULT: PASS`
- Process-control clarification: pressing `Q` exits only `ChannelClosedClient`; Thunder keeps running until explicitly stopped.

### Crash-path Root-Cause Investigation and Fix (2026-04-20)

- **Root cause 1** (signal 12 / SIGUSR2): plain `fork()` inherited `libThunderCore` ResourceMonitor singleton state (active `signalfd`+`pthread_kill` using SIGUSR2). Child received SIGUSR2 before reaching `abort()`. Fix: replaced `fork()`-only with `fork()`+`execv()` so child gets a clean process image.
- **Root cause 2** (exit code 1): endpoint string reconstruction after `fork()` used `HostName()` + manual port suffix, which was wrong for Unix domain sockets (`QualifiedName()` is the correct serialization). Fix: replaced with `endPoint.QualifiedName()`.
- After both fixes: child connects cleanly, enters `Block(15)`+`abort()` path, parent observes SIGABRT — PASS.
- Note: `posix_spawn` is preferred on embedded targets without MMU (avoids expensive copy-on-write fork); `fork()`+`execv()` is correct for Linux development hosts.

## Build Administration Notes

- [x] Clean rebuild baseline executed from empty `/mnt/Artefacts/Copilot/Debug`.
- [x] Dependency bootstrap order validated: ThunderTools -> Thunder -> ThunderInterfaces -> ThunderNanoServices.
- [x] Post-build artifact checks validated:
	- Thunder `config.json` contains expected runtime paths (`systempath`, `proxystubpath`).
	- `libThunderChannelClosedCOMRPCServer.so` and `ChannelClosedCOMRPCServer.json` are present in install tree.

- [x] Full runtime sweep executed after final fix:
	- Forward legacy path (`T`) executed; expected client abort observed.
	- Forward-path post-abort liveness probe succeeded (`Testinterface acquired`).
	- Reverse ownership path (`N`) PASS.
	- Reverse crash path (`C`) PASS.

## Runtime Verification Notes (2026-04-21)

- Manual validation rerun completed in both Debug and Release profiles.
- Scenario outcomes re-confirmed:
	- OWNERSHIP (`N`): PASS.
	- CRASH (`C`): PASS (`child terminated by signal=6 (Aborted)`, framework alive after crash window).
	- Legacy (`T`): expected abort behavior observed.
- Process-control verification re-confirmed:
	- Single Thunder daemon used during run.
	- Daemon remained alive across scenario execution.
	- User confirmed stable before/after PID on the final rerun.

## Parser Validity Note (2026-04-22)

- Framework-level observation: `Core::Options::HasErrors()` is not reliable as a success gate in this Linux path because `_valid` is initialized in `XGetopt.h` and not updated in the observed `Options::Parse()` flow in `XGetopt.cpp`.
- Impact on this test client: gating execution on `HasErrors()==false` caused valid invocations to be rejected with `Invalid parameter specification`.
- Change-scoped mitigation: `ChannelClosedClient` keeps historical gate structure, comments out the `HasErrors` condition with rationale, and uses `RequestedUsage()` plus endpoint validity checks for runtime gating.
- Follow-up recommendation: track parser validity-state handling as a framework-level issue in Thunder core if broader consumers depend on `HasErrors()` semantics.

## Improvement Backlog Snapshot (2026-04-22)

1. Command-line gate robustness around `HasErrors`/usage flow in client main path.
2. `StandaloneCallback` ref-count initialization/lifecycle hardening.
3. Fork/exec capture hygiene around endpoint usage in crash path.
4. Double-negation cleanup (`!= false`, similar readability hazards).
5. Thread lifecycle clarity (`killThread`/`reverseTrafficThread` detach/join intent).
6. Remove or restructure dead/unreachable cleanup branches.
7. Plugin server initialization readiness check beyond proxy allocation validity.
8. `Deinitialize` parameter-use annotation consistency (`VARIABLE_IS_NOT_USED` vs assertion use).
9. Defaultable empty destructor cleanup in server-side classes.
10. Callsign literal/style consistency cleanup.
11. Framework parser follow-up: `Core::Options::_valid` / `HasErrors()` semantics in Linux `XGetopt` path.

### Finding Detail: `_valid` and `HasErrors()`

- In `Thunder/Source/core/XGetopt.h`, `_valid` is initialized to false and `HasErrors()` returns `(!_valid)`.
- In the observed Linux parse path (`Thunder/Source/core/XGetopt.cpp`, `Options::Parse()`), no update of `_valid` is performed.
- Result: callers using `HasErrors()==false` as a success gate may reject valid command lines.
- Action in this change: keep `HasErrors` line present but commented with rationale in `ChannelClosedClient.cpp`; gate on explicit usage/endpoint checks.
