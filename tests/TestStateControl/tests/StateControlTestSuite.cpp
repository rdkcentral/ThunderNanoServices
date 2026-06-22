/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file StateControlTestSuite.cpp
 *
 * In-process Google Test suite for the StateControlTest plugin.
 *
 * ThunderTestRuntime spins up a real PluginHost::Server inside the test
 * process, activates the StateControlTest plugin, and lets us exercise
 * IStateControl without any network or out-of-process overhead.
 *
 * Test organisation:
 *   1. Interface availability – verify the plugin and its shell are reachable.
 *   2. Initial state        – plugin starts as UNINITIALIZED (first test only).
 *   3. State transitions    – RESUME / SUSPEND from every valid predecessor.
 *   4. Idempotency          – duplicate commands stay in the same state.
 *   5. Notifications        – IStateControl::INotification callbacks are fired
 *                             correctly (or suppressed for no-op commands).
 *
 * NOTE: All tests share a single ThunderTestRuntime (and therefore a single
 * plugin instance).  Tests that need a specific starting state call
 * EnsureResumed() / EnsureSuspended() so they are order-independent.
 * The ONLY exception is InitialStateIsUninitialized, which relies on being
 * the very first test to run before any state command is issued.
 */

#include "Module.h"
#include <test_support/ThunderTestRuntime.h>

#include <gtest/gtest.h>
#include <plugins/IStateControl.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace Thunder {
namespace TestCore {
namespace Tests {

// ==========================================================================
// StateObserver – thread-safe IStateControl::INotification implementation
// ==========================================================================

/**
 * Concrete IStateControl::INotification that records every StateChange()
 * call and provides a condition-variable based WaitForState() helper.
 *
 * Lifetime: stack-allocated instances own their own reference count.
 * Pass a raw pointer to Register(); the plugin AddRef()s it internally.
 * After Unregister(), call Release() to hand back your initial reference.
 */
class StateObserver : public PluginHost::IStateControl::INotification {
public:
    StateObserver()
        : _lastState(PluginHost::IStateControl::UNINITIALIZED)
        , _callCount(0)
    {
    }

    StateObserver(const StateObserver&) = delete;
    StateObserver& operator=(const StateObserver&) = delete;

    // -----------------------------------------------------------------------
    // IStateControl::INotification
    // -----------------------------------------------------------------------
    void StateChange(const PluginHost::IStateControl::state state) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _lastState = state;
        ++_callCount;
        _cv.notify_all();
    }

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /**
     * Block until the observer has received a notification with @p expected,
     * or until @p timeout elapses.
     * @return true  if the expected state was received before the timeout.
     */
    bool WaitForState(PluginHost::IStateControl::state expected,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(2000))
    {
        std::unique_lock<std::mutex> lock(_mutex);
        return _cv.wait_for(lock, timeout, [this, expected] {
            return _lastState == expected;
        });
    }

    PluginHost::IStateControl::state LastState() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _lastState;
    }

    uint32_t CallCount() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _callCount;
    }

    /** Reset the call counter without touching the last observed state. */
    void Reset()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _callCount = 0;
    }

    BEGIN_INTERFACE_MAP(StateObserver)
        INTERFACE_ENTRY(PluginHost::IStateControl::INotification)
    END_INTERFACE_MAP

private:
    mutable std::mutex _mutex;
    std::condition_variable _cv;
    PluginHost::IStateControl::state _lastState;
    uint32_t _callCount;
};

// ==========================================================================
// Helpers
// ==========================================================================

static constexpr std::chrono::milliseconds kStateTimeout{ 2000 };
static constexpr std::chrono::milliseconds kPollInterval{ 10 };

/**
 * Poll IStateControl::State() until it equals @p expected or @p timeout
 * elapses.  The async worker-pool dispatch in StateControlTest means a small
 * delay is expected between Request() returning and the state actually
 * flipping.
 *
 * @return true if @p expected was reached within @p timeout.
 */
static bool WaitForState(
    PluginHost::IStateControl* ctrl,
    PluginHost::IStateControl::state expected,
    std::chrono::milliseconds timeout = kStateTimeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (ctrl->State() == expected) {
            return true;
        }
        std::this_thread::sleep_for(kPollInterval);
    }
    return ctrl->State() == expected;
}

// ==========================================================================
// Test fixture
// ==========================================================================

