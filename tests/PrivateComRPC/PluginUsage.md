# Private COM-RPC QA Test Plugins

## 1. Overview

This document describes the Private COM-RPC test infrastructure implemented for Thunder NanoServices.

Two plugins were created:

| Plugin                | Purpose                                                                                                                          |
| --------------------- | -------------------------------------------------------------------------------------------------------------------------------- |
| `PrivateComRPCTest`   | Server plugin that implements the `IPrivateComRPC` interface                                                                     |
| `PrivateComRPCClient` | Client plugin that exposes a JSON-RPC API for QA and communicates with `PrivateComRPCTest` through the private COM-RPC interface |

The purpose of this implementation is to allow the existing Python QA framework to test private COM-RPC communication without requiring the Python framework to directly implement a COM-RPC client.

The communication flow is:

```text
Python QA Framework
        |
        | HTTP / JSON-RPC
        v
PrivateComRPCClient
        |
        | Private COM-RPC
        | IPrivateComRPC
        v
PrivateComRPCTest
        |
        v
ProcessLargeData()
```

The Python QA framework communicates with `PrivateComRPCClient` using the existing Thunder JSON-RPC infrastructure. The client plugin internally communicates with `PrivateComRPCTest` through the private COM-RPC interface.

---

# Part 1: QA Usage

## 2. Build and Install

Configure and install the Thunder NanoServices plugins:

```bash
cmake -S ThunderNanoServices -B build/ThunderNanoServices -DPLUGIN_PRIVATECOMRPCTEST=ON
cmake --build build/ThunderNanoServices --target install
```

Verify that both plugin libraries are installed:

```bash
ls install/lib/thunder/plugins/ | grep PrivateComRPC
```

Expected libraries:

```text
libThunderPrivateComRPCTest.so
libThunderPrivateComRPCClient.so
```

The optional in-process server test suite can be enabled with `-DPRIVATECOMRPCTEST_TESTS=ON`.

## 3. Configure and Start Thunder

The generated server configuration contains local mode and the private COM-RPC communicator:

```text
startmode = "@PLUGIN_PRIVATECOMRPCTEST_STARTMODE@"
resumed = "@PLUGIN_PRIVATECOMRPCTEST_RESUMED@"
communicator = "/tmp/privatecomrpctest"

configuration = JSON()
root = JSON()
root.add("mode", "Local")
configuration.add("root", root)
```

The client configuration contains only its startup settings:

```text
startmode = "@PLUGIN_PRIVATECOMRPCCLIENT_STARTMODE@"
resumed = "@PLUGIN_PRIVATECOMRPCCLIENT_RESUMED@"
```

The JSON-RPC HTTP endpoint is supplied by the Thunder deployment; this implementation does not configure port `55555`. The examples below assume:

```text
http://127.0.0.1:55555/jsonrpc
```

## 4. Activate the Plugins

Activate `PrivateComRPCTest` first because it provides `QualityAssurance::IPrivateComRPC`:

```bash
curl -s http://127.0.0.1:55555/jsonrpc -H 'Content-Type: application/json' -d '{"jsonrpc":"2.0","id":1,"method":"Controller.activate","params":{"callsign":"PrivateComRPCTest"}}'
```

Then activate the QA-facing client:

```bash
curl -s http://127.0.0.1:55555/jsonrpc -H 'Content-Type: application/json' -d '{"jsonrpc":"2.0","id":2,"method":"Controller.activate","params":{"callsign":"PrivateComRPCClient"}}'
```

Verify both plugins with `Controller.status` before running the test cases below.

# Part 2: Test Cases

## 5. Test Environment

The Thunder JSON-RPC endpoint used in the examples below is:

```text
http://127.0.0.1:55555/jsonrpc
```

The QA-facing JSON-RPC method is:

```text
PrivateComRPCClient.processLargeData
```

### Request Parameters

