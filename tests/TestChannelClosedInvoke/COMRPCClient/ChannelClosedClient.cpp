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
 
// COMRPCApp.cpp : This file contains the 'main' function. Program execution begins and ends there.

// Since lower levels 
// #define _TRACE_LEVEL 5

#define MODULE_NAME ChannelClosedClient

#include <core/core.h>
#include <com/com.h>
#include <definitions/definitions.h>
#include <plugins/Types.h>
#include <qa_interfaces/ITestChannelClosed.h>

#include <atomic>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

#ifdef __LINUX__
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>
#endif

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace Thunder;

class Callback : public QualityAssurance::ITestChannelClosed::ICallback {
public:
    Callback(const Callback&) = delete;
    Callback& operator=(const Callback&) = delete;
    Callback(Callback&&) = delete;
    Callback& operator=(Callback&&) = delete;

    Callback() = default;
    ~Callback() override = default;

    void Done() override
    {
        printf("\n\ndone received...\n\n");
    }

    BEGIN_INTERFACE_MAP(Callback)
    INTERFACE_ENTRY(QualityAssurance::ITestChannelClosed::ICallback)
    END_INTERFACE_MAP
};

class StandaloneCallback : public QualityAssurance::ITestChannelClosed::ICallback {
public:
    StandaloneCallback(const StandaloneCallback&) = delete;
    StandaloneCallback& operator=(const StandaloneCallback&) = delete;
    StandaloneCallback(StandaloneCallback&&) = delete;
    StandaloneCallback& operator=(StandaloneCallback&&) = delete;

    StandaloneCallback()
        : _referenceCount(1)
        , _doneCount(0)
    {
    }

    ~StandaloneCallback() override = default;

public:
    uint32_t AddRef() const override
    {
        _referenceCount.fetch_add(1);
        return Core::ERROR_NONE;
    }

    uint32_t Release() const override
    {
        const uint32_t updated = _referenceCount.fetch_sub(1) - 1;

        if (updated == 0) {
            delete this;
        }

        return Core::ERROR_NONE;
    }

    void Done() override
    {
        _doneCount.fetch_add(1);
        printf("\n\nstandalone done received...\n\n");
    }

    uint32_t DoneCount() const
    {
        return _doneCount.load();
    }

    BEGIN_INTERFACE_MAP(StandaloneCallback)
    INTERFACE_ENTRY(QualityAssurance::ITestChannelClosed::ICallback)
    END_INTERFACE_MAP

private:
    mutable std::atomic<uint32_t> _referenceCount;
    mutable std::atomic<uint32_t> _doneCount;
};

// SleepyCallback is intentionally composite (via SinkType<>) and sleeps inside Done().
// This keeps the reverse COM-RPC call in-flight long enough for the abort() to fire
// while the plugin still holds a proxy to this object.  That puts the composite object
// in Administrator::_channelReferenceMap so DeleteChannel() can exercise the
// IsComposit() == true → skip-forced-release branch during crash recovery.
class SleepyCallback : public QualityAssurance::ITestChannelClosed::ICallback {
public:
    SleepyCallback(const SleepyCallback&) = delete;
    SleepyCallback& operator=(const SleepyCallback&) = delete;
    SleepyCallback(SleepyCallback&&) = delete;
    SleepyCallback& operator=(SleepyCallback&&) = delete;

    SleepyCallback() = default;
    ~SleepyCallback() override = default;

    void Done() override
    {
        // Sleep well past the abort window (4 s) so the reverse-path proxy
        // is still held by the plugin when the client process crashes.
        // In the crash scenario this call should never return normally.
        printf("[CRASH] SleepyCallback::Done() entered - holding reverse-path proxy in-flight...\n");
        SleepMs(10000);
        printf("[CRASH] SleepyCallback::Done() woke (unexpected in crash scenario)\n");
    }

