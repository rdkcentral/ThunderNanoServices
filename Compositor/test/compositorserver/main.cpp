/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Metrological B.V.
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

#ifndef MODULE_NAME
#define MODULE_NAME CompositorTestServer
#endif

#include <core/core.h>
#include <localtracer/localtracer.h>
#include <messaging/messaging.h>
#include <plugins/plugins.h>

#include <interfaces/IComposition.h>
#include <interfaces/ICompositionBuffer.h>
#include <compositorbuffer/CompositorBufferType.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace Thunder {
class WorkerPoolImplementation : public Core::WorkerPool {

private:
    class Dispatcher : public Core::ThreadPool::IDispatcher {
    public:
        Dispatcher(const Dispatcher&) = delete;
        Dispatcher& operator=(const Dispatcher&) = delete;

        Dispatcher() = default;
        ~Dispatcher() override = default;

    private:
        void Initialize() override {}
        void Deinitialize() override {}
        void Dispatch(Core::IDispatch* job) override
        {
            ASSERT(job != nullptr);
            job->Dispatch();
        }
    };

public:
    WorkerPoolImplementation() = delete;
    WorkerPoolImplementation(const WorkerPoolImplementation&) = delete;
    WorkerPoolImplementation& operator=(const WorkerPoolImplementation&) = delete;

    WorkerPoolImplementation(const uint8_t threads, const uint32_t stackSize, const uint32_t queueSize)
        : WorkerPool(threads, stackSize, queueSize, &_dispatcher)
        , _dispatcher()
    {
    }

    ~WorkerPoolImplementation()
    {
        Core::WorkerPool::Stop();
    }

    void Run()
    {
        Core::WorkerPool::Run();
        Core::WorkerPool::Join();
    }

    void Stop()
    {
        Core::WorkerPool::Stop();
    }

private:
    Dispatcher _dispatcher;
}; // class WorkerPoolImplementation

class MockedShell : public PluginHost::IShell {
private:
    class RootObject : public Core::JSON::Container {
    private:
        RootObject(const RootObject&) = delete;
        RootObject& operator=(const RootObject&) = delete;

    public:
        RootObject()
            : Config(false)
        {
            Add(_T("root"), &Config);
        }
        ~RootObject() override = default;

    public:
        Core::JSON::String Config;
    }; // class RootObject

    class MockedSubSystem : public PluginHost::ISubSystem {
    private:
        MockedSubSystem(const MockedSubSystem&) = delete;
        MockedSubSystem& operator=(const MockedSubSystem&) = delete;

    public:
        MockedSubSystem()
        {
        }
        ~MockedSubSystem() override = default;

        BEGIN_INTERFACE_MAP(CompositorImplementation)
        INTERFACE_ENTRY(PluginHost::ISubSystem)
        END_INTERFACE_MAP

        // No reference counting for this object.
        uint32_t AddRef() const { return Core::ERROR_COMPOSIT_OBJECT; }
        uint32_t Release() const { return Core::ERROR_COMPOSIT_OBJECT; }

        void Register(ISubSystem::INotification* notification){};
        void Unregister(ISubSystem::INotification* notification){};

        // Events setter and getters.
        Core::hresult Set(const subsystem type, Core::IUnknown* information)
        {
            TRACE(Trace::Information, (_T("Set Subsystem: %d"), type));
            return Core::ERROR_UNAVAILABLE;
        }

        const Core::IUnknown* Get(const subsystem type) const
        {
            TRACE(Trace::Information, (_T("Get Subsystem: %d"), type));
            return nullptr;
        }

        bool IsActive(const subsystem type) const
        {
            TRACE(Trace::Information, (_T("IsActive Subsystem: %d"), type));
            return false;
        };

        // Report some version info on the Hosting Application
        string BuildTreeHash() const
        {
            return _T("Mocked");
        }
        string Version() const
        {
            return _T("1.0.0");
        };

    }; // class MockedSubSystem

public:
    MockedShell(const string& rootPath, const string& configurationFile)
        : _rootPath(rootPath)
        , _plugin()
        , _config()
        , _engine()
    {
        _engine = Core::ProxyType<WorkerPoolImplementation>::Create(1, Core::Thread::DefaultStackSize(), 16);
        Core::IWorkerPool::Assign(&(*_engine));

        std::thread(
            [this]() {
                _engine->Run();
            })
            .detach();

        Core::File file(std::string(PluginPath() + configurationFile).c_str());
        
        TRACE(Trace::Information, (_T("Opening config file [%s]."), file.Name().c_str()));

        ASSERT(file.Exists() == true);

        if (file.Open(true) == false) {
            TRACE(Trace::Error, (_T("Plugin config file [%s] could not be opened."), file.Name().c_str()));
        } else {
            Core::OptionalType<Core::JSON::Error> error;

            _config.IElement::FromFile(file, error);
            std::string compositionLibrary;

            if (error.IsSet() == true) {
                TRACE(Trace::Error, (_T("Error in config line: [%s]"), file.Name().c_str()));
            } else {
                Plugin::Config::RootConfig rootConfig;

                TRACE(Trace::Information, (_T("Plugin config line: [%s]"), ConfigLine().c_str()));

                RootObject root;

                root.FromString(ConfigLine(), error);

                if (error.IsSet() == true) {
                    TRACE(Trace::Error, (_T("Error in config line: [%s]"), ConfigLine().c_str()));
                } else {
                    rootConfig.FromString(root.Config.Value(), error);

                    compositionLibrary = DataPath() + _config.ClassName.Value() + std::string("/") + rootConfig.Locator.Value();
                }
            }

            _plugin = compositionLibrary.c_str();
        }
    }

