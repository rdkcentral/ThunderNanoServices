# JsonRpcMuxer Plugin

## Overview

The **JsonRpcMuxer** plugin extends Thunder's JSON-RPC interface to enable efficient batching of multiple JSON-RPC method calls within a single HTTP request, with explicit control over execution order and concurrency.

> **Analogy**: Like a Viking longship crew rowing in perfect coordination, JsonRpcMuxer orchestrates your JSON-RPC calls — whether one disciplined stroke at a time (sequential) or with all oars striking together (parallel).

---

## Configuration

### Configuration Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| **maxconcurrentjobs** | uint8 | 10 | Maximum total concurrent jobs across all batches |
| **maxbatchsize** | uint8 | 100 | Maximum number of requests allowed in a single batch |

### Example Configuration

```json
{
  "callsign": "JsonRpcMuxer",
  "classname": "JsonRpcMuxer",
  "locator": "libThunderJsonRpcMuxer.so",
  "startmode": "Activated",
  "configuration": {
    "maxconcurrentjobs": 10,
    "maxbatchsize": 100
  }
}
```

### Configuration via CMake

```cmake
set(PLUGIN_JSONRPCMUXER_STARTMODE "Activated")
set(PLUGIN_JSONRPCMUXER_MAX_CONCURRENT_JOBS "10")
set(PLUGIN_JSONRPCMUXER_MAX_BATCH_SIZE "100")
```

**Rule of Thumb**: Set `maxconcurrentjobs` to 50% of `threadpoolcount`.

---

## API Methods

### 1. `sequential`

Executes batch requests **in order**, one at a time. Each request starts only after the previous completes.

**Use Cases:**
- Operations with dependencies (activate → configure → start)
- Resource-constrained environments
- Operations requiring ordered execution
- **Default choice** for most scenarios

#### Request Format

```json
{
  "jsonrpc": "2.0",
  "id": 101,
  "method": "JsonRpcMuxer.1.sequential",
  "params": [
    { "id": 1, "method": "Controller.1.activate", "params": {"callsign": "Network"} },
    { "id": 2, "method": "Network.1.status" },
    { "id": 3, "method": "Controller.1.deactivate", "params": {"callsign": "Network"} }
  ]
}
```

#### Response Format

```json
{
  "jsonrpc": "2.0",
  "id": 101,
  "result": [
    { "id": 1, "result": null },
    { "id": 2, "result": { "state": "up", "interface": "eth0" } },
    { "id": 3, "result": null }
  ]
}
```

**Execution Timeline:**
```
T1: Job 1 starts  → T2: Job 1 completes
T3: Job 2 starts  → T4: Job 2 completes
T5: Job 3 starts  → T6: Job 3 completes → Batch complete
```

---

### 2. `parallel`

Executes all batch requests **concurrently** for maximum throughput.

**Use Cases:**
- Independent read operations (status, version queries)
- Time-critical scenarios requiring low latency
- Operations with no side effects or dependencies

⚠️ **Warning**: Use with caution on resource-constrained systems!

#### Request Format

```json
{
  "jsonrpc": "2.0",
  "id": 100,
  "method": "JsonRpcMuxer.1.parallel",
  "params": [
    { "id": 1, "method": "Controller.1.version" },
    { "id": 2, "method": "Controller.1.status" },
    { "id": 3, "method": "Controller.1.buildinfo" },
    { "id": 4, "method": "DeviceInfo.1.systeminfo" }
  ]
}
```

#### Response Format

```json
{
  "jsonrpc": "2.0",
  "id": 100,
  "result": [
    { "id": 1, "result": { "major": 5, "minor": 0, "patch": 0 } },
    { "id": 2, "result": { "state": "Activated" } },
    { "id": 3, "result": { "systemtype": "Linux", "threadpoolcount": 4 } },
    { "id": 4, "result": { "version": "1.0" } }
  ]
}
```

**Execution Timeline:**
```
T1: All 4 jobs start concurrently
    ├─ Job 1 → completes T2
    ├─ Job 2 → completes T3
    ├─ Job 3 → completes T4
    └─ Job 4 → completes T5 → Batch complete
```