    BEGIN_INTERFACE_MAP(SleepyCallback)
    INTERFACE_ENTRY(QualityAssurance::ITestChannelClosed::ICallback)
    END_INTERFACE_MAP
};

namespace {

    bool VerifyOwnershipModels(QualityAssurance::ITestChannelClosed* testinterface)
    {
        ASSERT(testinterface != nullptr);

        printf("[OWNERSHIP] Starting ownership model verification...\n");

        Core::SinkType<Callback> composite;
        const uint32_t compositeType = composite.AddRef();
        composite.Release();

        StandaloneCallback* standalone = new StandaloneCallback();
        ASSERT(standalone != nullptr);

        const uint32_t standaloneType = standalone->AddRef();
        standalone->Release();

        const Core::hresult compositeResult = testinterface->Test(&composite);
        const Core::hresult standaloneResult = testinterface->Test(standalone);
        const bool standaloneCallbackReached = (standalone->DoneCount() > 0);

        // Release creator-owned reference.
        standalone->Release();

        const bool distinguishedTypes =
            (compositeType == Core::ERROR_COMPOSIT_OBJECT) &&
            (standaloneType == Core::ERROR_NONE);

        const bool rpcResultOk =
            (compositeResult == Core::ERROR_NONE) &&
            (standaloneResult == Core::ERROR_NONE) &&
            (standaloneCallbackReached == true);

        printf("[OWNERSHIP] composite AddRef result: %u (expected %u)\n", compositeType, Core::ERROR_COMPOSIT_OBJECT);
        printf("[OWNERSHIP] standalone AddRef result: %u (expected %u)\n", standaloneType, Core::ERROR_NONE);
        printf("[OWNERSHIP] reverse callback path reached: %s\n", standaloneCallbackReached ? "yes" : "no");

        const bool pass = (distinguishedTypes == true) && (rpcResultOk == true);
        printf("[OWNERSHIP] RESULT: %s\n", pass ? "PASS" : "FAIL");

        return pass;
    }