    MockedShell() = delete;
    MockedShell(const MockedShell&) = delete;
    MockedShell& operator=(const MockedShell&) = delete;

    ~MockedShell()
    {
        Core::IWorkerPool::Assign(nullptr);

        if (_engine.IsValid() == true) {
            _engine->Stop();
            _engine.Release();
        }
    };

public:
    BEGIN_INTERFACE_MAP(CompositorImplementation)
    INTERFACE_ENTRY(PluginHost::IShell)
    END_INTERFACE_MAP

    void* Root(uint32_t& pid, const uint32_t waitTime, const string className, const uint32_t interface, const uint32_t version = ~0)
    {
        pid = 0;
        void* result = nullptr;

        if (_plugin.IsLoaded()) {
            result = Core::ServiceAdministrator::Instance().Instantiate(
                _plugin,
                className.c_str(),
                version,
                interface);
        }

        return result;
    }

    string ConfigLine() const
    {
        return (_config.Configuration.Value());
    }

    PluginHost::ISubSystem* SubSystems() { return &_subSystem; }

    string PersistentPath() const { return SystemRootPath() + string("/var/thunder/Thunder/"); }
    string VolatilePath() const { return "/tmp/"; }
    string DataPath() const { return SystemRootPath() + string("/usr/share/Thunder/"); }
    string ProxyStubPath() const { return SystemRootPath() + string("/usr/lib/thunder/proxystubs/"); }
    string SystemPath() const { return SystemRootPath() + string("/usr/lib/thunder/plugins/"); }
    string PluginPath() const { return SystemRootPath() + string("/etc/Thunder/plugins/"); }
    string SystemRootPath() const { return _rootPath; }

    // No reference counting for this object.
    uint32_t AddRef() const { return Core::ERROR_NONE;}
    uint32_t Release() const { return 0; }

    /**
     * mocked interface methods
     */
    void EnableWebServer(const string& URLPath, const string& fileSystemPath) {}
    void DisableWebServer() {}
    string Model() const { return "mocked"; }
    bool Background() const { return false; }
    string Accessor() const { return "mocked"; }
    string WebPrefix() const { return "mocked"; }
    string Locator() const { return "mocked"; }
    string ClassName() const { return "mocked"; }
    string Versions() const { return "mocked"; }
    string Callsign() const { return "mocked"; }
    Core::hresult SystemRootPath(const string& systemRootPath) { return Core::ERROR_UNAVAILABLE; };
    PluginHost::IShell::startmode StartMode() const { return PluginHost::IShell::startmode::ACTIVATED; }
    Core::hresult StartMode(const PluginHost::IShell::startmode value) { return Core::ERROR_UNAVAILABLE; }
    string Substitute(const string& input) const { return input; };
    bool Resumed() const { return true; };
    Core::hresult Resumed(const bool value) { return Core::ERROR_UNAVAILABLE; };
    string HashKey() const { return string(); };
    Core::hresult Metadata(string& info /* @out */) const { return Core::ERROR_NONE; };
    bool IsSupported(const uint8_t version) const { return true; }
    void Notify(const string& message) {}
    void Register(PluginHost::IPlugin::INotification* sink) {}
    void Unregister(PluginHost::IPlugin::INotification* sink) {}
    state State() const { return PluginHost::IShell::ACTIVATED; }
    void* QueryInterfaceByCallsign(const uint32_t id, const string& name) { return nullptr; }
    Core::hresult Activate(const reason) { return Core::ERROR_UNAVAILABLE; }
    Core::hresult Deactivate(const reason) { return Core::ERROR_UNAVAILABLE; }
    Core::hresult Unavailable(const reason) { return Core::ERROR_UNAVAILABLE; }
    Core::hresult Hibernate(const uint32_t timeout) { return Core::ERROR_UNAVAILABLE; }
    reason Reason() const { return PluginHost::IShell::REQUESTED; }
    uint32_t Submit(const uint32_t Id, const Core::ProxyType<Core::JSON::IElement>& response) { return Core::ERROR_UNAVAILABLE; }
    ICOMLink* COMLink() { return nullptr; }
    Core::hresult ConfigLine(const string& config) { return Core::ERROR_NONE; }

private:
    const std::string _rootPath;
    Core::Library _plugin;
    Plugin::Config _config;
    MockedSubSystem _subSystem;
    Core::ProxyType<WorkerPoolImplementation> _engine;
}; // class MockedShell
}

