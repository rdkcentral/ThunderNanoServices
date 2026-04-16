# Design: Reverse-Path Robustness for TestChannelClosedInvoke

## Architecture Context

The test currently validates abrupt client termination impact primarily while client-to-plugin invocations are active. This design adds reverse-path resilience validation without changing the intent of the existing forward-path test.

Design constraints are aligned with:

- `Thunder/docs/thunder-architecture-brief.md`
- `Thunder/docs/SME-Notes.md`

Assumption: no ThunderNanoServices-specific architecture brief/SME notes are currently available in this workspace, so Thunder governance guidance is applied.

## Design Intent

- Keep the existing scenario stable.
- Add one new reverse-path crash-window scenario.
- Focus on robustness outcomes (survival, liveness, cleanup), not business payload semantics.

## Contract Evolution Constraint

If reverse-path scenario changes require interface expansion, use a new versioned COM interface and keep existing published interface signatures unchanged.

## Composite Client Model

In the reverse path, the client acts as a composite endpoint and exposes both:

1. Service endpoint: plugin invokes request/response methods on the client.
2. Sink endpoint: plugin invokes callback/event-style methods on the client.

Both endpoint types should be active in the same scenario before crash is triggered.

## Reverse-Path Execution Flow

1. Establish communication between plugin and composite client.
2. Start plugin-to-client activity to both endpoint types (service and sink).
3. Confirm activity reached both paths (instrumentation/log marker).
4. Trigger abrupt client termination during active reverse-path communication.
5. Verify post-crash stability and cleanup conditions.

## Post-Crash Validation

Required checks:

- Framework host process remains alive.
- Plugin remains active/responding to a health probe.
- No communication deadlock/stall in worker/dispatch handling.
- Channel/session cleanup completes without fatal assertion or crash.

Validation should be run in both debug and release builds to cover build-mode differences in assertion behavior.

## Lifecycle and Ownership Guardrails

- Maintain strict Initialize/Deinitialize symmetry for all registrations and endpoint wiring.
- Ensure every register/addref has a matching unregister/release.
- Teardown order must be safe for in-flight callbacks; unregister sink-like paths before destroying state they access.

## Logging and Assertion Guardrails

- Use Thunder logging patterns in implementation (`SYSLOG()` / `TRACE()`), not direct stdio logging for new instrumentation.
- Use `ASSERT()` only for invariants; pass/fail test behavior must be observable through explicit checks and status markers.

## Observability and Pass/Fail Signals

Include explicit markers for:

- reverse-path start,
- service-path activity observed,
- sink-path activity observed,
- crash event,
- post-crash health check result,
- cleanup completion result.

A scenario is considered passing only when all required markers indicate success.

## Risk and Mitigation

- Race-window flakiness:
  - Use deterministic synchronization points to align crash timing with active reverse-path traffic.
- False positives:
  - Require explicit post-crash probe and cleanup confirmations.
- Scope creep:
  - Keep symmetry explicitly out-of-scope for this iteration.

## Future Refinement Hooks

- Configurable crash timing profiles.
- Repeated-run stress mode.
- Expanded reverse-path diagnostics.
- Optional path toward stricter forward/reverse symmetry if later required.