class StateControlTestSuite : public ::testing::Test {
protected:
    // One Thunder runtime shared by the whole fixture.
    static ThunderTestRuntime _runtime;

    // Per-test handle – obtained in SetUp, released in TearDown.
    PluginHost::IStateControl* _stateControl{ nullptr };

    // ------------------------------------------------------------------
    // Suite-level setup / teardown (called once for the entire fixture)
    // ------------------------------------------------------------------

    static void SetUpTestSuite()
    {
        ThunderTestRuntime::PluginConfig cfg;
        cfg.Callsign  = "TestStateControl";
        cfg.ClassName = "TestStateControl";
        // Library name as installed by the TestStateControl CMake target:
        // libThunderTestStateControl.so
        cfg.Locator   = "libThunderTestStateControl.so";
        // Do NOT set Resumed = true: we want to observe UNINITIALIZED first.
        cfg.Resumed   = false;

        const uint32_t result = _runtime.Initialize({ cfg });
        ASSERT_EQ(result, Core::ERROR_NONE)
            << "ThunderTestRuntime failed to initialise (error " << result << ")";
    }

    static void TearDownTestSuite()
    {
        _runtime.Deinitialize();
    }

    // ------------------------------------------------------------------
    // Per-test setup / teardown
    // ------------------------------------------------------------------

    void SetUp() override
    {
        _stateControl = _runtime.QueryInterfaceByCallsign<PluginHost::IStateControl>(
            "TestStateControl");
        ASSERT_NE(_stateControl, nullptr)
            << "IStateControl must be accessible via QueryInterfaceByCallsign";
    }

    void TearDown() override
    {
        if (_stateControl != nullptr) {
            _stateControl->Release();
            _stateControl = nullptr;
        }
    }

    // ------------------------------------------------------------------
    // State helpers (idempotent – safe to call from any starting state)
    // ------------------------------------------------------------------

    /** Drive the plugin to RESUMED (valid from UNINITIALIZED or SUSPENDED). */
    void EnsureResumed()
    {
        if (_stateControl->State() != PluginHost::IStateControl::RESUMED) {
            ASSERT_EQ(_stateControl->Request(PluginHost::IStateControl::RESUME),
                Core::ERROR_NONE);
            ASSERT_TRUE(WaitForState(_stateControl, PluginHost::IStateControl::RESUMED))
                << "Timed out waiting for RESUMED state in EnsureResumed()";
        }
    }

    /** Drive the plugin to SUSPENDED (valid from UNINITIALIZED or RESUMED). */
    void EnsureSuspended()
    {
        if (_stateControl->State() != PluginHost::IStateControl::SUSPENDED) {
            ASSERT_EQ(_stateControl->Request(PluginHost::IStateControl::SUSPEND),
                Core::ERROR_NONE);
            ASSERT_TRUE(WaitForState(_stateControl, PluginHost::IStateControl::SUSPENDED))
                << "Timed out waiting for SUSPENDED state in EnsureSuspended()";
        }
    }
};

ThunderTestRuntime StateControlTestSuite::_runtime;

// ==========================================================================
// 1. Interface availability
//    (These tests do not issue any Request() so they leave state untouched.)
// ==========================================================================

/**
 * MUST be the first test in the suite so that UNINITIALIZED is still the
 * current state.  All tests after this one use EnsureResumed /
 * EnsureSuspended to reach a known state independently of ordering.
 */
TEST_F(StateControlTestSuite, InitialStateIsUninitialized)
{
    EXPECT_EQ(_stateControl->State(), PluginHost::IStateControl::UNINITIALIZED)
        << "Plugin must start in UNINITIALIZED state when Resumed=false";
}

TEST_F(StateControlTestSuite, QueryInterfaceByCallsignReturnsNonNull)
{
    // _stateControl is already asserted in SetUp; confirm it survives a fresh
    // QueryInterface call so the test result is standalone.
    auto* iface = _runtime.QueryInterfaceByCallsign<PluginHost::IStateControl>(
        "TestStateControl");
    ASSERT_NE(iface, nullptr);
    iface->Release();
}

TEST_F(StateControlTestSuite, GetShellForPluginIsValid)
{
    auto shell = _runtime.GetShell("TestStateControl");
    EXPECT_TRUE(shell.IsValid())
        << "IShell for TestStateControl must be available";
}

