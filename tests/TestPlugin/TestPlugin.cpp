#include "Module.h"
#include "TestPlugin.h"

namespace Thunder {
namespace Plugin {

    namespace {
        static Metadata<TestPlugin> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

    const string TestPlugin::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        _service = service;

        QualityAssurance::JTestPlugin::Register(*this, this);

        return {};
    }

    void TestPlugin::Deinitialize(PluginHost::IShell* /* service */)
    {
        QualityAssurance::JTestPlugin::Unregister(*this);
        _service = nullptr;
    }

    Core::hresult TestPlugin::Echo(const string& input, string& output)
    {
        output = input;
        return Core::ERROR_NONE;
    }

    Core::hresult TestPlugin::Greet(const string& name, string& message)
    {
        if (name.empty()) {
            message = "Hello, World!";
        } else {
            message = "Hello, " + name + "!";
        }
        NotifyGreeting(message);
        return Core::ERROR_NONE;
    }

    Core::hresult TestPlugin::Register(ITestPlugin::INotification* notification)
    {
        ASSERT(notification != nullptr);
        _adminLock.Lock();
        auto it = std::find(_notifications.begin(), _notifications.end(), notification);
        ASSERT(it == _notifications.end());
        if (it == _notifications.end()) {
            notification->AddRef();
            _notifications.push_back(notification);
        }
        _adminLock.Unlock();
        return Core::ERROR_NONE;
    }

    Core::hresult TestPlugin::Unregister(ITestPlugin::INotification* notification)
    {
        ASSERT(notification != nullptr);
        _adminLock.Lock();
        auto it = std::find(_notifications.begin(), _notifications.end(), notification);
        ASSERT(it != _notifications.end());
        if (it != _notifications.end()) {
            (*it)->Release();
            _notifications.erase(it);
        }
        _adminLock.Unlock();
        return Core::ERROR_NONE;
    }

    void TestPlugin::NotifyGreeting(const string& message)
    {
        _adminLock.Lock();
        for (auto* notification : _notifications) {
            notification->OnGreeting(message);
        }
        _adminLock.Unlock();
        QualityAssurance::JTestPlugin::Event::OnGreeting(*this, message);
    }

} // namespace Plugin
} // namespace Thunder
