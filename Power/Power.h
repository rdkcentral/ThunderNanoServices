#ifndef __POWER_H
#define __POWER_H

#include "Module.h"
#include <interfaces/IPower.h>

namespace WPEFramework {
namespace Plugin {

class Power : public PluginHost::IPlugin, public PluginHost::IWeb {
private:
    Power(const Power&) = delete;
    Power& operator=(const Power&) = delete;

    class Notification : 
        public PluginHost::IPlugin::INotification,
        public PluginHost::VirtualInput::Notifier {

    private:
        Notification() = delete;
        Notification(const Notification&) = delete;
        Notification& operator=(const Notification&) = delete;

    public:
        explicit Notification(Power* parent)
            : _parent(*parent)
        {
            ASSERT(parent != nullptr);
        }
        ~Notification()
        {
        }

    public:
        virtual void Dispatch(uint32_t& keyCode)
        {
            _parent.KeyEvent(keyCode);
        }

        virtual void StateChange(PluginHost::IShell* plugin)
        {
            _parent.StateChange(plugin);
        }
        BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
        END_INTERFACE_MAP

    private:
        Power& _parent;
    };

    class Entry
    {
    private:
        Entry() = delete;
        Entry(const Entry& copy) = delete;
        Entry& operator= (const Entry&) = delete;

    public:
        Entry(PluginHost::IStateControl* entry)
            : _shell(entry)
            , _lastStateResumed(false)
        {
            ASSERT(_shell != nullptr);
            _shell->AddRef();
        }
        ~Entry()
        {
            _shell->Release();
        }

    public:
       bool Suspend()
       {
           bool succeeded (true);
           if (_shell->State() == PluginHost::IStateControl::RESUMED) {
               _lastStateResumed = true;
               succeeded = (_shell->Request(PluginHost::IStateControl::SUSPEND) == Core::ERROR_NONE);
           }
           return (succeeded);
       }
       bool Resume()
       {
           bool succeeded (true);
           if (_lastStateResumed == true) {
               _lastStateResumed = false;
               succeeded = (_shell->Request(PluginHost::IStateControl::RESUME) == Core::ERROR_NONE);
           }
           return (succeeded);
       }

    public:
        PluginHost::IStateControl* _shell;
        bool _lastStateResumed;
    };

    class Config : public Core::JSON::Container {
    private:
        Config(const Config&);
        Config& operator=(const Config&);

    public:
        Config()
            : Core::JSON::Container()
            , OutOfProcess(true)
        {
            Add(_T("outofprocess"), &OutOfProcess);
        }
        ~Config()
        {
        }

    public:
        Core::JSON::Boolean OutOfProcess;
    };

    typedef std::map<const string, Entry> Clients;

public:
    class Data : public Core::JSON::Container {
    private:
        Data(const Data&) = delete;
        Data& operator=(const Data&) = delete;

    public:
        Data()
            : Core::JSON::Container()
            , Status()
            , PowerState()
            , Timeout()
        {
            Add(_T("Status"), &Status);
            Add(_T("PowerState"), &PowerState);
            Add(_T("Timeout"), &Timeout);
        }
        ~Data()
        {
        }

    public:
        Core::JSON::DecUInt32 Status;
        Core::JSON::DecUInt32 PowerState;
        Core::JSON::DecUInt32 Timeout;
    };

public:
#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
    Power()
        : _adminLock()
        , _skipURL(0)
        , _pid(0)
        , _power(nullptr)
        , _service(nullptr)
        , _sink(this)
    {
    }
#ifdef __WIN32__
#pragma warning(default : 4355)
#endif
    virtual ~Power()
    {
    }

public:
    BEGIN_INTERFACE_MAP(Power)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_AGGREGATE(Exchange::IPower, _power)
    END_INTERFACE_MAP

public:
    //  IPlugin methods
    // -------------------------------------------------------------------------------------------------------
    // First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
    // information and services for this particular plugin. The Service object contains configuration information that
    // can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
    // If there is an error, return a string describing the issue why the initialisation failed.
    // The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
    // The lifetime of the Service object is guaranteed till the deinitialize method is called.
    virtual const string Initialize(PluginHost::IShell* service);

    // The plugin is unloaded from WPEFramework. This is call allows the module to notify clients
    // or to persist information if needed. After this call the plugin will unlink from the service path
    // and be deactivated. The Service object is the same as passed in during the Initialize.
    // After theis call, the lifetime of the Service object ends.
    virtual void Deinitialize(PluginHost::IShell* service);

    // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
    // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
    virtual string Information() const;

    //  IWeb methods
    // -------------------------------------------------------------------------------------------------------
    virtual void Inbound(Web::Request& request);
    virtual Core::ProxyType<Web::Response> Process(const Web::Request& request);
    PluginHost::IShell* GetService() { return _service; }

private:
    void KeyEvent(const uint32_t keyCode);
    void StateChange(PluginHost::IShell* plugin);
    void ControlClients(Exchange::IPower::PCState state);

private:
    Core::CriticalSection _adminLock;
    uint32_t _skipURL;
    uint32_t _pid;
    PluginHost::IShell* _service;
    Clients  _clients;
    Exchange::IPower* _power;
    Core::Sink<Notification> _sink;
};
} //namespace Plugin
} //namespace WPEFramework

#endif // __POWER_H