| Parameter | Type    | Description                              |
| --------- | ------- | ---------------------------------------- |
| `payload` | string  | Data sent to the Private COM-RPC server  |
| `delayMs` | uint32  | Processing delay in milliseconds; defaults to `0` |
| `echo`    | boolean | Controls whether the payload is returned; defaults to `true` |

### Response Parameters

| Parameter  | Type   | Description                     |
| ---------- | ------ | ------------------------------- |
| `response` | string | Response returned by the server |

---

## TC-01: Small Payload With Echo Enabled

### Objective

Verify that a small payload can successfully travel through the complete communication path.

```text
JSON-RPC
    |
    v
PrivateComRPCClient
    |
    | Private COM-RPC
    v
PrivateComRPCTest
```

### Command

```bash
curl -s http://127.0.0.1:55555/jsonrpc -H 'Content-Type: application/json' -d '{"jsonrpc":"2.0","id":1,"method":"PrivateComRPCClient.processLargeData","params":{"payload":"Hello Private COMRPC","delayMs":0,"echo":true}}'
```

### Expected Result

```json
{
    "jsonrpc": "2.0",
    "id": 1,
    "result": {
        "response": "Hello Private COMRPC"
    }
}
```

---

## TC-02: Empty Payload

### Objective

Verify that an empty payload is rejected by the Private COM-RPC server.

### Command

```bash
curl -s http://127.0.0.1:55555/jsonrpc -H 'Content-Type: application/json' -d '{"jsonrpc":"2.0","id":2,"method":"PrivateComRPCClient.processLargeData","params":{"payload":"","delayMs":0,"echo":true}}'
```

### Expected Result

The request returns a JSON-RPC error because `PrivateComRPCTest` returns `Core::ERROR_BAD_REQUEST` for an empty payload.

---

## TC-03: Echo Disabled

### Objective

Verify that the request reaches `PrivateComRPCTest` when echo is disabled.

### Command

```bash
curl -s http://127.0.0.1:55555/jsonrpc -H 'Content-Type: application/json' -d '{"jsonrpc":"2.0","id":3,"method":"PrivateComRPCClient.processLargeData","params":{"payload":"Echo should be disabled","delayMs":0,"echo":false}}'
```

### Expected Result

The request should complete successfully.

The response should be empty.

```json
{
    "jsonrpc": "2.0",
    "id": 3,
    "result": {
        "response": ""
    }
}
```

---

## TC-04: Delayed Processing

### Objective

Verify that Private COM-RPC communication remains functional when the server introduces a processing delay.

### Command

```bash
time curl -s http://127.0.0.1:55555/jsonrpc -H 'Content-Type: application/json' -d '{"jsonrpc":"2.0","id":4,"method":"PrivateComRPCClient.processLargeData","params":{"payload":"Delayed Private COMRPC","delayMs":1000,"echo":true}}'
```

### Expected Result

The response should be returned after approximately one second.

```json
{
    "jsonrpc": "2.0",
    "id": 4,
    "result": {
        "response": "Delayed Private COMRPC"
    }
}
```

---

## TC-05: 32 KB Payload

### Objective

Verify Private COM-RPC communication with a moderately large payload.

### Command

```bash
PAYLOAD=$(head -c 32768 /dev/zero | tr '\0' 'A'); curl -s http://127.0.0.1:55555/jsonrpc -H 'Content-Type: application/json' -d "$(printf '{"jsonrpc":"2.0","id":5,"method":"PrivateComRPCClient.processLargeData","params":{"payload":"%s","delayMs":0,"echo":true}}' "$PAYLOAD")"
```

### Expected Result

The request should complete successfully and return the same payload.

---

## TC-06: Maximum Representable Payload

### Objective

Verify the largest payload representable by the generated text serializer.

### Command

```bash
PAYLOAD=$(head -c 65535 /dev/zero | tr '\0' 'B'); curl -s http://127.0.0.1:55555/jsonrpc -H 'Content-Type: application/json' -d "$(printf '{"jsonrpc":"2.0","id":6,"method":"PrivateComRPCClient.processLargeData","params":{"payload":"%s","delayMs":0,"echo":true}}' "$PAYLOAD")"
```