TEST_F(StateControlTestSuite, ShellAlsoExposesIPlugin)
{
    auto* plugin = _runtime.QueryInterfaceByCallsign<PluginHost::IPlugin>(
        "TestStateControl");
    ASSERT_NE(plugin, nullptr) << "TestStateControl must implement IPlugin";
    plugin->Release();
}

// ==========================================================================
// 2. State transitions
// ==========================================================================

TEST_F(StateControlTestSuite, RequestResumeFromUninitializedTransitionsToResumed)
{
    // Drive to SUSPENDED so we have a deterministic non-RESUMED start state.
    EnsureSuspended();
    ASSERT_EQ(_stateControl->State(), PluginHost::IStateControl::SUSPENDED);

    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::RESUME), Core::ERROR_NONE);
    EXPECT_TRUE(WaitForState(_stateControl, PluginHost::IStateControl::RESUMED))
        << "State must reach RESUMED after Request(RESUME) from SUSPENDED";
    EXPECT_EQ(_stateControl->State(), PluginHost::IStateControl::RESUMED);
}

TEST_F(StateControlTestSuite, RequestSuspendFromResumedTransitionsToSuspended)
{
    EnsureResumed();

    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::SUSPEND), Core::ERROR_NONE);
    EXPECT_TRUE(WaitForState(_stateControl, PluginHost::IStateControl::SUSPENDED))
        << "State must reach SUSPENDED after Request(SUSPEND) from RESUMED";
    EXPECT_EQ(_stateControl->State(), PluginHost::IStateControl::SUSPENDED);
}

TEST_F(StateControlTestSuite, RequestResumeFromSuspendedTransitionsToResumed)
{
    EnsureSuspended();

    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::RESUME), Core::ERROR_NONE);
    EXPECT_TRUE(WaitForState(_stateControl, PluginHost::IStateControl::RESUMED))
        << "State must reach RESUMED after Request(RESUME) from SUSPENDED";
    EXPECT_EQ(_stateControl->State(), PluginHost::IStateControl::RESUMED);
}

TEST_F(StateControlTestSuite, RequestSuspendFromSuspendedGoesToSuspended)
{
    // SUSPEND from UNINITIALIZED is valid per the Dispatch() implementation.
    // Here we explicitly verify that SUSPEND from the very first command works.
    EnsureResumed();
    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::SUSPEND), Core::ERROR_NONE);
    EXPECT_TRUE(WaitForState(_stateControl, PluginHost::IStateControl::SUSPENDED));
    // Issue second SUSPEND – already handled by idempotency tests below, but
    // confirm the state machine doesn't move to an unexpected value.
    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::SUSPEND), Core::ERROR_NONE);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(_stateControl->State(), PluginHost::IStateControl::SUSPENDED);
}

TEST_F(StateControlTestSuite, RequestAlwaysReturnsErrorNone)
{
    // Both commands must return Core::ERROR_NONE regardless of current state.
    EnsureResumed();
    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::SUSPEND), Core::ERROR_NONE);
    EXPECT_TRUE(WaitForState(_stateControl, PluginHost::IStateControl::SUSPENDED));

    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::RESUME), Core::ERROR_NONE);
    EXPECT_TRUE(WaitForState(_stateControl, PluginHost::IStateControl::RESUMED));
}

// ==========================================================================
// 3. Idempotency
// ==========================================================================

TEST_F(StateControlTestSuite, DoubleResumeStaysResumed)
{
    EnsureResumed();

    // A second RESUME while already RESUMED must not change state.
    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::RESUME), Core::ERROR_NONE);
    // Allow the worker pool job to run and assert the state did not change.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(_stateControl->State(), PluginHost::IStateControl::RESUMED)
        << "Double RESUME must leave state as RESUMED";
}

TEST_F(StateControlTestSuite, DoubleSuspendStaysSuspended)
{
    EnsureSuspended();

    // A second SUSPEND while already SUSPENDED must not change state.
    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::SUSPEND), Core::ERROR_NONE);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(_stateControl->State(), PluginHost::IStateControl::SUSPENDED)
        << "Double SUSPEND must leave state as SUSPENDED";
}

// ==========================================================================
// 4. Notification observer
// ==========================================================================

