# JsonRpcMuxer Interface Specification

## Overview

The **JsonRpcMuxer** plugin extends the Thunder JSON-RPC interface to allow batching of multiple JSON-RPC method calls within a single HTTP request.

> Like a Viking longship crew rowing in perfect coordination, JsonRpcMuxer orchestrates your JSON-RPC calls – whether one disciplined stroke at a time (sequential) or with all oars striking together (parallel).

Unlike the standard JSON-RPC 2.0 batch array format, this plugin defines its own methods under `JsonRpcMuxer.1` and provides explicit control over **sequential** and **parallel** execution of grouped operations.

---

## Plugin Information

| Attribute | Value |
|------------|--------|
| **Namespace** | `JsonRpcMuxer.1` |
| **Purpose** | Execute batches of JSON-RPC calls via HTTP |
| **Interface Type** | HTTP JSON-RPC (`/jsonrpc` endpoint) |
| **Dependencies** | Thunder `Controller` plugin |
| **Plugin States** | `Activated` / `Deactivated` |

---

## Configuration

The plugin can be configured in Thunder's `config.json` or via the `Controller` API.  
Typical configuration fields control the maximum batch size, concurrency, and request timeout.

### Example Configuration
```json
{
  "callsign": "JsonRpcMuxer",
  "classname": "JsonRpcMuxer",
  "locator": "libJsonRpcMuxer.so",
  "autostart": false,
  "configuration": {
    "maxbatchsize": 10,
    "maxbatches": 5,
    "timeout": 5000
  }
}
```

### Configuration Fields

| Field | Type | Default | Description |
|--------|------|----------|--------------|
| **maxbatchsize** | integer | 10 | Maximum number of JSON-RPC calls allowed in one batch |
| **maxbatches** | integer | 5 | Maximum number of concurrent batches the plugin can process |
| **timeout** | integer | 5000 | Timeout (milliseconds) for batch processing. Use `0` for infinite timeout. |
| **autostart** | boolean | false | Whether the plugin should start automatically on launch |

If a batch exceeds the `maxbatchsize`, or if all concurrent slots are already occupied (`maxbatches` active), new requests will be rejected with an error response.

---

## Methods

### 1. `sequential`

#### Description
Executes the batch of JSON-RPC calls **in order**.  
Each call starts only after the previous one completes.  
This guarantees ordered behavior for dependent calls and avoids contention in the worker pool.  
Use this mode when call order matters or when the target services share resources.

#### Parameters
| Name | Type | Description |
|------|------|--------------|
| `params` | *array* | Array of JSON-RPC request objects. Each must contain `jsonrpc`, `id`, `method`, and optionally `params`. |

#### Example Request
```json
{
  "jsonrpc": "2.0",
  "id": 101,
  "method": "JsonRpcMuxer.1.sequential",
  "params": [
    { "id": 1, "method": "Controller.1.activate", "params": {"callsign": "Network"} },
    { "id": 2, "method": "Controller.1.status" },
    { "id": 3, "method": "Unknown.1.status" }
  ]
}
```

#### Example Response
```json
{
  "jsonrpc": "2.0",
  "id": 101,
  "result": [
    { "id": 1, "result": null },
    { "id": 2, "result": { "state": "Activated" } },
    { "id": 3, "error": { "code": -31022, "message": "ERROR_UNKNOWN_KEY" } }
  ]
}
```

#### Error Conditions
| Condition | Error Response |
|------------|----------------|
| Empty batch | `{"error": {"code": -32600, "message": "Empty message array"}}` |
| Batch size exceeds limit | `{"error": {"code": -32600, "message": "Batch size exceeds maximum allowed"}}` |
| Plugin inactive | `{"error": {"code": -32000, "message": "Service is not active"}}` |

---

### 2. `parallel`

#### Description
Executes all calls in the provided batch **concurrently**.  
Each request in the batch is dispatched independently to its target plugin and processed in parallel.

This mode can provide higher throughput for independent calls but should be used with care:  
parallel batches will create **multiple concurrent jobs in the Thunder worker pool**, which may **delay or temporarily block** other framework tasks.

> ⚠️ **Worker Pool Impact:**  
> Parallel mode submits all batch jobs simultaneously to Thunder's worker pool.  
> With the default 4 worker threads, a parallel batch of 4+ requests can **temporarily saturate the pool**.  
> This may delay other Thunder framework operations (plugin activation, JSON-RPC calls, etc.) until batch jobs complete.
>
> **Recommendations:**
> - Use `parallel` only for truly independent, non-blocking calls
> - Monitor system performance if using large parallel batches frequently

#### Parameters
Same as for `sequential`.

#### Example Request
```json
{
  "jsonrpc": "2.0",
  "id": 100,
  "method": "JsonRpcMuxer.1.parallel",
  "params": [
    { "id": 1, "method": "Controller.1.version" },
    { "id": 2, "method": "Controller.1.status" },
    { "id": 3, "method": "Controller.1.buildinfo" }
  ]
}
```