### Expected Result

The 65535-byte payload is the maximum representable 16-bit length and should be recorded as a boundary result.

---

## TC-07: Payload Above 16-bit Boundary

### Objective

Verify Private COM-RPC behavior when the payload exceeds 65535 bytes.

This test is important because the generated COM-RPC proxy currently serializes the payload using a 16-bit length type.

### Command

```bash
PAYLOAD=$(head -c 65536 /dev/zero | tr '\0' 'C'); curl -s http://127.0.0.1:55555/jsonrpc -H 'Content-Type: application/json' -d "$(printf '{"jsonrpc":"2.0","id":7,"method":"PrivateComRPCClient.processLargeData","params":{"payload":"%s","delayMs":0,"echo":true}}' "$PAYLOAD")"
```

### Expected Result

The generated proxy cannot represent 65536 bytes with its default 16-bit text length. The server rejects the resulting empty payload with `Core::ERROR_BAD_REQUEST`.

---

## TC-08: Large Payload With Echo Disabled

### Objective

Verify that a large payload can be sent through Private COM-RPC without returning the same payload.

### Command

```bash
PAYLOAD=$(head -c 32768 /dev/zero | tr '\0' 'E'); curl -s http://127.0.0.1:55555/jsonrpc -H 'Content-Type: application/json' -d "$(printf '{"jsonrpc":"2.0","id":9,"method":"PrivateComRPCClient.processLargeData","params":{"payload":"%s","delayMs":0,"echo":false}}' "$PAYLOAD")"
```

### Expected Result

The request should succeed.

The response should be empty.

```json
{
    "jsonrpc": "2.0",
    "id": 9,
    "result": {
        "response": ""
    }
}
```

---

## TC-09: Combined Delay and Large Payload

### Objective

Verify Private COM-RPC behavior when both payload size and server processing delay are present.

### Command

```bash
PAYLOAD=$(head -c 32768 /dev/zero | tr '\0' 'F'); time curl -s http://127.0.0.1:55555/jsonrpc -H 'Content-Type: application/json' -d "$(printf '{"jsonrpc":"2.0","id":10,"method":"PrivateComRPCClient.processLargeData","params":{"payload":"%s","delayMs":1000,"echo":true}}' "$PAYLOAD")"
```

### Expected Result

The request should complete after approximately one second and return the payload.

---

## TC-10: Verify Private COM-RPC Server Plugin Availability

### Objective

Verify that the `PrivateComRPCTest` plugin is active before executing Private COM-RPC tests.

### Command

```bash
curl -s http://127.0.0.1:55555/jsonrpc -H 'Content-Type: application/json' -d '{"jsonrpc":"2.0","id":11,"method":"Controller.status","params":{"callsign":"PrivateComRPCTest"}}'
```

### Expected Result

The plugin should report an active or operational state.

---

## TC-11: Verify Private COM-RPC Client Plugin Availability

### Objective

Verify that the QA-facing client plugin is active.

### Command

```bash
curl -s http://127.0.0.1:55555/jsonrpc -H 'Content-Type: application/json' -d '{"jsonrpc":"2.0","id":12,"method":"Controller.status","params":{"callsign":"PrivateComRPCClient"}}'
```

### Expected Result

The plugin should report an active or operational state.

---

# Part 3: QA Reference

## 6. QA Testing Flow

The intended QA testing flow is:

```text
1. Start Thunder
2. Activate PrivateComRPCTest
3. Activate PrivateComRPCClient
4. Send PrivateComRPCClient.processLargeData from Python QA
5. The client acquires QualityAssurance::IPrivateComRPC
6. The server executes ProcessLargeData
7. The response returns through PrivateComRPCClient and JSON-RPC
```

## 7. Payload Boundary Testing

