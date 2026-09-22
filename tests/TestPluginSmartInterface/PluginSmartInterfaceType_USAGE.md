# Plugin Smart Interface Type

## Quick Start

This component provides two Thunder test plugins:

- `TestSmartProvider` — provides the `IMath` interface.
- `TestSmartConsumer` — consumes the `IMath` interface from `TestSmartProvider`.

The test verifies Smart Interface behavior when the provider is activated,
deactivated, and reactivated.

Build the component:

```bash
cmake -G Ninja -S ThunderNanoServices -B build/ThunderNanoServices \
  -DCMAKE_INSTALL_PREFIX="$PWD/install" \
  -DPLUGIN_TESTSMARTINTERFACE=ON \
  -DTESTPLUGINSMARTINTERFACE_TESTS=ON

cmake --build build/ThunderNanoServices --target install -j"$(nproc)"
```

Run the integration tests:

```bash
ctest --test-dir build/ThunderNanoServices --output-on-failure \
  -R PluginSmartInterfaceIntegrationTests
```

Expected:

```text
[==========] Running 6 tests from 2 test suites.
...
[  PASSED  ] 6 tests.
```

## Run The Daemon

Start Thunder:

```bash
./install/bin/Thunder -f -c install/etc/Thunder/config.json
```

Check the plugin status:

```bash
curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"Controller.1.status",
    "params":{}
  }'
```

The response should contain:

```text
TestSmartConsumer
TestSmartProvider
```

The Consumer configuration includes:

```text
root.mode = Local
```

In Thunder, `Local` runs the plugin out of process. `Off` runs the plugin
in process; `Container` and `Distributed` select their respective execution
modes.

Because the Consumer uses `PluginSmartInterfaceType` out of process, the
Thunder worker pool must have at least two threads to avoid deadlocks.

The expected initial state is:

```text
TestSmartConsumer  -> Activated
TestSmartProvider  -> Deactivated
```

## Test Scenario

The Consumer uses `TestSmartProvider` as its Provider.

The basic lifecycle is:

```text
Consumer starts
      |
      v
Provider is inactive
      |
      | Activate Provider
      v
Provider becomes active
      |
      v
Consumer can use Provider
      |
      | Deactivate Provider
      v
Consumer can no longer use Provider
      |
      | Activate Provider
      v
Consumer can use Provider again
```

The test also verifies the case where the Provider is already active before
the Consumer starts.

## Provider Lifecycle

### Provider Inactive

With the default configuration:

```text
TestSmartConsumer  -> Activated
TestSmartProvider  -> Deactivated
```

The Consumer should not be able to use the Provider while the Provider is
inactive.

The expected operation result is:

```text
ERROR_UNAVAILABLE
```

### Activate Provider

Activate the Provider:

```bash
curl -i -X POST http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"Controller.1.activate",
    "params":{
      "callsign":"TestSmartProvider"
    }
  }'
```

Expected:

```text
HTTP 200
```

and the JSON-RPC request should complete successfully.

Verify the state:

```bash
curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{
    "jsonrpc":"2.0",
    "id":2,
    "method":"Controller.1.status",
    "params":{}
  }'
```

Expected:

```text
TestSmartProvider -> Activated
```

### Calculate Operation

When the Provider is active, send one `Calculate` request. The Consumer uses
the Smart Interface to perform both the addition and subtraction:

```bash
curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"TestSmartConsumer.1.calculate",
    "params":{
      "a":7,
      "b":5
    }
  }'
```

Expected result:

```text
{"addResult":12,"subResult":2}
```

### Deactivate Provider

Deactivate the Provider:

```bash
curl -i -X POST http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"Controller.1.deactivate",
    "params":{
      "callsign":"TestSmartProvider"
    }
  }'
```

Expected:

```text
HTTP 200
TestSmartProvider -> Deactivated
```

After the Provider is deactivated, the Consumer should no longer be able to
use the Provider.

Expected:

```text
ERROR_UNAVAILABLE
```

### Reactivate Provider

Activate the Provider again:

```bash
curl -i -X POST http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"Controller.1.activate",
    "params":{
      "callsign":"TestSmartProvider"
    }
  }'
```

