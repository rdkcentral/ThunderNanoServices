# SlimOOP

An idle out-of-process implementation baseline for memory measurement. The
in-process wrapper implements only `PluginHost::IPlugin`, Thunder's required
lifecycle interface. Its child implementation exposes only `Core::IUnknown`.
`PluginHost::IShell` is supplied by the host, not implemented by the plugin.

There are no JSON-RPC methods, custom COM-RPC interfaces, notifications,
application worker threads, or implementation-owned state. The wrapper retains
only the remote interface pointer and connection ID. Thunder uses its built-in
COM-RPC infrastructure to manage the remote implementation's lifecycle.

## Build And Run

Using an existing Thunder SDK build environment, configure NanoServices with
`-DPLUGIN_SLIMOOP=ON`, then build the `ThunderSlimOOP` target (or the matching
target for your SDK namespace) and install it using the usual deployment flow.
The plugin starts activated by default; override this with
`-DPLUGIN_SLIMOOP_STARTMODE=Deactivated` when needed.

Keep `-DPLUGIN_SLIMOOP_MODE=Local` (the default). Initialization calls
`IShell::Root<Core::IUnknown>()` to create `SlimOOPImplementation` and fails if
the implementation cannot be created or is configured to run in process.
Deinitialization releases the object and terminates its remote connection.
The generated config uses a nested root, not a top-level root:

```json
{
    "configuration": {
        "root": {
            "mode": "Local"
        }
    }
}
```

This is host configuration, not a JSON-RPC API. Both classes are built into the
same library; no separate implementation or application proxy/stub library is
needed. No crash-notification sink is registered, keeping the wrapper minimal.

## Measure

After activation, identify SlimOOP's ThunderPlugin child PID and wait for startup
to settle before sampling. On Linux, read `/proc/<pid>/smaps_rollup` and compare
PSS and private memory as well as RSS; RSS includes shared library mappings.
The result includes the unavoidable ThunderPlugin host, built-in IPC, runtime
libraries, and thread stacks, not just the implementation object. Thunder's
default single RPC worker does not imply that the entire process has only one
thread. Measure the main Thunder process separately for the wrapper's cost.