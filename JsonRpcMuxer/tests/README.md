# JsonRpcMuxer Test Suite

Test scripts for the Thunder JsonRpcMuxer plugin, covering functional correctness, stress testing, and extended soak testing.

## Overview

The JsonRpcMuxer plugin enables batching multiple JSON-RPC requests into a single call. These tests verify:
- Batch processing correctness
- Size and concurrency limits
- Error handling
- System stability under load

## Prerequisites

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install python3-websocket python3-requests

# Or with pip
pip3 install websocket-client requests
```

## Test Scripts

### 1. Functional Tests (`test_jsonrpcmuxer.py`)

**Purpose:** Verify correct behavior and error handling

**Duration:** ~30 seconds

**Tests:**
- HTTP batch processing (single, maximum size, oversized, empty)
- Concurrent batch handling
- WebSocket batch processing
- WebSocket connection limits
- Error responses for invalid inputs

**Usage:**
```bash
python3 test_jsonrpcmuxer.py

# With custom configuration
python3 test_jsonrpcmuxer.py --host 192.168.1.100 --port 9998 --max-batch-size 10 --max-batches 5
```

**Expected output:** All tests pass

---

### 2. Stress Tests (`stress_test_jsonrpcmuxer.py`)

**Purpose:** Test behavior under heavy concurrent load

**Duration:** ~20 seconds

**Tests:**
- **Maxbatches limit:** Verifies concurrent batch limit enforcement using slow TimeSync.synchronize requests
- **Rapid fire:** 20 batches sent simultaneously
- **Sustained load:** Continuous batches over 5 seconds

**Usage:**
```bash
python3 stress_test_jsonrpcmuxer.py

# Custom configuration
python3 stress_test_jsonrpcmuxer.py --max-batches 5 --timeout 10000
```

**Note:** Uses `TimeSync.1.synchronize` for slow requests (~100-500ms each) to properly test concurrency limits. If TimeSync is not available, some tests may show all batches accepted.

---

### 3. Controller Stress Tests (`controller_stress_test.py`)

**Purpose:** Stress testing with fast Controller methods only

**Duration:** ~1 minute (without soak test)

**Tests:**
- **High concurrency:** 20 batches sent at once
- **Burst waves:** 3 waves of 10 batches each
- **Soak test (optional):** Extended duration testing

**Usage:**
```bash
# Quick tests only
python3 controller_stress_test.py

# With 5-minute soak test
python3 controller_stress_test.py --soak 5

# With 30-minute soak test
python3 controller_stress_test.py --soak 30

# Full options
python3 controller_stress_test.py --host localhost --port 55555 --max-batches 5 --soak 10
```

**Soak test features:**
- Sends batches continuously for specified duration
- Reports progress every 30 seconds
- Press Ctrl+C to stop early
- Shows acceptance rate and error rate

---

## Common Parameters

All test scripts support these parameters:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `--host` | localhost | Thunder host |
| `--port` | 55555 | Thunder port |
| `--timeout` | 5000 | Request timeout (ms) |
| `--max-batch-size` | 10 | Expected max batch size |
| `--max-batches` | 5 | Expected max concurrent batches |
| `--soak` | 0 | Soak test duration in minutes (controller_stress_test.py only) |

**Note:** The `--max-batch-size` and `--max-batches` parameters should match your JsonRpcMuxer configuration.

---

## Configuration

The tests expect JsonRpcMuxer to be configured with these defaults:

```json
{
  "timeout": 5000,
  "maxbatchsize": 10,
  "maxbatches": 5
}
```

If your configuration differs, pass the corresponding parameters to the test scripts.

---

## Understanding Test Results

### Functional Tests

```
✓ HTTP: Single batch (4 requests)
✓ HTTP: Maximum batch size (10 requests)
✓ HTTP: Exceed batch size limit (should reject)
...
Total: 9, Passed: 9, Failed: 0
```

All 9 tests should pass. Any failures indicate incorrect behavior.

### Stress Tests

```
[TEST] Maxbatches limit: 8 concurrent batches
ℹ Accepted: 5, Rejected: 3, Errors: 0
✓ System correctly handled overload
```

With slow requests (TimeSync), you should see rejections when exceeding `maxbatches`. With fast Controller methods, all batches may be accepted if they complete too quickly - this is expected and indicates good performance.

### Soak Tests

```
[TEST] Soak test: 5 minutes at 2.0 batches/sec
ℹ   2.5min: sent=300, accepted=298, rejected=2, errors=0
ℹ Accept rate: 99.3%, Error rate: 0.0%
✓ Soak test completed successfully
```

- **Accept rate >90%:** Good
- **Error rate <10%:** Good
- **Higher reject rate:** System is correctly enforcing limits

---

## Troubleshooting

**"Connection refused"**
- Verify Thunder is running: `curl http://localhost:55555/jsonrpc`
- Check port number matches

**"Module 'websocket' has no attribute 'create_connection'"**
- Wrong package installed. Need: `python3-websocket` or `pip3 install websocket-client`

**All batches accepted in stress test**
- System is fast and batches complete quickly (good!)
- Try with TimeSync plugin: `stress_test_jsonrpcmuxer.py` uses slower requests
- Or increase concurrent batches: `--max-batches 3` and send more batches

**TimeSync not available**
- Install TimeSync plugin or use `controller_stress_test.py` instead

**High error rates in soak test**
- Check Thunder logs for issues
- Verify system resources (CPU, memory)
- May indicate memory leak or resource exhaustion

---

## Test Development Notes

### Why Different Test Scripts?

1. **Functional tests:** Fast, deterministic, verify correctness
2. **Stress with TimeSync:** Properly tests concurrency limits with slow requests
3. **Controller stress:** Tests with always-available fast methods, good for quick checks

### Race Condition Testing

The `maxbatches` limit uses atomic `fetch_add()` to prevent race conditions. The stress tests verify this works correctly by sending many batches simultaneously.

With fast Controller methods, batches may complete before the limit is reached. This is why `stress_test_jsonrpcmuxer.py` uses `TimeSync.1.synchronize` - it's slow enough (~100-500ms) to properly test concurrent batch limits.

---

## Example Test Session

```bash
# 1. Quick functional check
python3 test_jsonrpcmuxer.py
# Expected: All 9 tests pass in ~30s

# 2. Stress test with proper concurrency testing
python3 stress_test_jsonrpcmuxer.py
# Expected: Some rejections when limit exceeded

# 3. Extended stability test
python3 controller_stress_test.py --soak 10
# Expected: 10 minutes of stable operation, <10% errors

# All passed? JsonRpcMuxer is working correctly!
```
