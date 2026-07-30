# ES1 JSON-RPC Benchmark Plugin — Design Proposal

## 1. Purpose

Provide a **standalone Thunder plugin** that measures the JSON-RPC round-trip
performance of the WPEFramework stack on ES1-class devices.  
The plugin exposes echo endpoints over JSON-RPC.  An external client script
drives the measurements, exercising **two connection strategies** and
**three parameter-type classes**, and reports per-category statistics:
average latency, standard deviation, and round-trip time.

> **Scope note:** This is an entirely new plugin.  It is independent of any
> existing Benchmark or QA infrastructure.

---

## 2. Design Goals

| Goal | Detail |
|------|--------|
| **Configurable iterations** | The number of calls per method is set by the caller at run time. |
| **Configurable payload sizes** | String length and array element count are caller-supplied parameters. |
| **Per-type statistics** | Results are reported separately for strings, arrays (vectors), and scalar/struct types. |
| **Dual client strategy** | Every test suite is run twice: once over a **persistent WebSocket** connection and once via independent **one-shot HTTP (curl-style)** connections. |
| **Minimal device footprint** | The plugin only echoes data — no storage, no side effects, no heavy dependencies. |
| **No test framework on device** | GTest and similar frameworks are not used on the device side. |

---

## 3. Plugin Overview

**Name:** `ES1Benchmark`  
**Namespace:** `WPEFramework::Plugin`  
**Activation mode:** `Local` (in-process) — used for the initial porting and
box-verification phase.  All measured time is pure JSON-RPC
serialise → dispatch → deserialise cost with no COM-RPC hop.

> **Phase 2 note:** Once the plugin is confirmed working on the box, activation
> mode can be switched to `Activated` (OOP) to include the COM-RPC IPC cost.

### 3.1 Repository placement

```
ThunderNanoServices/
└── ES1Benchmark/
    ├── CMakeLists.txt
    ├── Module.h
    ├── Module.cpp
    ├── ES1BenchmarkData.h      # shared JSON wire types (plugin + client)
    ├── ES1Benchmark.h          # plugin class (IPlugin + JSONRPC)
    ├── ES1Benchmark.cpp        # echo handler implementations
    ├── ES1Benchmark.conf.in
    └── client/
        ├── CMakeLists.txt
        ├── Module.h
        ├── Module.cpp
        └── ES1BenchmarkClient.cpp  # standalone C++ measurement client
```

No COM-RPC interface header is required for Local mode — the plugin
registers JSON-RPC handlers directly using `PluginHost::JSONRPC::Register<>()`.

---

## 4. Shared JSON Wire Types (`ES1BenchmarkData.h`)

No COM-RPC interface is used in Local mode.  Instead, `ES1BenchmarkData.h`
defines `Core::JSON::Container` subclasses that are shared between the plugin
(server) and `ES1BenchmarkClient` (client) — both sides use the same types
for serialisation without linking one against the other.

```cpp
namespace WPEFramework {
namespace JsonData {
namespace ES1Benchmark {

    // echostring params:  { "size": <uint32>, "value": "<string>" }
    // echostring result:  { "echo":  "<string>" }
    struct StringEchoParams : public Core::JSON::Container { ... };
    struct StringEchoResult : public Core::JSON::Container { ... };

    // echoarray params:   { "count": <uint32>, "values": [<uint32>,...] }
    // echoarray result:   { "echo":  [<uint32>,...] }
    struct ArrayEchoParams  : public Core::JSON::Container { ... };
    struct ArrayEchoResult  : public Core::JSON::Container { ... };

    // Scalar methods use Core::JSON::DecUInt32 / DecUInt64 / Boolean /
    // Float / Double directly — no wrapper needed.

} // namespace ES1Benchmark
} // namespace JsonData
} // namespace WPEFramework
```

---

## 5. JSON-RPC API

All methods are registered by the plugin shell.  The client calls them over the
`com.ES1Benchmark` call-sign.