#### Example Response (with mixed results and errors)
```json
{
  "jsonrpc": "2.0",
  "id": 100,
  "result": [
    {
      "id": 1,
      "result": {
        "hash": "engineering_build_for_debug_purpose_only",
        "major": 5,
        "minor": 0,
        "patch": 0
      }
    },
    {
      "id": 2,
      "result": {
        "state": "Activated"
      }
    },
    {
      "id": 3,
      "result": {
        "systemtype": "Linux",
        "buildtype": "Debug",
        "messaging": true,
        "exceptioncatching": false,
        "deadlockdetection": false,
        "wcharsupport": false,
        "instanceidbits": 64,
        "tracelevel": 0,
        "threadpoolcount": 4,
        "comrpctimeout": 4294967295
      }
    }
  ]
}
```

#### Error Conditions
| Condition | Error Response |
|------------|----------------|
| Batch size exceeds limit | `{"error": {"code": -32600, "message": "Batch size exceeds maximum allowed"}}` |
| Empty batch | `{"error": {"code": -32600, "message": "Empty message array"}}` |
| Plugin inactive | `{"error": {"code": -32000, "message": "Service is not active"}}` |
| Too many concurrent batches | `{"error": {"code": -32000, "message": "Too many concurrent batches, try again later"}}` |

---

## Usage Recommendations

### When to Use Sequential
- ✅ Operations that depend on each other (activate plugin → configure plugin)
- ✅ Resource-constrained environments (RPi, STBs)
- ✅ Default choice for most use cases

### When to Use Parallel
- ✅ Truly independent read operations (status checks, version queries)
- ✅ Time-critical scenarios where latency reduction is essential
- ❌ NOT for write operations or operations with side effects

---

### Cancellation Behavior

When the plugin is deactivated:

1. **In-flight jobs**: Jobs already executing continue to completion, but results are discarded
2. **Queued jobs**: Jobs not yet started are cancelled immediately
3. **Response**: HTTP channel closes without sending response for cancelled batches
4. **Sequential batches**: Remaining jobs in sequence are not started

This ensures clean shutdown without corrupting Thunder's worker pool state.

---

## Timeout Behavior

If a batch exceeds the configured `timeout` (default 5000ms):
1. All in-flight jobs in the batch are marked as cancelled
2. No new jobs in the batch will start
3. An error response is sent to the client
4. Jobs already running may complete but results are discarded

**Example timeout error**:
```json
{
  "jsonrpc": "2.0",
  "id": 100,
  "error": {
    "code": -32003,
    "message": "Batch processing timed out"
  }
}
```

**Note**: The timeout applies to the **entire batch**, not individual calls. Set `timeout: 0` in configuration for infinite timeout.

---

## Example Error Responses

### Error in batched requests

```json
{
  "jsonrpc": "2.0",
  "id": 100,
  "result": [
    {
      "id": 1,
      "result": {
        "hash": "engineering_build_for_debug_purpose_only",
        "major": 5,
        "minor": 0,
        "patch": 0
      }
    },
    {
      "id": 2,
      "result": {
        "systemtype": "Linux",
        "buildtype": "Debug",
        "messaging": true,
        "exceptioncatching": false,
        "deadlockdetection": false,
        "wcharsupport": false,
        "instanceidbits": 64,
        "tracelevel": 0,
        "threadpoolcount": 4,
        "comrpctimeout": 4294967295
      }
    },
    {
      "id": 3,
      "error": {
        "code": -31022,
        "message": "ERROR_UNKNOWN_KEY"
      }
    },
    {
      "id": 4,
      "error": {
        "code": -31002,
        "message": "Requested service is not available."
      }
    }
  ]
}
```

### Batch too long
```json
{
  "jsonrpc": "2.0",
  "id": 100,
  "error": {
    "code": -31030,
    "message": "Batch size exceeds maximum allowed (10)"
  }
}
```

### Batch empty
```json
{
  "jsonrpc": "2.0",
  "id": 100,
  "error": {
    "code": -31030,
    "message": "Empty message array"
  }
}
```

### Too many concurrent batches
```json
{
  "jsonrpc": "2.0",
  "id": 100,
  "error": {
    "code": -32000,
    "message": "Too many concurrent batches, try again later"
  }
}
```

### Batch timeout
```json
{
  "jsonrpc": "2.0",
  "id": 100,
  "error": {
    "code": -32003,
    "message": "Batch processing timed out"
  }
}
```

---

## Error Codes Reference

| Code | Thunder Constant | Meaning |
|------|------------------|---------|
| -32600 | ERROR_BAD_REQUEST | Invalid request format or parameters |
| -32000 | ERROR_UNAVAILABLE | Plugin inactive or service unavailable |
| -32003 | ERROR_TIMEDOUT | Batch processing exceeded timeout limit |
| -31030 | ERROR_BAD_REQUEST | Batch validation failed (size/empty) |
| -31022 | ERROR_UNKNOWN_KEY | Unknown method or plugin |
| -31002 | ERROR_UNAVAILABLE | Requested service not available |

**Note**: Individual batched requests may return their own error codes if they fail. The batch itself succeeds as long as it's properly formatted and accepted by JsonRpcMuxer.