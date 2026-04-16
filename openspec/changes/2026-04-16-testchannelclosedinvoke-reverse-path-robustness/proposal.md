# Proposal: Extend TestChannelClosedInvoke with Reverse-Path Robustness

## Problem Statement

Current TestChannelClosedInvoke coverage is strong for the forward direction (client-to-plugin) under abrupt client termination, but incomplete for the reverse direction (plugin-to-client) during the same failure mode.

## Goal

Add reverse-path robustness validation where plugin-originated communication targets a composite client endpoint while the client is terminated abruptly.

## Architecture Alignment

This proposal is aligned to Thunder governance guidance from:

- `Thunder/docs/thunder-architecture-brief.md`
- `Thunder/docs/SME-Notes.md`

Assumption: ThunderNanoServices-specific architecture brief/SME notes are not currently present in this workspace, so Thunder architecture and SME guidance are treated as authoritative for this change.

## Scope

- Preserve existing forward-path scenario behavior and intent.
- Add one reverse-path scenario for plugin-to-client communication under abrupt client termination.
- In the reverse-path scenario, the client exposes both:
  - a service endpoint invokable by the plugin, and
  - a sink/callback endpoint invokable by the plugin.
- Validate framework/plugin liveness and channel cleanup outcomes after client crash.

## Non-goals

- Enforce strict symmetry between forward and reverse scenarios.
- Add feature behavior unrelated to resilience under channel closure.
- Add performance benchmarking.

## Proposed Change

- Extend test topology so the client can act as a composite endpoint in the reverse path.
- Trigger abrupt client termination during active plugin-to-client traffic.
- Add explicit pass/fail markers for pre-crash activity, crash event, and post-crash health and cleanup.
- If reverse-path coverage requires interface evolution, introduce a new versioned interface rather than modifying a published interface in place.

## Success Criteria

- [ ] Existing forward-path test scenario remains operational and unchanged in intent.
- [ ] New reverse-path scenario exists and exercises plugin-originated calls to both client service and sink endpoints.
- [ ] Abrupt client termination occurs while reverse-path traffic is active.
- [ ] Framework process remains alive after crash event.
- [ ] Plugin remains active/responding after crash event.
- [ ] No deadlock/stall is observed in communication handling.
- [ ] Cleanup completes without fatal errors.
- [ ] Logs/assertions clearly show reverse-path pass/fail.
- [ ] Validation is executed in both debug and release builds for meaningful robustness coverage.

## Future Refinement

This change intentionally provides a minimal reverse-path robustness addition and keeps room for future refinement in timing control, stress repetition, and deeper cleanup diagnostics.
