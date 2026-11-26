# QA Security Agent Guide

## Quick Start

This component provides two Thunder plugins:

- `QASecurityAgent`: validates QA authorization tokens.
- `QASecurityTarget`: provides protected HTTP and JSON-RPC endpoints.

Build with the QA security component enabled:

```bash
cmake -G Ninja -S ThunderNanoServices -B build/ThunderNanoServices \
  -DCMAKE_INSTALL_PREFIX="$PWD/install" \
  -DPLUGIN_QASECURITY=ON
cmake --build build/ThunderNanoServices --target install -j"$(nproc)"
```

Integration tests also require Thunder to be built and installed with
`-DENABLE_TEST_RUNTIME=ON`, plus GTest:

```bash
ctest --test-dir build/ThunderNanoServices --output-on-failure \
  -R QASecurityAgentIntegrationTests
```

## Run The Daemon

Start Thunder:

```bash
./install/bin/Thunder -f -c install/etc/Thunder/config.json
```

QASecurityAgent must start in the `Activated` start mode. Thunder reads its
metadata, sees that it controls the `SECURITY` subsystem, and enforces security
from startup. While the agent is `Deactivated` there is no officer to accept a
token, so every request, including `Controller.1.activate`, returns HTTP `401`
and the agent cannot be activated over JSON-RPC.

Set the start mode before starting Thunder, using either:

- `"startmode":"Activated"` in `install/etc/Thunder/plugins/QASecurityAgent.json`, or
- `-DPLUGIN_QASECURITYAGENT_STARTMODE=Activated` when configuring the build.

The startup log shows `Activation of plugin [QASecurityAgent] delayed, start
mode is Deactivated` when the start mode is wrong.

Before sending protected requests, ensure QASecurityAgent is active and has
registered the Security subsystem. The expected log lines are:

```text
Activated plugin [QASecurityAgent]:[QASecurityAgent]
EVENT: Security
QASecurityAgent: Security subsystem registered after activation
```

Activate QASecurityTarget:

```bash
curl -i -X POST http://127.0.0.1:55555/jsonrpc \
  -H 'Content-Type: application/json' \
  -H 'Authorization: Bearer ALL_ALLOW' \
  --data '{"jsonrpc":"2.0","id":1,"method":"Controller.1.activate","params":{"callsign":"QASecurityTarget"}}'
```

Expected: HTTP `200` and a JSON-RPC `result` of `null`.

## Token Scenarios

Supply the token as `Authorization: Bearer <TOKEN>`.

| Token | Plain HTTP | JSON-RPC over HTTP | Controller JSON-RPC | QA use |
|---|---|---|---|---|
| `PATH_ALLOW` | Denied | Denied | Denied | Path-policy unit coverage only. |
| `HTTP_ALLOW` | Allowed | Denied | Denied | Plain HTTP authorization only. |
| `JSONRPC_ALLOW` | Allowed | Allowed | Allowed | Positive JSON-RPC authorization. |
| `ALL_ALLOW` | Allowed | Allowed | Allowed | Positive case for every implemented endpoint. |
| `DENY` | Denied | Denied | Denied | Negative baseline. |

HTTP and JSON-RPC requests read the bearer token from the `Authorization`
header.