int main(int argc, const char* argv[])
{
    Thunder::Messaging::LocalTracer& tracer = Thunder::Messaging::LocalTracer::Open();

    const char* executableName(Thunder::Core::FileNameOnly(argv[0]));

    // constexpr char defaultCompositionLibrary[] = "libcompositorplugin.so";
    constexpr char defaultInstallPath[] = ""; // "/home/bram/Projects/metrological/Thunder/install";

    Thunder::Core::ProxyType<Thunder::RPC::InvokeServerType<2, 0, 8>> rpcEngine = Thunder::Core::ProxyType<Thunder::RPC::InvokeServerType<2, 0, 8>>::Create();

    std::string compositionLibrary;

    // if (argc > 1) {
    //     compositionLibrary = "Compositor/libcompositorplugin.so";
    // } else {
    //     compositionLibrary = argv[1];
    // }

    {
        Thunder::Messaging::ConsolePrinter printer(true);

        tracer.Callback(&printer);

        const std::vector<string> modules = {
            "CompositorMesaImplementation",
            "CompositorTestServer",
            "CompositorBuffer",
            "CompositorBackend",
            "CompositorRenderer",
            "DRMCommon",
            "CompositorRenderEGL",
            "CompositorRenderer"
        };

        for (auto module : modules) {
            tracer.EnableMessage(module, "", true);
        }

        TRACE_GLOBAL(Thunder::Trace::Information, ("%s - build: %s", executableName, __TIMESTAMP__));

        Thunder::MockedShell service(defaultInstallPath, "Compositor.json");

        // ASSERT(Thunder::Core::IWorkerPool::IsAvailable() == true);

        uint32_t connectionId = 0;
        Thunder::Exchange::IComposition* composition = nullptr;
        Thunder::Exchange::IComposition::IDisplay* display = nullptr;

        void* iface = service.Root(connectionId, 2000, _T("CompositorImplementation"), Thunder::Exchange::IComposition::ID);

        if (iface != nullptr) {
            composition = reinterpret_cast<Thunder::Exchange::IComposition*>(iface);
        }

        ASSERT(composition != nullptr);

        if (composition != nullptr) {
            composition->Configure(&service);

            TRACE_GLOBAL(Thunder::Trace::Information, (_T("Composition: %p, %dx%d"), composition, Thunder::Exchange::IComposition::HeightFromResolution(composition->Resolution()), Thunder::Exchange::IComposition::WidthFromResolution(composition->Resolution())));

            display = composition->QueryInterface<Thunder::Exchange::IComposition::IDisplay>();

            ASSERT(display != nullptr);

            TRACE_GLOBAL(Thunder::Trace::Information, (_T("Display: %p, %s %dx%d"), display, display->Port().c_str(), Thunder::Exchange::IComposition::HeightFromResolution(display->Resolution()), Thunder::Exchange::IComposition::WidthFromResolution(display->Resolution())));
        }

        Thunder::Exchange::IComposition::IClient* client = nullptr;

        char keyPress;

        do {
            keyPress = toupper(getchar());

            switch (keyPress) {
            case 'C': {
                if (client == nullptr) {
                    client = display->CreateClient("test", Thunder::Exchange::IComposition::WidthFromResolution(display->Resolution()), Thunder::Exchange::IComposition::HeightFromResolution(display->Resolution()));
                }

                if (client != nullptr) {
                    TRACE_GLOBAL(Thunder::Trace::Information, (_T("Active client: %p, %s %dx%d"), client, client->Name().c_str(), client->Geometry().width, client->Geometry().height));
                }
                break;
            }
            case 'R': {
                if (client != nullptr) {
                    client->Release();
                    client = nullptr;
                }
                break;
            }
            default:
                break;
            }
        } while (keyPress != 'Q');

        if (client != nullptr) {
            client->Release();
        }

        if (display != nullptr) {
            display->Release();
        }

        if (composition != nullptr) {
            composition->Release();
        }

        TRACE_GLOBAL(Thunder::Trace::Information, ("Exiting %s.... ", executableName));
    }

    tracer.Close();
    Thunder::Core::Singleton::Dispose();

    return 0;
}