Expected:

```text
HTTP 200
TestSmartProvider -> Activated
```

After the Provider becomes active again, the Consumer should regain access
to it.

`Calculate(7, 5)` should again produce:

```text
{"addResult":12,"subResult":2}
```

Allow a short period for the Provider lifecycle change to propagate before
checking the Consumer operation.

## Provider Already Active

The integration test also verifies that the Consumer can find a Provider
that was already active before the Consumer started.

Expected sequence:

```text
Provider activated
      |
      v
Provider is ACTIVE
      |
      v
Consumer activated
      |
      v
Consumer discovers Provider
      |
      v
Consumer operations succeed
```

The expected result is:

```text
Calculate(9, 4) -> addResult=13, subResult=5
```

## Integration Tests

The integration test contains six scenarios:

| Test | Expected behavior |
|---|---|
| `ConsumerIsUnavailableWhileProviderIsInactive` | Consumer cannot use an inactive Provider. |
| `ConsumerForwardsCallsAfterProviderActivation` | Consumer uses the Provider after activation. |
| `ConsumerDropsInterfaceAfterProviderDeactivation` | Consumer loses Provider access after deactivation. |
| `ConsumerReacquiresInterfaceAfterProviderReactivation` | Consumer regains Provider access after reactivation. |
| `RepeatedProviderLifecycleDoesNotLeaveAStaleInterface` | Repeated Provider lifecycle changes do not leave stale access. |
| `ProviderAlreadyActiveFixture.ConsumerFindsProviderThatWasAlreadyActive` | Consumer finds a Provider that was active before Consumer startup. |

Run the integration test directly:

```bash
ctest --test-dir build/ThunderNanoServices   --output-on-failure   -R PluginSmartInterfaceIntegrationTests
```

## Expected Results

| Scenario | Expected result |
|---|---|
| Provider inactive | Consumer cannot use Provider |
| Provider activated | Consumer can use Provider |
| `Calculate(7,5)` | `addResult=12`, `subResult=2` |
| Provider deactivated | Consumer loses Provider access |
| Provider reactivated | Consumer regains Provider access |
| Provider active before Consumer | Consumer discovers Provider |
| Repeated lifecycle | No stale Provider interface |

## QA Validation

QA should verify:

- Both plugins load successfully.
- Consumer starts in the Activated state.
- Provider starts in the Deactivated state.
- Provider activation succeeds.
- Consumer can use the Provider after activation through `Calculate`.
- Provider deactivation succeeds.
- Consumer loses access after deactivation.
- Provider reactivation succeeds.
- Consumer regains access after reactivation.
- Repeated activation/deactivation does not leave stale access.
- Consumer can discover a Provider that was already active before Consumer startup.
- The integration test completes with all six tests passing.

## Troubleshooting

### TestSmartConsumer is not listed

Verify that the Consumer plugin was built and is available in the configured
Thunder plugin directory.

### TestSmartProvider is not listed

Verify that the Provider plugin was built and is available in the configured
Thunder plugin directory.

### Provider activation fails

Check the Thunder console output for plugin loading or initialization
errors.

### Consumer cannot use the Provider

First check the Provider status:

```bash
curl -s http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  --data '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"Controller.1.status",
    "params":{}
  }'
```

If the Provider is `Deactivated`, `ERROR_UNAVAILABLE` is expected.

If the Provider is `Activated` and the Consumer still cannot use it, allow
a short period for the lifecycle change to propagate and check the Thunder
logs.

### JSON-RPC returns `Unknown method`

An `Unknown method` response means the requested JSON-RPC method is not
exposed by the current Consumer configuration. The exposed method is
`TestSmartConsumer.1.calculate`; there are no separate `.add` or `.sub`
methods.

This is separate from the Smart Interface lifecycle validation. The
integration test validates the Smart Interface behavior independently.

## Relevant Files

- `ThunderNanoServices/tests/TestPluginSmartInterface/Provider/`
- `ThunderNanoServices/tests/TestPluginSmartInterface/Consumer/`
- `ThunderNanoServices/tests/TestPluginSmartInterface/tests/PluginSmartInterfaceTestSuite.cpp`
