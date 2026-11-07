# JSONRPCThrottle

`JSONRPCThrottle` is a test plugin for verifying Thunder's per-plugin JSON-RPC
concurrency throttle. Its `delay` method keeps requests active long enough to
create contention, while `statistics` reports the concurrency observed inside
the plugin.

## Usage

### Build and run the integration test

Use clean build directories. CMake build trees are not relocatable, and a tree
copied or moved from another checkout may retain invalid absolute paths.

```bash
ROOT="$PWD"
PREFIX="$ROOT/install-throttle-test"

cmake -S "$ROOT/Thunder" -B "$ROOT/build/Thunder-throttle-test" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DBUILD_TESTS=ON \
    -DENABLE_TEST_RUNTIME=ON \
    -DTEST_RUNTIME_THREADPOOL_COUNT=8
cmake --build "$ROOT/build/Thunder-throttle-test" -j"$(nproc)"
cmake --install "$ROOT/build/Thunder-throttle-test"

cmake -S "$ROOT/ThunderNanoServices" \
    -B "$ROOT/build/ThunderNanoServices-throttle-test" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DPLUGIN_JSONRPCTHROTTLE=ON \
    -DJSONRPCTHROTTLE_TESTS=ON
cmake --build "$ROOT/build/ThunderNanoServices-throttle-test" -j"$(nproc)"
cmake --install "$ROOT/build/ThunderNanoServices-throttle-test"

cd "$ROOT/build/ThunderNanoServices-throttle-test/tests/JSONRPCThrottle/tests"
LD_LIBRARY_PATH="$PREFIX/lib:$ROOT/build/ThunderNanoServices-throttle-test/tests/JSONRPCThrottle" \
    ./JSONRPCThrottleTest
cd "$ROOT"
```

Run the commands from the workspace root, which contains `Thunder` and
`ThunderNanoServices`. GTest must be available when configuring
ThunderNanoServices. The test is skipped at configure time if either GTest or
the installed `thunder_test_support` package cannot be found.

The `HTTPThrottleFour` case exercises the production HTTP dispatch path. It
configures:

- plugin throttle: 4 requests
- channel throttle: 8 requests per connection
- test-runtime worker threads: 8
- workload: 20 concurrent HTTP requests, each delayed for 2 seconds

Each request uses a separate connection, so the plugin throttle is the limit
exercised by this test. A successful run reports all tests passed and satisfies:

- `totalCalls == 20`
- `activeCalls == 0` after all clients complete
- `maximumConcurrentCalls == 4`
- elapsed time is at least 7 seconds

With four plugin slots, the requests execute in approximately five batches and
normally take about 10 seconds. The test requires
`maximumConcurrentCalls == 4`, not merely a value below the limit.

### Run manually over HTTP

After installing Thunder and the plugin, set an explicit plugin limit in
`$PREFIX/etc/Thunder/plugins/JSONRPCThrottle.json`:

```json
{
  "locator": "libThunderJSONRPCThrottle.so",
  "classname": "JSONRPCThrottle",
  "startmode": "Activated",
  "throttle": 4
}
```

The generated plugin configuration does not currently add `throttle` itself.
If it is omitted, the plugin inherits the top-level Thunder `throttle` value.
A value of `0` means unlimited concurrency.

Merge the following settings into `$PREFIX/etc/Thunder/config.json`:

```json
{
    "channel_throttle": 8,
    "process": {
        "threadpoolcount": 8
    }
}
```

The daemon needs more worker threads than the plugin throttle because it also
uses workers for request processing. With only four workers, the observed
plugin concurrency may stop at three. Confirm that startup reports
`created threads=8` before running the manual test.

Start Thunder in one terminal:

```bash
LD_LIBRARY_PATH="$PREFIX/lib:$PREFIX/lib/thunder:$PREFIX/lib/thunder/plugins" \
    "$PREFIX/bin/Thunder" -f -c "$PREFIX/etc/Thunder/config.json"
```

The commands below use the plugin-specific endpoint. The generic `/jsonrpc`
endpoint routes through the Controller throttle and must not be used for this
test.

Reset the counters:

```bash
curl -sS -H 'Content-Type: application/json' \
    --data '{"jsonrpc":"2.0","id":1,"method":"reset"}' \
    http://127.0.0.1:55555/jsonrpc/JSONRPCThrottle
```

Send 20 concurrent requests, each delayed for 2 seconds:

```bash
seq 20 | xargs -P 20 -I '{}' curl -sS \
    -H 'Content-Type: application/json' \
    --data '{"jsonrpc":"2.0","id":{},"method":"delay","params":{"milliseconds":2000}}' \
    http://127.0.0.1:55555/jsonrpc/JSONRPCThrottle
```

Read the result:

```bash
curl -sS -H 'Content-Type: application/json' \
    --data '{"jsonrpc":"2.0","id":22,"method":"statistics"}' \
    http://127.0.0.1:55555/jsonrpc/JSONRPCThrottle
```

The final response should contain values equivalent to:

```json
{
  "totalCalls": 20,
  "activeCalls": 0,
  "maximumConcurrentCalls": 4
}
```

With the documented worker, channel, plugin, and client settings,
`maximumConcurrentCalls` should equal `4`. A lower value means another resource
is limiting execution first.

### JSON-RPC methods

| Method | Parameters | Result | Purpose |
| --- | --- | --- | --- |
| `delay` | `milliseconds` (unsigned integer, default `1000`) | none | Counts the call and sleeps so concurrent requests overlap. |
| `fast` | none | none | Counts a call without an intentional delay. |
| `statistics` | none | `totalCalls`, `activeCalls`, `maximumConcurrentCalls` | Reads the current counters. It does not increment them. |
| `reset` | none | none | Clears all counters. Use only when no measured calls are active. |

## Implementation

### Plugin measurement

`delay` and `fast` call `Enter()` before doing work and `Leave()` afterward.
`Enter()` atomically increments the total and active counters, then uses a
compare-and-exchange loop to update the maximum observed concurrency. `Leave()`
decrements the active count. `delay` sleeps between these operations, making it
the method used to expose queueing behavior.

`statistics` is deliberately outside the measured path, so reading the result
does not alter it. `reset` directly clears all three atomic counters and is
intended to delimit a measurement window while no calls are active.

### Thunder throttle path

Thunder creates a throttle queue for each plugin service. The queue's slot
count comes from the plugin descriptor's `throttle` value when set; otherwise
it uses the server's top-level `throttle` value. When all slots are in use,
additional JSON-RPC jobs remain queued. Completion calls `Pop()`, which submits
the next queued job or releases a slot when the queue is empty.

Each transport channel has a separate queue whose slot count comes from
`channel_throttle`. The integration test extends `ThunderTestRuntime` so it can
open a real HTTP port and configure this channel limit. Direct
`ThunderTestRuntime::JSONRPCLink` calls bypass the transport queues, so the
counter-focused tests validate the plugin but do not prove framework
throttling. `HTTPThrottleFour` uses TCP/HTTP specifically to pass through the
production channel and plugin queues.

### Test coverage

The suite covers basic method dispatch, counter reset and observation,
sequential accounting, requested delay duration, atomic accounting under
direct concurrent calls, and end-to-end HTTP throttling. The HTTP test uses one
connection per request; proving `channel_throttle` independently would require
multiple simultaneous requests over a persistent or multiplexed channel.