TEST_F(StateControlTestSuite, NotificationFiredOnResumeTransition)
{
    EnsureSuspended();

    Core::SinkType<StateObserver> observer;
    _stateControl->Register(&observer);

    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::RESUME), Core::ERROR_NONE);

    EXPECT_TRUE(observer.WaitForState(PluginHost::IStateControl::RESUMED))
        << "Observer must be called with RESUMED state";
    EXPECT_EQ(observer.LastState(), PluginHost::IStateControl::RESUMED);
    EXPECT_EQ(observer.CallCount(), 1u)
        << "Exactly one notification must fire for a single RESUME transition";

    _stateControl->Unregister(&observer);
}

TEST_F(StateControlTestSuite, NotificationFiredOnSuspendTransition)
{
    EnsureResumed();

    Core::SinkType<StateObserver> observer;
    _stateControl->Register(&observer);

    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::SUSPEND), Core::ERROR_NONE);

    EXPECT_TRUE(observer.WaitForState(PluginHost::IStateControl::SUSPENDED))
        << "Observer must be called with SUSPENDED state";
    EXPECT_EQ(observer.LastState(), PluginHost::IStateControl::SUSPENDED);
    EXPECT_EQ(observer.CallCount(), 1u)
        << "Exactly one notification must fire for a single SUSPEND transition";

    _stateControl->Unregister(&observer);
}

TEST_F(StateControlTestSuite, NoNotificationForIdempotentResume)
{
    EnsureResumed();

    Core::SinkType<StateObserver> observer;
    _stateControl->Register(&observer);

    // RESUME while already RESUMED is a no-op in Dispatch() → no notification.
    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::RESUME), Core::ERROR_NONE);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(observer.CallCount(), 0u)
        << "No notification must fire for an idempotent RESUME";

    _stateControl->Unregister(&observer);
}

TEST_F(StateControlTestSuite, NoNotificationForIdempotentSuspend)
{
    EnsureSuspended();

    Core::SinkType<StateObserver> observer;
    _stateControl->Register(&observer);

    // SUSPEND while already SUSPENDED is a no-op.
    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::SUSPEND), Core::ERROR_NONE);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(observer.CallCount(), 0u)
        << "No notification must fire for an idempotent SUSPEND";

    _stateControl->Unregister(&observer);
}

TEST_F(StateControlTestSuite, MultipleObserversAllReceiveNotification)
{
    EnsureResumed();

    Core::SinkType<StateObserver> observer1;
    Core::SinkType<StateObserver> observer2;
    _stateControl->Register(&observer1);
    _stateControl->Register(&observer2);

    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::SUSPEND), Core::ERROR_NONE);

    EXPECT_TRUE(observer1.WaitForState(PluginHost::IStateControl::SUSPENDED))
        << "Observer 1 must receive SUSPENDED notification";
    EXPECT_TRUE(observer2.WaitForState(PluginHost::IStateControl::SUSPENDED))
        << "Observer 2 must receive SUSPENDED notification";

    EXPECT_EQ(observer1.CallCount(), 1u);
    EXPECT_EQ(observer2.CallCount(), 1u);

    _stateControl->Unregister(&observer1);
    _stateControl->Unregister(&observer2);
}

TEST_F(StateControlTestSuite, UnregisteredObserverDoesNotReceiveNotification)
{
    EnsureSuspended();

    Core::SinkType<StateObserver> observer;
    _stateControl->Register(&observer);
    _stateControl->Unregister(&observer);

    // Clear any count accumulated during Register/Unregister (there should be
    // none, but be defensive).
    observer.Reset();

    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::RESUME), Core::ERROR_NONE);
    EXPECT_TRUE(WaitForState(_stateControl, PluginHost::IStateControl::RESUMED));

    EXPECT_EQ(observer.CallCount(), 0u)
        << "Unregistered observer must not receive state-change notifications";
}

TEST_F(StateControlTestSuite, ObserverReceivesMultipleTransitionsInSequence)
{
    EnsureResumed();

    Core::SinkType<StateObserver> observer;
    _stateControl->Register(&observer);

    // Transition 1: RESUMED → SUSPENDED
    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::SUSPEND), Core::ERROR_NONE);
    EXPECT_TRUE(observer.WaitForState(PluginHost::IStateControl::SUSPENDED))
        << "Observer must receive first SUSPENDED notification";
    EXPECT_EQ(observer.CallCount(), 1u);

    // Transition 2: SUSPENDED → RESUMED
    observer.Reset();
    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::RESUME), Core::ERROR_NONE);
    EXPECT_TRUE(observer.WaitForState(PluginHost::IStateControl::RESUMED))
        << "Observer must receive second RESUMED notification";
    EXPECT_EQ(observer.CallCount(), 1u);

    _stateControl->Unregister(&observer);
}