    void TriggerCrashWhileReversePathActive(const Core::NodeId& endPoint)
    {
        Core::ProxyType<RPC::CommunicatorClient> primaryClient =
            Core::ProxyType<RPC::CommunicatorClient>::Create(
                endPoint,
                Core::ProxyType<Core::IIPCServer>(Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create()));

        QualityAssurance::ITestChannelClosed* blockingInterface =
            primaryClient->Open<QualityAssurance::ITestChannelClosed>(_T("TestChannelClosed"), ~0, 2000);

        if (blockingInterface == nullptr) {
            printf("[CRASH] Could not acquire interface in crash child.\n");
            std::_Exit(EXIT_FAILURE);
        }

        std::thread killThread([]() {
            SleepMs(4000);
            printf("[CRASH] aborting client while reverse path call is pending...\n");
            fflush(stdout);
            abort();
        });

        std::thread reverseTrafficThread([&endPoint]() {
            // SinkType<SleepyCallback> is a composite-owned endpoint.
            // The plugin will call Done() on it via reverse COM-RPC.  Done()
            // sleeps 10 s, keeping the proxy in Administrator::_channelReferenceMap
            // with IsComposit()==true across the abort window (4 s).  When the
            // client crashes, DeleteChannel() will find this composite entry and
            // must NOT force-release it.  The post-crash probe in the parent
            // confirms the framework survived that decision.
            Core::SinkType<SleepyCallback> reverseSink;

            Core::ProxyType<RPC::CommunicatorClient> reverseClient =
                Core::ProxyType<RPC::CommunicatorClient>::Create(
                    endPoint,
                    Core::ProxyType<Core::IIPCServer>(Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create()));

            QualityAssurance::ITestChannelClosed* reverseInterface =
                reverseClient->Open<QualityAssurance::ITestChannelClosed>(_T("TestChannelClosed"));

            if (reverseInterface != nullptr) {
                SleepMs(1000); // give primary-channel Block() time to establish
                printf("[CRASH] invoking Test(composite-sink) - reverse RPC will block inside Done()...\n");
                // This call blocks: plugin calls Done() (reverse RPC) which sleeps
                // 10 s → plugin is held waiting → abort() fires at 4 s mid-sleep.
                reverseInterface->Test(&reverseSink);
                // Never reached in crash scenario.
                printf("[CRASH] reverse interface returned unexpectedly before abort window.\n");
                reverseInterface->Release();
            } else {
                printf("[CRASH] Could not acquire reverse interface in crash child.\n");
            }

            reverseClient->Close(RPC::CommunicationTimeOut);
        });

        printf("[CRASH] blocking worker to create deterministic crash window...\n");
        blockingInterface->Block(15);
        printf("[CRASH] Block(15) returned in crash child before abort path completed.\n");

        // Should never be reached due to abort().
        reverseTrafficThread.join();
        killThread.join();

        blockingInterface->Release();
        primaryClient->Close(RPC::CommunicationTimeOut);
        std::_Exit(EXIT_FAILURE);
    }

#ifdef __LINUX__
    bool VerifyCompositeCrashProtection(const Core::NodeId& endPoint)
    {
        printf("[CRASH] Starting crash scenario verification...\n");

        // Use fork()+execv() instead of plain fork() to give the child a clean
        // process image with no inherited libThunderCore singleton state
        // (ResourceMonitor signalfd / thread handles that would fire SIGUSR2
        // before abort() gets a chance to run).
        //
        // Note: posix_spawn() is available on this platform and avoids fork()
        // entirely, which is preferred on embedded targets without an MMU where
        // fork() is expensive.  Thunder is typically deployed on embedded systems
        // — consider posix_spawn as a future refinement for this launch path.
        char selfPath[PATH_MAX] = {};
        if (readlink("/proc/self/exe", selfPath, sizeof(selfPath) - 1) < 0) {
            printf("[CRASH] readlink(/proc/self/exe) failed: %s\n", strerror(errno));
            return false;
        }

        const std::string endPointStr = endPoint.QualifiedName();

        const pid_t child = fork();

        if (child == 0) {
            // Re-execute this binary with the crash-worker flag so the child
            // starts with a clean address space — no inherited singleton state.
            char* const args[] = {
                selfPath,
                const_cast<char*>("--crash-worker"),
                const_cast<char*>(endPointStr.c_str()),
                nullptr
            };
            execv(selfPath, args);
            // execv only returns on failure.
            printf("[CRASH] execv failed: %s\n", strerror(errno));
            std::_Exit(EXIT_FAILURE);
        }

        if (child < 0) {
            printf("[CRASH] fork() failed.\n");
            return false;
        }

        int status = 0;
        const pid_t waited = waitpid(child, &status, 0);

        if (waited < 0) {
            printf("[CRASH] waitpid() failed: errno=%d (%s)\n", errno, strerror(errno));
            return false;
        } else {
            printf("[CRASH] waitpid() returned pid=%d, raw status=%d\n", static_cast<int>(waited), status);
        }

        if (WIFEXITED(status)) {
            printf("[CRASH] child exited normally with code=%d\n", WEXITSTATUS(status));
        }

        if (WIFSIGNALED(status)) {
            const int signalNumber = WTERMSIG(status);
            printf("[CRASH] child terminated by signal=%d (%s)\n", signalNumber, strsignal(signalNumber));
        }

        const bool childAborted = WIFSIGNALED(status) && (WTERMSIG(status) == SIGABRT);
        printf("[CRASH] child termination expected SIGABRT: %s\n", childAborted ? "yes" : "no");

        // Probe the framework liveness with retries rather than a fixed delay.
        // In Release builds the daemon processes channel cleanup faster than in
        // Debug, so the daemon may still be in the middle of releasing the dead
        // channel's composite proxy when the first probe arrives.  A retry loop
        // is robust regardless of build type and does not rely on timing constants.
        static constexpr uint32_t ProbeRetryIntervalMs = 500;
        static constexpr uint32_t ProbeRetryMaxMs = 10000;

        bool frameworkAlive = false;
        uint32_t probeElapsedMs = 0;

        while ((frameworkAlive == false) && (probeElapsedMs < ProbeRetryMaxMs)) {
            SleepMs(ProbeRetryIntervalMs);
            probeElapsedMs += ProbeRetryIntervalMs;

            Core::ProxyType<RPC::CommunicatorClient> probeClient =
                Core::ProxyType<RPC::CommunicatorClient>::Create(
                    endPoint,
                    Core::ProxyType<Core::IIPCServer>(Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create()));

            QualityAssurance::ITestChannelClosed* probeInterface =
                probeClient->Open<QualityAssurance::ITestChannelClosed>(_T("TestChannelClosed"), ~0, 2000);

            if (probeInterface != nullptr) {
                const Core::hresult probeResult = probeInterface->Test();
                frameworkAlive = (probeResult == Core::ERROR_NONE);
                probeInterface->Release();
            }

            probeClient->Close(RPC::CommunicationTimeOut);

            if (frameworkAlive == false) {
                printf("[CRASH] probe attempt at %ums: not yet alive, retrying...\n", probeElapsedMs);
            }
        }

        // probeClient is closed inside the loop; nothing to close here.

        const bool pass = (childAborted == true) && (frameworkAlive == true);
        printf("[CRASH] framework alive after composite crash window: %s\n", frameworkAlive ? "yes" : "no");
        printf("[CRASH] RESULT: %s\n", pass ? "PASS" : "FAIL");

        return pass;
    }
#endif
}