---

## Error Handling

### Individual Request Errors

If individual requests fail, the batch continues and returns mixed results:

```json
{
  "jsonrpc": "2.0",
  "id": 100,
  "result": [
    {
      "id": 1,
      "result": { "state": "Activated" }
    },
    {
      "id": 2,
      "error": {
        "code": -31022,
        "message": "ERROR_UNKNOWN_KEY"
      }
    },
    {
      "id": 3,
      "result": { "version": "1.0" }
    }
  ]
}
```

**Key Point**: Individual failures don't fail the entire batch!

### Batch-Level Errors

These errors reject the entire batch request:

#### Invalid Method
```json
{
  "jsonrpc": "2.0",
  "id": 100,
  "error": {
    "code": -32601,
    "message": "Invalid method, must be 'parallel' or 'sequential'"
  }
}
```

#### Parse Failure
```json
{
  "jsonrpc": "2.0",
  "id": 100,
  "error": {
    "code": -31050,
    "message": "Invalid value. \"null\" or \"[\" expected."
  }
}
```

#### Empty Batch
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

#### Batch Too Large
```json
{
  "jsonrpc": "2.0",
  "id": 100,
  "error": {
    "code": -31030,
    "message": "Batch size (150) exceeds maximum allowed (100)"
  }
}
```

#### Plugin Shutting Down
```json
{
  "jsonrpc": "2.0",
  "id": 100,
  "error": {
    "code": -31058,
    "message": "Rejected batch submission during shutdown"
  }
}
```

---

## Error Codes Reference

| JSON-RPC Code | Thunder Constant | Core Value | Scenario |
|---------------|------------------|------------|----------|
| `-31030` | ERROR_BAD_REQUEST | 30 | Empty batch or exceeds `maxbatchsize` |
| `-31050` | ERROR_PARSE_FAILURE | 50 | Malformed batch JSON |
| `-32601` | ERROR_UNKNOWN_METHOD | 53 | Invalid method (not `parallel` or `sequential`) |
| `-31058` | ERROR_ABORTED | 58 | Plugin is shutting down |

---

## Performance Considerations

### Worker Pool Impact

JsonRpcMuxer submits jobs to Thunder's shared worker pool. Understanding this interaction is critical:

**Thunder's Worker Pool:**
- Shared resource across ALL Thunder plugins
- Default size: 4 threads (configurable via `threadpoolcount`)
- Processes: JSON-RPC calls, plugin operations, framework tasks

**Parallel Batch Impact:**
```
Scenario: Parallel batch with 4 requests, worker pool = 4 threads

Before batch: [Thread 1: idle] [Thread 2: idle] [Thread 3: idle] [Thread 4: idle]
After submit: [Thread 1: Job 1] [Thread 2: Job 2] [Thread 3: Job 3] [Thread 4: Job 4]

Result: Worker pool fully saturated!
Effect: Other Thunder operations must wait until batch jobs complete
```

### Best Practices

**Sequential Mode (Default):**
- ✅ Predictable resource usage
- ✅ Safe for embedded systems
- ✅ Suitable for 90% of use cases
- ✅ Multiple sequential batches can run in parallel efficiently

**Parallel Mode (Use Sparingly):**
- ⚠️ Can saturate worker pool
- ⚠️ May block other plugins temporarily
- ✅ Use for truly independent operations
- ✅ Monitor system performance

---

## Shutdown Behavior

When the plugin is deactivated or Thunder shuts down:

### Clean Shutdown Process

1. **Stop accepting new batches**: Returns `ERROR_ABORTED` (code 58)
2. **Mark active batches as aborted**: Sets internal abort flag
3. **Revoke active jobs**: Waits for running jobs to complete
4. **Delete batch resources**: Cleans up memory
5. **Release dispatcher**: Releases Controller interface

### Job Lifecycle During Shutdown

```
Job State          Action
────────────────────────────────────────────────
Not yet started    Cancelled immediately
Currently running  Completes execution, result discarded
Completed          Normal processing (if before shutdown)
```

### Example Timeline