`PATH_ALLOW` maps to `ISecurity::Allowed(const string& path)`. Thunder applies
it to the WebSocket upgrade on `/Service/QASecurityTarget`; QASecurityTarget
needs no WebSocket channel handler of its own for the handshake. See
[WebSocket](#websocket).

### Plain HTTP

Run each command against `GET /Service/QASecurityTarget/http`:

```bash
curl -i http://127.0.0.1:55555/Service/QASecurityTarget/http -H 'Authorization: Bearer PATH_ALLOW'
curl -i http://127.0.0.1:55555/Service/QASecurityTarget/http -H 'Authorization: Bearer HTTP_ALLOW'
curl -i http://127.0.0.1:55555/Service/QASecurityTarget/http -H 'Authorization: Bearer JSONRPC_ALLOW'
curl -i http://127.0.0.1:55555/Service/QASecurityTarget/http -H 'Authorization: Bearer ALL_ALLOW'
curl -i http://127.0.0.1:55555/Service/QASecurityTarget/http -H 'Authorization: Bearer DENY'
```

Expected: `HTTP_ALLOW`, `JSONRPC_ALLOW`, and `ALL_ALLOW` return HTTP `200`.
`PATH_ALLOW` and `DENY` return HTTP `401`.

### JSON-RPC

Run each command against `QASecurityTarget.1.echo`. Thunder checks both HTTP
and JSON-RPC permission for a request sent to `/jsonrpc`.

```bash
curl -i -X POST http://127.0.0.1:55555/jsonrpc -H 'Content-Type: application/json' -H 'Authorization: Bearer PATH_ALLOW' --data '{"jsonrpc":"2.0","id":1,"method":"QASecurityTarget.1.echo","params":{}}'
curl -i -X POST http://127.0.0.1:55555/jsonrpc -H 'Content-Type: application/json' -H 'Authorization: Bearer HTTP_ALLOW' --data '{"jsonrpc":"2.0","id":2,"method":"QASecurityTarget.1.echo","params":{}}'
curl -i -X POST http://127.0.0.1:55555/jsonrpc -H 'Content-Type: application/json' -H 'Authorization: Bearer JSONRPC_ALLOW' --data '{"jsonrpc":"2.0","id":3,"method":"QASecurityTarget.1.echo","params":{}}'
curl -i -X POST http://127.0.0.1:55555/jsonrpc -H 'Content-Type: application/json' -H 'Authorization: Bearer ALL_ALLOW' --data '{"jsonrpc":"2.0","id":4,"method":"QASecurityTarget.1.echo","params":{}}'
curl -i -X POST http://127.0.0.1:55555/jsonrpc -H 'Content-Type: application/json' -H 'Authorization: Bearer DENY' --data '{"jsonrpc":"2.0","id":5,"method":"QASecurityTarget.1.echo","params":{}}'
```

Expected: `JSONRPC_ALLOW` and `ALL_ALLOW` return:

```text
HTTP/1.1 200 JSONRPC executed succesfully
{"jsonrpc":"2.0","id":1,"result":null}
```

`PATH_ALLOW`, `HTTP_ALLOW`, and `DENY` return HTTP `401`.

### WebSocket

The token is passed as the `token` query parameter. Curl can show the handshake
result but cannot send WebSocket frames; `-m 3` stops it after the upgrade.

```bash
curl -i -m 3 --http1.1 -N \
  -H 'Connection: Upgrade' -H 'Upgrade: websocket' \
  -H 'Sec-WebSocket-Version: 13' -H 'Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==' \
  'http://127.0.0.1:55555/Service/QASecurityTarget?token=PATH_ALLOW'
```

Expected: `PATH_ALLOW` and `ALL_ALLOW` return `101 SWITCH_PROTOCOL`. Curl then
times out with `(28)`, which is expected. `DENY` returns HTTP `403` with
`Security prohibites this connection.` Replace `PATH_ALLOW` in the URL to try
other tokens.

## Token Creation

`IAuthenticate::CreateToken()` maps these byte payloads to the same token
strings:

```text
PATH_ALLOW
HTTP_ALLOW
JSONRPC_ALLOW
ALL_ALLOW
```

Any other payload, an empty payload, or a null buffer produces `DENY`.
`Officer()` accepts the five documented token strings. An unknown token, such
as `INVALID_TOKEN`, is rejected and returns no security context.

## Implementation Reference

QASecurityAgent implements `PluginHost::IPlugin` and
`PluginHost::IAuthenticate`. Its metadata declares ownership of the Thunder
`SECURITY` subsystem. Once activated, it publishes an
`ISubSystem::ISecurity` object with its callsign. Thunder then calls `Officer()`
to turn a bearer token into an `ISecurity` policy context.

QASecurityTarget implements `PluginHost::IPlugin`, `PluginHost::JSONRPC`, and
`PluginHost::IWeb`. It declares `SECURITY` as a precondition and exposes:

```text
QASecurityTarget.1.echo
GET /Service/QASecurityTarget/http
```

The policy dimensions correspond to Thunder's three `ISecurity` calls:

```cpp
Allowed(const string& path)
Allowed(const Web::Request& request)
Allowed(const Core::JSONRPC::Message& message)
```

## Relevant Files

- [QASecurityAgent.cpp](QASecurityAgent/QASecurityAgent.cpp)
- [QASecurityTarget.cpp](QASecurityTarget/QASecurityTarget.cpp)
- [SecurityAgentTestSuite.cpp](tests/SecurityAgentTestSuite.cpp)