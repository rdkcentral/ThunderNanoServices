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
    -DENABLE_TEST_RUNTIME=ON
cmake --build "$ROOT/build/Thunder-throttle-test"
cmake --install "$ROOT/build/Thunder-throttle-test"

cmake -S "$ROOT/ThunderNanoServices" \
    -B "$ROOT/build/ThunderNanoServices-throttle-test" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DPLUGIN_JSONRPCTHROTTLE=ON \
    -DJSONRPCTHROTTLE_TESTS=ON
cmake --build "$ROOT/build/ThunderNanoServices-throttle-test"

ctest --test-dir "$ROOT/build/ThunderNanoServices-throttle-test" \
    -R '^JSONRPCThrottleTest$' \
    --output-on-failure -V
```

Run the commands from the workspace root, which contains `Thunder` and
`ThunderNanoServices`. GTest must be available when configuring
ThunderNanoServices. The test is skipped at configure time if either GTest or
the installed `thunder_test_support` package cannot be found.

The `HTTPThrottleFour` case is the end-to-end throttle proof. It configures:

- plugin throttle: 4 requests
- channel throttle: 2 requests per connection
- workload: 20 concurrent HTTP requests, each delayed for 2 seconds

Each request uses a separate connection, so the plugin throttle is the limit
exercised by this test. A successful run reports all tests passed and satisfies:

- `totalCalls == 20`
- `activeCalls == 0` after all clients complete
- `maximumConcurrentCalls <= 4`
- elapsed time is at least 7 seconds

Without throttling, the 20 requests can overlap and finish in approximately 2
seconds. With four plugin slots, they execute in approximately five batches and
normally take about 10 seconds.

### Inspect the throttle instrumentation

Run CTest with `-V`, as shown above, to display Thunder and plugin diagnostics.
The relevant messages are:

```text
[PLUGIN-THROTTLE] EXECUTE used=4 slots=4
[PLUGIN-THROTTLE] QUEUE used=4 slots=4
[PLUGIN] ACTIVE=4 MAX=4 thread=...
[PLUGIN-THROTTLE] POP used=4 slots=4
```

`QUEUE` while `used=4 slots=4` shows that Thunder held excess work before it
entered the plugin. The plugin's `MAX=4` observation independently confirms
that no more than four delayed methods ran concurrently.

The temporary framework instrumentation labels a queue as
`PLUGIN-THROTTLE` when its slot count is exactly 4 and as `CHANNEL-THROTTLE`
otherwise. Treat the label as test-specific: `used` and `slots`, together with
the plugin statistics, are the reliable evidence.

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
A value of `0` means unlimited concurrency. The top-level
`channel_throttle` setting separately limits concurrent requests on each
client connection.

Start Thunder with the installed configuration and library paths appropriate
for the local installation. With the default HTTP endpoint on port 55555,
reset the counters and submit concurrent work:

```bash
ENDPOINT=http://127.0.0.1:55555/jsonrpc

curl -sS -H 'Content-Type: application/json' \
    -d '{"jsonrpc":"2.0","id":1,"method":"JSONRPCThrottle.1.reset"}' \
    "$ENDPOINT"

seq 20 | xargs -P 20 -I '{}' curl -sS \
    -H 'Content-Type: application/json' \
    -d '{"jsonrpc":"2.0","id":{},"method":"JSONRPCThrottle.1.delay","params":{"milliseconds":2000}}' \
    "$ENDPOINT"

curl -sS -H 'Content-Type: application/json' \
    -d '{"jsonrpc":"2.0","id":22,"method":"JSONRPCThrottle.1.statistics"}' \
    "$ENDPOINT"
```

The final response should contain values equivalent to:

```json
{
  "totalCalls": 20,
  "activeCalls": 0,
  "maximumConcurrentCalls": 4
}
```

The maximum may be lower if the client or worker pool does not produce enough
overlap, but it must not exceed the configured plugin throttle.

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