```
T1: Deinitialize() called
T2: _shuttingDown = true
T3: All batches marked as ABORTED
T4: Revoke(job1) blocks...
    ├─ job1 completes work
    ├─ job1 calls Checkout()
    ├─ Checkout() sees _shuttingDown, skips processing
    └─ Revoke() returns
T5: Delete all batches
T6: Cleanup complete
```

**Key Point**: No jobs are forcibly terminated; they complete gracefully but results are discarded.

---

## Usage Examples

### Example 1: Plugin Lifecycle Management (Sequential)

```bash
curl -X POST http://localhost:9998/jsonrpc \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "JsonRpcMuxer.1.sequential",
    "params": [
      {
        "id": 1,
        "method": "Controller.1.activate",
        "params": {"callsign": "WebServer"}
      },
      {
        "id": 2,
        "method": "WebServer.1.status"
      },
      {
        "id": 3,
        "method": "Controller.1.deactivate",
        "params": {"callsign": "WebServer"}
      }
    ]
  }'
```

**Why sequential?** Each operation depends on the previous one completing.

### Example 2: System Status Collection (Parallel)

```bash
curl -X POST http://localhost:9998/jsonrpc \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 2,
    "method": "JsonRpcMuxer.1.parallel",
    "params": [
      { "id": 1, "method": "Controller.1.version" },
      { "id": 2, "method": "Controller.1.status" },
      { "id": 3, "method": "DeviceInfo.1.systeminfo" },
      { "id": 4, "method": "DeviceInfo.1.addresses" },
      { "id": 5, "method": "Monitor.1.status" }
    ]
  }'
```

**Why parallel?** All operations are independent read-only queries.

### Example 3: Mixed Workflow

```bash
# First batch: Sequential setup
curl -X POST http://localhost:9998/jsonrpc \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 3,
    "method": "JsonRpcMuxer.1.sequential",
    "params": [
      { "id": 1, "method": "Controller.1.activate", "params": {"callsign": "Network"} },
      { "id": 2, "method": "Network.1.configure", "params": {...} }
    ]
  }'

# Second batch: Parallel status checks (after setup completes)
curl -X POST http://localhost:9998/jsonrpc \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 4,
    "method": "JsonRpcMuxer.1.parallel",
    "params": [
      { "id": 1, "method": "Network.1.status" },
      { "id": 2, "method": "Network.1.statistics" },
      { "id": 3, "method": "Network.1.interfaces" }
    ]
  }'
```

---

## Testing

### Quick Test Script

```python
#!/usr/bin/env python3
import requests
import json

def test_sequential():
    response = requests.post(
        "http://localhost:9998/jsonrpc",
        json={
            "jsonrpc": "2.0",
            "id": 1,
            "method": "JsonRpcMuxer.1.sequential",
            "params": [
                {"id": 1, "method": "Controller.1.version"},
                {"id": 2, "method": "Controller.1.status"}
            ]
        }
    )
    print("Sequential:", json.dumps(response.json(), indent=2))

def test_parallel():
    response = requests.post(
        "http://localhost:9998/jsonrpc",
        json={
            "jsonrpc": "2.0",
            "id": 2,
            "method": "JsonRpcMuxer.1.parallel",
            "params": [
                {"id": 1, "method": "Controller.1.version"},
                {"id": 2, "method": "Controller.1.buildinfo"}
            ]
        }
    )
    print("Parallel:", json.dumps(response.json(), indent=2))

if __name__ == "__main__":
    test_sequential()
    test_parallel()
```

---

## Troubleshooting

### Issue: Parallel batches seem slow

**Cause**: Worker pool saturation

**Solutions:**
1. Check `threadpoolcount` in Thunder config
2. Reduce parallel batch sizes
3. Use sequential mode for non-critical operations
4. Increase Thunder's worker pool size

### Issue: Some requests in batch fail

**Expected Behavior**: Batch returns mixed results (some success, some errors)

**Action**: Check individual response error codes and handle accordingly

### Issue: Getting error code -31058 (ERROR_ABORTED)

**Cause**: Plugin is shutting down or being deactivated

**Action**: Retry the request after plugin restarts, or check if intentional shutdown