### 5.1 Method table

| JSON-RPC method | Parameter class | Configurable inputs |
|----------------|----------------|---------------------|
| `echostring`   | String         | `size` (chars), `value` |
| `echoarray`    | Array          | `count` (elements), `values` |
| `echoint32`    | Scalar         | `value` |
| `echoint64`    | Scalar         | `value` |
| `echobool`     | Scalar         | `value` |
| `echofloat`    | Scalar         | `value` |
| `echodouble`   | Scalar         | `value` |

### 5.2 Example: `echostring` request / response

```json
// Request
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "ES1Benchmark.1.echostring",
  "params": {
    "size": 2048,
    "value": "aaaa...aaaa"
  }
}

// Response
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "echo": "aaaa...aaaa"
  }
}
```

### 5.3 Example: `echoarray` request / response

```json
// Request
{
  "jsonrpc": "2.0",
  "id": 2,
  "method": "ES1Benchmark.1.echoarray",
  "params": {
    "count": 512,
    "values": [1, 2, 3, ..., 512]
  }
}

// Response
{
  "jsonrpc": "2.0",
  "id": 2,
  "result": {
    "echo": [1, 2, 3, ..., 512]
  }
}
```

---

## 6. Configuration

### 6.1 Plugin configuration (`ES1Benchmark.conf.in`)

```
autostart = "@PLUGIN_ES1BENCHMARK_AUTOSTART@"
```

The `write_config()` CMake macro generates the final `ES1Benchmark.json`
installed to `/etc/WPEFramework/plugins/`:

```json
{
  "locator": "libWPEFrameworkES1Benchmark.so",
  "classname": "ES1Benchmark",
  "autostart": true
}
```

### 6.2 Client-side run parameters

The client script (not the device plugin) owns all measurement parameters.
They are passed as command-line arguments or environment variables:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `--host` | `localhost` | Device hostname or IP |
| `--port` | `9998` | JSON-RPC port |
| `--iterations` | `100` | Calls per method per size per client-mode |
| `--string-sizes` | `64,2048,20480` | Comma-separated list of string lengths (bytes) |
| `--array-counts` | `10,512,1024` | Comma-separated list of array element counts |

Example:

```bash
ES1BenchmarkClient \
  --host 192.168.1.10 \
  --port 9998 \
  --iterations 200 \
  --string-sizes 64,512,2048,20480 \
  --array-counts 10,128,512,1024
```

---

## 7. Client Strategies

The same measurement matrix is executed under both strategies so that
connection-setup cost can be isolated from serialisation cost.

### 7.1 Persistent WebSocket client ("phone call")

```
Client                                    Device (plugin)
  |── open WebSocket once ──────────────>|
  |── call echostring (iter 1) ─────────>|
  |<─ response ───────────────────────── |
  |── call echostring (iter 2) ─────────>|
  |<─ response ───────────────────────── |
  |    ... (N iterations over same conn) |
  |── call echoarray (iter 1) ──────────>|
  |<─ response ───────────────────────── |
  |    ...                               |
  |── close connection ─────────────────>|
```

- Connection opened **once**.
- All iterations for all methods/sizes share that connection.
- Timing **excludes** TCP/WebSocket handshake overhead.
- Represents a long-lived app (e.g., a media player that stays connected).

### 7.2 One-shot HTTP client ("individual letters")

```
Client                                    Device (plugin)
  |── open connection ─────────────────>|
  |── call echostring (iter 1) ─────────>|
  |<─ response ───────────────────────── |
  |── close connection ─────────────────|

  |── open NEW connection ──────────────>|
  |── call echostring (iter 2) ─────────>|
  |<─ response ───────────────────────── |
  |── close connection ─────────────────|
  ...
```

- A **fresh connection** is opened for every single call.
- Timing **includes** TCP + WebSocket handshake per call.
- Represents a monitoring script or cold-start caller.

---