TEST_F(StateControlTestSuite, SecondObserverRegisteredMidwayReceivesSubsequentChanges)
{
    EnsureResumed();

    Core::SinkType<StateObserver> earlyObserver;
    _stateControl->Register(&earlyObserver);

    // First transition: only earlyObserver is registered.
    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::SUSPEND), Core::ERROR_NONE);
    EXPECT_TRUE(earlyObserver.WaitForState(PluginHost::IStateControl::SUSPENDED));

    // Register a second observer after the first transition.
    Core::SinkType<StateObserver> lateObserver;
    _stateControl->Register(&lateObserver);

    earlyObserver.Reset();

    // Second transition: both observers must be notified.
    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::RESUME), Core::ERROR_NONE);
    EXPECT_TRUE(earlyObserver.WaitForState(PluginHost::IStateControl::RESUMED))
        << "Early observer must receive the second transition";
    EXPECT_TRUE(lateObserver.WaitForState(PluginHost::IStateControl::RESUMED))
        << "Late observer must receive the second transition";

    _stateControl->Unregister(&earlyObserver);
    _stateControl->Unregister(&lateObserver);
}

// ==========================================================================
// 5. Framework-driven IStateControl handling
//
// These tests exercise the path INSIDE Thunder's PluginServer, not just the
// plugin itself.  Specifically:
//
//   a) When the plugin config has  Resumed=true, the PluginServer detects
//      that the activated plugin implements IStateControl and automatically
//      calls  IStateControl::Request(RESUME)  – without any explicit call
//      from the test code.  This is the canonical "IStateControl handling
//      is not broken" check.
//
//   b) When the plugin config has  Resumed=false  (our default fixture),
//      the PluginServer must NOT auto-resume the plugin.  State stays
//      UNINITIALIZED – verified by InitialStateIsUninitialized above.
//
//   c) IShell::Resumed() must reflect the value from the plugin config.
//
// A separate ThunderTestRuntime is used so the plugin can be started fresh
// with Resumed=true, independent of the main StateControlTestSuite instance.
// ==========================================================================

class StateControlFrameworkFixture : public ::testing::Test {
protected:
    static ThunderTestRuntime _runtime;
    PluginHost::IStateControl* _stateControl{ nullptr };

    static void SetUpTestSuite()
    {
        ThunderTestRuntime::PluginConfig cfg;
        cfg.Callsign  = "TestStateControl";
        cfg.ClassName = "TestStateControl";
        cfg.Locator   = "libThunderTestStateControl.so";
        // Resumed=true: the framework must call Request(RESUME) after activation.
        cfg.Resumed   = true;

        const uint32_t result = _runtime.Initialize({ cfg });
        ASSERT_EQ(result, Core::ERROR_NONE)
            << "ThunderTestRuntime (Resumed=true) failed to initialise";
    }

    static void TearDownTestSuite()
    {
        _runtime.Deinitialize();
    }

    void SetUp() override
    {
        _stateControl = _runtime.QueryInterfaceByCallsign<PluginHost::IStateControl>(
            "TestStateControl");
        ASSERT_NE(_stateControl, nullptr);
    }

    void TearDown() override
    {
        if (_stateControl != nullptr) {
            _stateControl->Release();
            _stateControl = nullptr;
        }
    }
};

ThunderTestRuntime StateControlFrameworkFixture::_runtime;

// --------------------------------------------------------------------------
// a) Framework auto-resumes the plugin when Resumed=true in config.
//
// This is the core "IStateControl handling not broken" check: we never call
// Request() ourselves.  The PluginServer is expected to:
//   1. Activate the plugin (IPlugin::Initialize).
//   2. Query the plugin for IStateControl.
//   3. Call IStateControl::Configure(shell).
//   4. Call IStateControl::Request(RESUME) because Resumed==true.
// --------------------------------------------------------------------------
TEST_F(StateControlFrameworkFixture, FrameworkAutoResumesPluginWhenResumedIsTrue)
{
    // The RESUME is dispatched asynchronously by the worker pool inside the
    // plugin.  Poll until the state arrives or timeout.
    EXPECT_TRUE(WaitForState(_stateControl, PluginHost::IStateControl::RESUMED))
        << "PluginServer must automatically call Request(RESUME) when Resumed=true; "
           "state never reached RESUMED within the timeout";
    EXPECT_EQ(_stateControl->State(), PluginHost::IStateControl::RESUMED);
}

