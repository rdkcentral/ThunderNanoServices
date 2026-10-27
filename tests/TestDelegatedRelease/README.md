# TestDelegatedRelease QA Test Guide

This test verifies that Thunder releases COM-RPC references that belong to an
out-of-process plugin channel when that channel is closed.

## What Is Tested

`TestDelegatedRelease` exposes these QA interfaces:

- `ITestDelegatedRelease::Ping()` checks COM-RPC connectivity and returns
  `0x12345678`.
- `ITestDelegatedRelease::HoldPeer()` sends an
  `ITestDelegatedReleasePeer` implementation to the plugin. The plugin keeps
  the peer reference until the channel is closed.

The test passes when the peer remains alive before channel closure and is
destroyed after the plugin is deactivated. JSON-RPC is used only to control
the plugin lifecycle; `Ping()` and `HoldPeer()` are COM-RPC calls.

## Prerequisites

- Build and install ThunderInterfaces so the QA proxy/stub code is available.
- Build with Ninja, GTest, and `thunder_test_support` available.
- Run the plugin out of process. The plugin root mode must be `Local`.

Do not use in-process mode for this test. Delegated release is exercised when
the OOP COM-RPC channel closes.

## Build

From the workspace root, configure NanoServices with the test enabled:

```bash
cmake -G Ninja -S ThunderNanoServices -B build/ThunderNanoServices \
    -DCMAKE_INSTALL_PREFIX="$PWD/install" \
    -DPLUGIN_TESTDELEGATEDRELEASE=ON \
    -DTESTDELEGATEDRELEASE_TESTS=ON

cmake --build build/ThunderNanoServices --target install
```

The build must produce:

- `libThunderTestDelegatedRelease.so`
- `TestDelegatedReleaseIntegrationTests`
- the generated QA proxy/stub library
- `TestDelegatedRelease.json` with the plugin root mode set to `Local`

If CMake reports that GTest or `thunder_test_support` is unavailable, the
integration test target is skipped. Install or build those dependencies and
reconfigure before testing.

## Run The Supported Integration Test

Run the complete delegated-release suite:

```bash
ctest --test-dir build/ThunderNanoServices \
    -R TestDelegatedReleaseIntegrationTests \
    --output-on-failure
```

Run the test executable directly when a single scenario is needed:

```bash
build/ThunderNanoServices/tests/TestDelegatedRelease/TestDelegatedReleaseIntegrationTests \
    --gtest_filter=DelegatedReleaseTestSuite.ClosingChannelReleasesRetainedPeer
```

The exact executable path can vary with the CMake build layout. Locate it with:

```bash
find build/ThunderNanoServices -type f \
    -name TestDelegatedReleaseIntegrationTests -executable
```

The test runtime configures the plugin as `Local`, starts it out of process,
and supplies the generated proxy/stub directory through `LD_LIBRARY_PATH`.
No manually started Thunder daemon or Python test script is required for this
repository integration test.

## Scenarios

The suite contains these checks:

| Test | What it verifies |
| --- | --- |
| `PingOverCOMRPC` | The plugin can be reached over COM-RPC and returns `0x12345678`. |
| `ClosingChannelReleasesRetainedPeer` | Deactivating one plugin releases one retained peer. |
| `ClosingChannelReleasesMultiplePeerReferences` | Deactivating one plugin releases all retained peers on its channel. |
| `ClosingOneChannelDoesNotReleaseAnotherChannelsPeer` | Closing one channel does not release a peer belonging to another channel. |

Run the full suite directly with:

```bash
<path-to-TestDelegatedReleaseIntegrationTests>
```

## Expected Results

For a passing run:

- `PingOverCOMRPC` returns `Core::ERROR_NONE` and `0x12345678`.
- Each peer is still alive immediately after the client releases its own
  reference.
- Deactivating the plugin causes the retained peer or peers to be destroyed.
- Deactivating the first plugin instance does not destroy the peer held by the
  second instance.
- CTest reports `100% tests passed` for the selected test.

## Troubleshooting

- **The test target is missing:** GTest or `thunder_test_support` was not found
  during CMake configuration.
- **The plugin cannot load:** check the installed plugin, proxy/stub library,
  and `LD_LIBRARY_PATH`.
- **The peer is not released:** verify the plugin is configured with `Local`
  mode and that the test deactivates the plugin after `HoldPeer()` succeeds.
- **Only JSON-RPC is being used:** JSON-RPC cannot transport the peer interface;
  use the generated COM-RPC proxy/stub code.