## 8. Measurement Methodology

For each `(method, size, client-strategy)` combination:

1. **Warm-up:** Discard the first call to avoid JIT / page-fault noise.
2. **Timed loop:** Record the wall-clock time (`t_start` / `t_end`) for each
   individual call.
3. **Compute statistics** over the `N` recorded samples:

$$
\bar{t} = \frac{1}{N} \sum_{i=1}^{N} t_i
\qquad
\sigma = \sqrt{\frac{1}{N} \sum_{i=1}^{N} (t_i - \bar{t})^2}
$$

| Metric | Symbol | Description |
|--------|--------|-------------|
| **Average latency** | $\bar{t}$ | Mean call duration in µs |
| **Standard deviation** | $\sigma$ | Spread / jitter in µs |
| **Round-trip time** | RTT | Same as average latency for synchronous calls; both endpoints are reported for clarity |
| **Min / Max** | — | Useful for spotting outliers; included in raw output |

---

## 9. Result Schema

Each test produces one result record:

```json
{
  "method":          "echostring",
  "paramType":       "string",
  "payloadSize":     2048,
  "clientStrategy":  "persistent_websocket",
  "iterations":      100,
  "avgUs":           312,
  "stddevUs":        18,
  "minUs":           290,
  "maxUs":           450,
  "rttUs":           312
}
```

Results for all combinations are collected into a single JSON array and written
to a file (e.g., `results.json`) and optionally printed as a Markdown table.

### 9.1 Example table output (console)

```
Method       | Type   | Size   | Strategy          | Avg µs | Stddev µs | RTT µs
-------------|--------|--------|-------------------|--------|-----------|-------
echostring   | string |    64B | persistent_ws     |     87 |         4 |     87
echostring   | string |    64B | oneshot_curl      |    320 |        22 |    320
echostring   | string |  2 KB  | persistent_ws     |    145 |         7 |    145
echostring   | string |  2 KB  | oneshot_curl      |    410 |        35 |    410
echoarray    | array  |   10   | persistent_ws     |     92 |         5 |     92
echoarray    | array  |   10   | oneshot_curl      |    328 |        19 |    328
echoarray    | array  | 1 024  | persistent_ws     |    280 |        12 |    280
echoint32    | scalar |   n/a  | persistent_ws     |     75 |         3 |     75
echoint32    | scalar |   n/a  | oneshot_curl      |    295 |        17 |    295
...
```

---

## 10. Out-of-Scope

- GTest / CTest annotations on the device side.
- Pass/fail thresholds (this plugin only measures — it does not assert).
- COM-RPC proxy/stub generation (not needed for Local mode).
- Any dependency on the existing `Benchmark` plugin or `IBenchmark` / `IBenchmarkPayload` interfaces.
- `Activated` (OOP) mode — deferred to Phase 2 once Local mode is verified on the box.

---

## 11. Deliverables Summary

| Deliverable | Location |
|-------------|----------|
| `ES1BenchmarkData.h` — shared JSON wire types | `ThunderNanoServices/ES1Benchmark/` |
| `ES1Benchmark.h` / `ES1Benchmark.cpp` — plugin | `ThunderNanoServices/ES1Benchmark/` |
| `ES1Benchmark.conf.in` — plugin config template | `ThunderNanoServices/ES1Benchmark/` |
| `PLUGIN_ES1BENCHMARK` CMake option | `ThunderNanoServices/CMakeLists.txt` |
| `ES1BenchmarkClient` — standalone C++ client | `ThunderNanoServices/ES1Benchmark/client/` |
| `PLUGIN_ES1BENCHMARK_CLIENT` CMake option | `ThunderNanoServices/ES1Benchmark/CMakeLists.txt` |
| `wpeframework-es1benchmark_git.bb` — BitBake recipe | `meta-rdk-video/recipes-extended/wpeframework-es1benchmark/` |
| `results.json` schema | defined in §9 above |
