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
**Namespace:** `Thunder::Plugin`  
**Interface namespace:** `Thunder::Exchange`  
**Activation mode:** `Activated` (out-of-process), so real COM-RPC + JSON-RPC
serialisation overhead is measured end-to-end.

### 3.1 Repository placement

```
ThunderNanoServices/
└── ES1Benchmark/
    ├── CMakeLists.txt
    ├── Module.h
    ├── Module.cpp
    ├── ES1Benchmark.h          # plugin shell (IPlugin + JSONRPC)
    ├── ES1Benchmark.cpp
    ├── ES1BenchmarkImpl.cpp    # out-of-process implementation
    └── ES1Benchmark.conf.in

ThunderInterfaces/
└── interfaces/
    └── IES1Benchmark.h         # COM-RPC interface
```

---

## 4. Interface Definition (`IES1Benchmark.h`)

```cpp
namespace Thunder {
namespace Exchange {

    struct EXTERNAL IES1Benchmark : virtual public Core::IUnknown {
        enum { ID = ID_ES1BENCHMARK };

        ~IES1Benchmark() override = default;

        // -----------------------------------------------------------------
        // String echo — measures JSON string serialisation round-trip.
        // The server reflects the input string unchanged.
        // @param size: length of the string to echo (caller-generated)
        // @param value: the string payload
        // @param echo: the reflected string
        // -----------------------------------------------------------------
        virtual uint32_t EchoString(
            const uint32_t size,
            const string& value,
            string& echo /* @out */) = 0;

        // -----------------------------------------------------------------
        // Array (vector<uint32_t>) echo — measures array serialisation.
        // The server reflects the input vector unchanged.
        // @param count: number of elements
        // @param values: the array payload (@restrict:1..4096)
        // @param echo: the reflected array
        // -----------------------------------------------------------------
        virtual uint32_t EchoArray(
            const uint32_t count,
            const std::vector<uint32_t>& values /* @restrict:1..4096 */,
            std::vector<uint32_t>& echo /* @out @restrict:1..4096 */) = 0;

        // -----------------------------------------------------------------
        // Scalar echo — measures individual primitive type round-trips.
        // One method per scalar type so that each can be timed separately.
        // -----------------------------------------------------------------
        virtual uint32_t EchoUint32(const uint32_t value, uint32_t& echo /* @out */) = 0;
        virtual uint32_t EchoUint64(const uint64_t value, uint64_t& echo /* @out */) = 0;
        virtual uint32_t EchoBool  (const bool    value, bool&     echo /* @out */) = 0;
        virtual uint32_t EchoFloat (const float   value, float&    echo /* @out */) = 0;
        virtual uint32_t EchoDouble(const double  value, double&   echo /* @out */) = 0;
    };

} // namespace Exchange
} // namespace Thunder
```

A corresponding `ID_ES1BENCHMARK` entry is added to
`ThunderInterfaces/interfaces/Ids.h`.

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
| `echodbuble`   | Scalar         | `value` |

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
startmode = "Activated"
resumed   = "true"

configuration = JSON()
root = JSON()
root.add("mode", "@PLUGIN_ES1BENCHMARK_MODE@")
configuration.add("root", root)
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
python3 es1_benchmark.py \
  --host 192.168.1.10 \
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
- In-process (`Local`) mode performance numbers.
- Any dependency on the existing `Benchmark` plugin or `IBenchmark` / `IBenchmarkPayload` interfaces.

---

## 11. Deliverables Summary

| Deliverable | Location |
|-------------|----------|
| `IES1Benchmark.h` — COM-RPC interface | `ThunderInterfaces/interfaces/` |
| `ID_ES1BENCHMARK` ID entry | `ThunderInterfaces/interfaces/Ids.h` |
| `ES1Benchmark` plugin (shell + OOP impl) | `ThunderNanoServices/ES1Benchmark/` |
| `PLUGIN_ES1BENCHMARK` CMake option | `ThunderNanoServices/CMakeLists.txt` |
| `es1_benchmark.py` — external client | `ThunderNanoServices/ES1Benchmark/` |
| `results.json` schema | defined in §9 above |