Recommended QA payload sizes are 0 bytes, a small payload, 32 KB, 65535 bytes, and 65536 bytes. Boundary tests are important because the generated proxy serializes text lengths using a 16-bit type.

## 8. QA Automation

The Python QA framework only invokes `PrivateComRPCClient.processLargeData` through Thunder JSON-RPC. It does not create a COM-RPC communicator, connect directly to the server, generate proxies, handle serialization, or manage interface lifetimes.

# Part 4: Plugin Implementation

`PrivateComRPCTest` implements `QualityAssurance::IPrivateComRPC`. `PrivateComRPCClient` exposes `processLargeData` through JSON-RPC, obtains the server interface by callsign, and invokes `ProcessLargeData` over private COM-RPC.

## PrivateComRPCTest

The server implements:

```cpp
virtual Core::hresult ProcessLargeData(
    const string& payload,
    const uint32_t delayMs,
    const bool echo,
    /* @out */ string& response) = 0;
```
The server rejects an empty payload, optionally delays processing, and returns the payload only when `echo` is enabled. It also registers a direct JSON-RPC method, but the QA tests use the client plugin to exercise COM-RPC.

## PrivateComRPCClient

The client exposes:

```text
PrivateComRPCClient.processLargeData
```

It acquires `QualityAssurance::IPrivateComRPC` using the `PrivateComRPCTest` callsign, invokes `ProcessLargeData`, and returns the response through JSON-RPC.

## JSON Request

Example request:

```json
{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "PrivateComRPCClient.processLargeData",
    "params": {
        "payload": "Hello Private COMRPC",
        "delayMs": 0,
        "echo": true
    }
}
```

---

## JSON Response

Example response:

```json
{
    "jsonrpc": "2.0",
    "id": 1,
    "result": {
        "response": "Hello Private COMRPC"
    }
}
```

---

## Plugin Configuration

Private COM-RPC is configured through the plugin configuration.

The relevant configuration uses:

```cpp
cfg.Root.Mode = Plugin::Config::RootConfig::ModeType::LOCAL;
```

The server communicator endpoint is `/tmp/privatecomrpctest`. The CMake cache variable `PLUGIN_PRIVATECOMRPCTEST_COMMUNICATOR` is exposed for configuration but is not substituted into the current template.

---

## Plugin Files

## PrivateComRPCTestPlugin

```text
PrivateComRPCTestPlugin/
├── CMakeLists.txt
├── Module.cpp
├── Module.h
├── PrivateComRPCTest.cpp
├── PrivateComRPCTest.h
└── PrivateComRPCTest.conf.in
```

## PrivateComRPCClient

```text
PrivateComRPCClient/
├── CMakeLists.txt
├── Module.cpp
├── Module.h
├── PrivateComRPCClient.cpp
├── PrivateComRPCClient.h
└── PrivateComRPCClient.conf.in
```

---

## Module Files

Both plugins use the standard Thunder module declaration:

```cpp
#include "Module.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE)
```

```cpp
#pragma once

#ifndef MODULE_NAME
#define MODULE_NAME Plugin_PrivateComRPCTest
#endif

#include <plugins/plugins.h>

#undef EXTERNAL
#define EXTERNAL
```

The client uses:

```cpp
#define MODULE_NAME Plugin_PrivateComRPCClient
```

## Build

Build and install the plugins using:

```bash
cmake -S ThunderNanoServices -B build/ThunderNanoServices -DPLUGIN_PRIVATECOMRPCTEST=ON
cmake --build build/ThunderNanoServices --target install
```

Verify that both plugin libraries are installed:

```bash
ls install/lib/thunder/plugins/ | grep PrivateComRPC
```

Expected libraries include:

```text
libThunderPrivateComRPCTest.so
libThunderPrivateComRPCClient.so
```

---

## Summary

```text
Python QA Framework
        |
        | JSON-RPC
        v
PrivateComRPCClient
        |
        | Private COM-RPC
        | IPrivateComRPC
        v
PrivateComRPCTest
```