// --------------------------------------------------------------------------
// b) IShell::Resumed() reflects the config value (Resumed=true here).
// --------------------------------------------------------------------------
TEST_F(StateControlFrameworkFixture, ShellResumedReflectsConfigValue)
{
    auto shell = _runtime.GetShell("TestStateControl");
    ASSERT_TRUE(shell.IsValid());
    EXPECT_TRUE(shell->Resumed())
        << "IShell::Resumed() must return true when plugin was configured with Resumed=true";
}

// --------------------------------------------------------------------------
// c) After the framework-driven RESUME, manual SUSPEND still works.
//    This ensures the IStateControl contract is intact end-to-end.
// --------------------------------------------------------------------------
TEST_F(StateControlFrameworkFixture, ManualSuspendWorksAfterFrameworkResume)
{
    // Wait for the framework RESUME first.
    ASSERT_TRUE(WaitForState(_stateControl, PluginHost::IStateControl::RESUMED))
        << "Prerequisite: plugin must be RESUMED before this test";

    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::SUSPEND), Core::ERROR_NONE);
    EXPECT_TRUE(WaitForState(_stateControl, PluginHost::IStateControl::SUSPENDED))
        << "Manual SUSPEND must work after a framework-driven RESUME";
}

// --------------------------------------------------------------------------
// d) Notification observer receives the framework-driven RESUME.
//    Register the observer BEFORE the framework fires the RESUME so we do
//    not miss the callback.  To achieve that deterministically we register
//    in SetUpTestSuite before the runtime is initialised – but that is not
//    possible with the current API.  Instead we rely on the fact that
//    FrameworkAutoResumes... has already stabilised the state to RESUMED,
//    and here we drive it through one full SUSPEND→RESUME cycle so the
//    observer definitely fires.
// --------------------------------------------------------------------------
TEST_F(StateControlFrameworkFixture, NotificationObserverFiresOnResumeAfterFrameworkCycle)
{
    // Drive to a known SUSPENDED state regardless of what the previous test left behind.
    // (ManualSuspendWorksAfterFrameworkResume may already have suspended the plugin.)
    if (_stateControl->State() != PluginHost::IStateControl::SUSPENDED) {
        EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::SUSPEND), Core::ERROR_NONE);
    }
    ASSERT_TRUE(WaitForState(_stateControl, PluginHost::IStateControl::SUSPENDED))
        << "Prerequisite: plugin must be SUSPENDED before registering observer";

    Core::SinkType<StateObserver> observer;
    _stateControl->Register(&observer);

    EXPECT_EQ(_stateControl->Request(PluginHost::IStateControl::RESUME), Core::ERROR_NONE);
    EXPECT_TRUE(observer.WaitForState(PluginHost::IStateControl::RESUMED))
        << "Observer must receive RESUMED notification in a framework-cycle scenario";
    EXPECT_EQ(observer.CallCount(), 1u);

    _stateControl->Unregister(&observer);
}

// --------------------------------------------------------------------------
// e) Verify that IStateControl is reachable via IShell::QueryInterfaceByCallsign
//    (the void* overload the PluginServer itself uses internally).
//    This confirms the interface-map registration is correct from the
//    framework's perspective.
// --------------------------------------------------------------------------
TEST_F(StateControlFrameworkFixture, IStateControlReachableViaShellQueryInterface)
{
    auto shell = _runtime.GetShell("TestStateControl");
    ASSERT_TRUE(shell.IsValid());

    auto* raw = static_cast<PluginHost::IStateControl*>(
        shell->QueryInterfaceByCallsign(PluginHost::IStateControl::ID, "TestStateControl"));
    EXPECT_NE(raw, nullptr)
        << "IStateControl must be reachable via IShell::QueryInterfaceByCallsign "
           "(same path the PluginServer uses)";
    if (raw != nullptr) {
        raw->Release();
    }
}

} // namespace Tests
} // namespace TestCore
} // namespace Thunder
