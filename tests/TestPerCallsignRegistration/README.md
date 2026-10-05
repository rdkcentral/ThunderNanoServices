# Per-Callsign Registration QA Plugin

`TestPerCallsignRegistration` exercises both Thunder notification registration APIs:

- `IShell::Register/Unregister` for `IPlugin::INotification` lifecycle callbacks.
- `IController::ILifeTime::Register/Unregister` for controller lifecycle and state-control callbacks.

Its API-prefixed JSON-RPC methods and separate notification counters keep the two test paths distinct.

## Build and Install

From the Integration workspace root:

```bash
cmake -G Ninja -S ThunderInterfaces -B build/ThunderInterfaces \
  -DCMAKE_INSTALL_PREFIX="$PWD/install"
cmake --build build/ThunderInterfaces --target install -j"$(nproc)"

cmake -G Ninja -S ThunderNanoServices -B build/ThunderNanoServices \
  -DCMAKE_INSTALL_PREFIX="$PWD/install" \
  -DPLUGIN_TESTPERCALLSIGNREGISTRATION=ON
cmake --build build/ThunderNanoServices --target install -j"$(nproc)"
```

Start Thunder and activate the observer:

```bash
./install/bin/Thunder -f -c install/etc/Thunder/config.json

curl -s -X POST http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":1,"method":"Controller.1.activate","params":{"callsign":"TestPerCallsignRegistration"}}'
```

Keep it active during testing. It unregisters any remaining subscriptions during `Deinitialize()`.

## Automated Tests

An optional in-process GTest suite exercises both registration APIs and the QA result controls. Enable the plugin and suite:

```bash
cmake -G Ninja -S ThunderNanoServices -B build/ThunderNanoServices \
  -DCMAKE_INSTALL_PREFIX="$PWD/install" \
  -DPLUGIN_TESTSTATECONTROL=ON \
  -DPLUGIN_TESTPERCALLSIGNREGISTRATION=ON \
  -DTESTPERCALLSIGNREGISTRATION_TESTS=ON
cmake --build build/ThunderNanoServices --target PerCallsignRegistrationTests -j"$(nproc)"
ctest --test-dir build/ThunderNanoServices --output-on-failure \
  -R PerCallsignRegistrationTests
```

The suite requires GTest, `thunder_test_support`, and the existing `TestStateControl` plugin; Thunder must be built and installed with `-DENABLE_TEST_RUNTIME=ON`. CMake skips the suite if any prerequisite is missing. It covers callsign snapshots/filtering for both APIs, global subscriptions, duplicate registration, unregister behavior, independent clear/count/latest-notification controls, and state-control snapshot, suspend, resume, unregister, and global-delivery scenarios through two `TestStateControl` instances.

## JSON-RPC Methods

| Registration API | Methods |
|---|---|
| IShell | `monitorShell`, `stopMonitoringShell`, `clearShellNotifications`, `shellNotificationCount`, `lastShellNotification` |
| IController | `monitorController`, `stopMonitoringController`, `clearControllerNotifications`, `controllerNotificationCount`, `lastControllerNotification` |

Prefix each method with `TestPerCallsignRegistration.`. For example:

```text
TestPerCallsignRegistration.monitorShell
TestPerCallsignRegistration.monitorController
```

Both monitor methods require a `callsign` parameter. A non-empty string selects one plugin; `null` selects all callsigns. Do not omit the member or use an empty `params` object. Empty string is also normalized to the global selection, but `null` is clearer.

The two APIs maintain separate notification results. Clear them independently with `clearShellNotifications` or `clearControllerNotifications`; the matching count is cumulative until cleared, and `last*Notification` returns only the latest callback.

## IShell Filtering

Replace `PluginA` and `PluginB` with two configured, independently controllable plugin callsigns. `PluginA` is selected; `PluginB` is the control.