class CommandLineParameters : public Core::Options
{
public :
    CommandLineParameters() = delete;

    CommandLineParameters(const CommandLineParameters&) = delete;
    CommandLineParameters(CommandLineParameters&&) = delete;

    CommandLineParameters& operator=(const CommandLineParameters&) = delete;
    CommandLineParameters& operator=(CommandLineParameters&&) = delete;

    CommandLineParameters(int count, TCHAR* arguments[])
        : Core::Options{ count, arguments, _T("n:?") }
        , _node{}
        , _processName{ Core::FileNameOnly(arguments[0]) }
        , _requestedUsage{ false }
    {
        Parse();
    }

    ~CommandLineParameters() override = default;

    void Option(const TCHAR option, const TCHAR* argument) override
    {
PUSH_WARNING(DISABLE_WARNING_IMPLICIT_FALLTHROUGH);
        switch(option) {
        case 'n' : _node = Core::NodeId{ argument };
                   break;
        case '?' :
        default  :
                   if (_requestedUsage != true) {
                      PrintUsage();
                   }
                   _requestedUsage = true;
        }
POP_WARNING();
    }

    const Core::NodeId& EndPoint() const
    {
        return _node;
    }

    bool RequestedUsage() const
    {
        return _requestedUsage;
    }

private :

    void PrintUsage() const
    {
        printf("Usage : %s -n <domain socket or ip address / port string> | -?\n", _processName.data());
        printf("  -n NodeId specification, e.g. '/tmp/domainsocket' or '127.0.0.0:8080'\n");
        printf("  -? Help.\n");
    }

    mutable Core::NodeId _node;
    const std::string _processName;
    mutable bool _requestedUsage;
};

int main(int argc, char* argv[])
{
#ifdef __LINUX__
    // Crash-worker mode: launched by VerifyCompositeCrashProtection() via
    // fork()+execv().  Run the crash scenario directly and exit — no
    // interactive loop, no inherited singleton state from the parent.
    if (argc == 3 && std::string(argv[1]) == "--crash-worker") {
        const Core::NodeId workerEndPoint{ argv[2] };
        if (workerEndPoint.IsValid() != false) {
            TriggerCrashWhileReversePathActive(workerEndPoint);
        } else {
            printf("[CRASH] crash-worker: invalid endpoint '%s'\n", argv[2]);
        }
        std::_Exit(EXIT_FAILURE);
    }
#endif

    CommandLineParameters parameters(argc, argv);

    if (   argc > 1
        // HasErrors() relies on Core::Options::_valid, which is not updated
        // on Linux by the current XGetopt parse path used here.
        // && parameters.HasErrors() != false
    ) {
        if (parameters.RequestedUsage() != true) {
            const Core::NodeId& endPoint{ parameters.EndPoint() };

            if (endPoint.IsValid() != false) {

                Core::ProxyType<RPC::CommunicatorClient> client = Core::ProxyType<RPC::CommunicatorClient>::Create(endPoint, Core::ProxyType<Core::IIPCServer>(Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create()));

                QualityAssurance::ITestChannelClosed* testinterface = client->Open<QualityAssurance::ITestChannelClosed>(_T("TestChannelClosed"), ~0, 2000);

                if (testinterface == nullptr) {
                    printf("Could not acquire testinterface\n");
                } else {
                    printf("Testinterface acquired\n\n");
                    printf("Press 'T' for legacy scenario, 'N' for ownership proof, 'C' for crash proof, and 'Q' to quit followed by ENTER/RETURN. : ");

                    Core::SinkType<Callback> callback;

                    char keyPress;

                    do {
                        keyPress = toupper(getchar());

                        switch (keyPress) {
                        case 'N': {
                            printf("Begin ownership distinction test...\n");
                            VerifyOwnershipModels(testinterface);
                            break;
                        }
                        case 'C': {
                            printf("Begin composite crash protection test...\n");
#ifdef __LINUX__
                            VerifyCompositeCrashProtection(endPoint);
#else
                            printf("[CRASH] Skipped: this scenario requires fork()/waitpid() and runs on Linux only.\n");
#endif
                            break;
                        }
                        case 'T': {
                            printf("Begin Testcase...\n");

                            std::thread t2;
                            std::thread t1([&]() {
                                t2 = std::move(std::thread([]() {
                                    SleepMs(4000); // give other thread time to send request and then kill this client so all channels get closed...
                                    printf("Kill client\n");
                                    abort();
                                }));
                                Core::ProxyType<RPC::CommunicatorClient> client = Core::ProxyType<RPC::CommunicatorClient>::Create(endPoint, Core::ProxyType<Core::IIPCServer>(Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create()));
                                QualityAssurance::ITestChannelClosed* testinterface = client->Open<QualityAssurance::ITestChannelClosed>(_T("TestChannelClosed"));
                                if (testinterface != nullptr) {
                                    SleepMs(1000); // give main thread time to send block call and block workerpool
                                    printf("Send test request\n");
                                    testinterface->Test();
            //                        testinterface->Test(&callback);
                                    testinterface->Release();
                                    client->Close(RPC::CommunicationTimeOut);
                                    client = std::move(Core::ProxyType<RPC::CommunicatorClient>());
                                }
                            });
                            printf("Blocking on testinterface\n");
                            testinterface->Block(15); // block for 15 seconds, might return sooner when comrpc timeout smaller of course
                            printf("Block ended\n");

                            t1.join();
                            t2.join();

                            break;
                        }
                        case 'Q':
                            break;
                        default:
                            break;
                        };
                    } while (keyPress != 'Q');

                    testinterface->Release();

                    callback.WaitReleased(RPC::CommunicationTimeOut);
                }

                client->Close(RPC::CommunicationTimeOut);

                client = std::move(Core::ProxyType<RPC::CommunicatorClient>());

            } else {
                printf("Invalid endpoint specification\n");
                parameters.Option('?', nullptr);
            }
        }

    } else {

        if (parameters.RequestedUsage() != true) {
            printf("Invalid parameter specification\n");
            parameters.Option('?', nullptr);
        }

    }

    Core::Singleton::Dispose();

    return 0;
}