```bash
curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":2,"method":"TestPerCallsignRegistration.clearShellNotifications","params":{}}'

curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":3,"method":"TestPerCallsignRegistration.monitorShell","params":{"callsign":"PluginA"}}'

# Clear any initial snapshot for an already active PluginA.
curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":4,"method":"TestPerCallsignRegistration.clearShellNotifications","params":{}}'

curl -s -X POST http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":5,"method":"Controller.1.deactivate","params":{"callsign":"PluginB"}}'

curl -s -X POST http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":6,"method":"Controller.1.deactivate","params":{"callsign":"PluginA"}}'

curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":7,"method":"TestPerCallsignRegistration.shellNotificationCount","params":{}}'

curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":8,"method":"TestPerCallsignRegistration.lastShellNotification","params":{}}'
```

Expected after clearing the snapshot: count `1`, latest `Deactivated:PluginA`. This verifies that Thunder filtered out `PluginB` and delivered `PluginA` through the production IShell registration.

## IController Lifecycle Filtering

The controller path reports plugin state and reason. Use the same selected/control callsigns:

```bash
curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":9,"method":"TestPerCallsignRegistration.clearControllerNotifications","params":{}}'

curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":10,"method":"TestPerCallsignRegistration.monitorController","params":{"callsign":"PluginA"}}'

# Clear the initial active-state snapshot before checking transitions.
curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":11,"method":"TestPerCallsignRegistration.clearControllerNotifications","params":{}}'

curl -s -X POST http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":12,"method":"Controller.1.deactivate","params":{"callsign":"PluginB"}}'

curl -s -X POST http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":13,"method":"Controller.1.deactivate","params":{"callsign":"PluginA"}}'

curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":14,"method":"TestPerCallsignRegistration.controllerNotificationCount","params":{}}'

curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":15,"method":"TestPerCallsignRegistration.lastControllerNotification","params":{}}'
```

Expected after clearing the snapshot: count `1`, latest `StateChange:PluginA:DEACTIVATED:REQUESTED`.

## IController State-Control Notifications

Use ThunderNanoServices `TestStateControl` as the target because it implements `IStateControl` and produces deterministic suspend/resume transitions. These transitions are required to generate `StateControlStateChange` through the controller API.

Enable both plugins and activate `TestStateControl`:

```bash
cmake -G Ninja -S ThunderNanoServices -B build/ThunderNanoServices \
  -DCMAKE_INSTALL_PREFIX="$PWD/install" \
  -DPLUGIN_TESTSTATECONTROL=ON \
  -DPLUGIN_TESTPERCALLSIGNREGISTRATION=ON
cmake --build build/ThunderNanoServices --target install -j"$(nproc)"

curl -s -X POST http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":16,"method":"Controller.1.activate","params":{"callsign":"TestStateControl"}}'
```

Register for the target and clear its initial snapshot before measuring transitions. Monitoring an active target can immediately report both `StateChange` and `StateControlStateChange`; counts below begin after the clear:

```bash
curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":17,"method":"TestPerCallsignRegistration.monitorController","params":{"callsign":"TestStateControl"}}'

curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":18,"method":"TestPerCallsignRegistration.clearControllerNotifications","params":{}}'

curl -s -X POST http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":19,"method":"Controller.1.suspend","params":{"callsign":"TestStateControl"}}'

curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":20,"method":"TestPerCallsignRegistration.controllerNotificationCount","params":{}}'

curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":21,"method":"TestPerCallsignRegistration.lastControllerNotification","params":{}}'
```

After suspend, expect count `1` and `StateControlStateChange:TestStateControl:SUSPENDED`. After `Controller.1.resume`, expect cumulative count `2` and latest `StateControlStateChange:TestStateControl:RESUMED`.

## Cleanup

Unregister active selections and restore modified plugin states:

```bash
curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":22,"method":"TestPerCallsignRegistration.stopMonitoringShell","params":{"callsign":"PluginA"}}'

curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":23,"method":"TestPerCallsignRegistration.stopMonitoringController","params":{"callsign":"PluginA"}}'

curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":24,"method":"TestPerCallsignRegistration.stopMonitoringController","params":{"callsign":"TestStateControl"}}'

curl -s -X POST http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":25,"method":"Controller.1.resume","params":{"callsign":"TestStateControl"}}'
```
