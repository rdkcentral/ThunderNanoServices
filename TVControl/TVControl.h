#ifndef ___PLUGINTVCONTROL_H
#define ___PLUGINTVCONTROL_H

#include "Module.h"
#include <JSData.h>
#include <interfaces/IGuide.h>
#include <interfaces/IStreaming.h>

namespace WPEFramework {
namespace Plugin {

class TVControl : public PluginHost::IPlugin, public PluginHost::IWeb {
private:
    TVControl(const TVControl&) = delete;
    TVControl& operator=(const TVControl&) = delete;

    class StreamingNotification
        : public RPC::IRemoteProcess::INotification
        , public Exchange::IStreaming::INotification {
    private:
        StreamingNotification() = delete;
        StreamingNotification(const StreamingNotification&) = delete;

    public:
        explicit StreamingNotification(TVControl* parent)
            : _parent(*parent)
        {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            ASSERT(parent != nullptr);
            if (parent != nullptr)
                _service = parent->GetService();
        }
        virtual ~StreamingNotification()
        {
            TRACE_L1("TVControl::StreamingNotification destructed. Line: %d", __LINE__);
        }

    public:
        // IRemoteProcess::INotification methods
        virtual void Activated(RPC::IRemoteProcess* /* process */)
        {
        }
        virtual void Deactivated(RPC::IRemoteProcess* process)
        {
            _parent.Deactivated(process);
        }

        // IStreaming::IStreamingNotification methods.
        void ScanningStateChanged(const uint32_t state)
        {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            TRACE(Trace::Information, (_T("StateChanged: { \"State\": %d }"), state));
            string message(string("{ \"ScanningStateChanged\": ") + std::to_string(state) + string(" }"));
            _service->Notify(message);
        }

        void CurrentChannelChanged(const string& lcn)
        {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            TRACE(Trace::Information, (_T("Channel changed: { \"channel\": %d }"), lcn));
            string message(string("{ \"CurrentChannelChanged\": ") + lcn + string(" }"));
            _service->Notify(message);
        }

        void TestNotification(const string& msg)
        {
            TRACE(Trace::Information, (_T("TestNotification: msg=%s"), msg.c_str()));
        }

        BEGIN_INTERFACE_MAP(StreamingNotification)
            INTERFACE_ENTRY(RPC::IRemoteProcess::INotification)
            INTERFACE_ENTRY(Exchange::IStreaming::INotification)
        END_INTERFACE_MAP

    private:
        PluginHost::IShell* _service;
        TVControl& _parent;
    };

    class GuideNotification
        : public RPC::IRemoteProcess::INotification
        , public Exchange::IGuide::INotification {
    private:
        GuideNotification() = delete;
        GuideNotification(const GuideNotification&) = delete;

    public:
        explicit GuideNotification(TVControl* parent)
            : _parent(*parent)
        {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            ASSERT(parent != nullptr);
            if (parent != nullptr)
                _service = parent->GetService();
        }
        virtual ~GuideNotification()
        {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            TRACE_L1("TVControl::Notification destructed. Line: %d", __LINE__);
        }

    public:
        // IRemoteProcess::INotification methods.
        virtual void Activated(RPC::IRemoteProcess* /* process */)
        {
        }
        virtual void Deactivated(RPC::IRemoteProcess* process)
        {
            _parent.Deactivated(process);
        }
        void EITBroadcast()
        {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            string message(string("{ \"Event\":EITBroadcast }"));
            _service->Notify(message);
        }
        void EmergencyAlert()
        {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            string message(string("{ \"Event\":EmergencyAlert }"));
            _service->Notify(message);
        }
        void ParentalControlChanged()
        {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            string message(string("{ \"Event\":ParentalControlChanged }"));
            _service->Notify(message);
        }
        void ParentalLockChanged(const string& lcn)
        {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            TRACE(Trace::Information, (_T("ParentaLockChanged: { \"channel\": %d }"), lcn));
            string message(string("{ \"ParentaLockChanged\": ") + lcn + string(" }"));
            _service->Notify(message);
        }
        void TestNotification(const string& msg)
        {
            TRACE(Trace::Information, (_T("%s: msg=%s"), __FUNCTION__, msg.c_str()));
        }

        BEGIN_INTERFACE_MAP(GuideNotification)
            INTERFACE_ENTRY(RPC::IRemoteProcess::INotification)
            INTERFACE_ENTRY(Exchange::IGuide::INotification)
        END_INTERFACE_MAP

    private:
        PluginHost::IShell* _service;
        TVControl& _parent;
    };

    class Config : public Core::JSON::Container {
    private:
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

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


public:
    class Data : public Core::JSON::Container {
    private:
        Data(const Data&) = delete;
        Data& operator=(const Data&) = delete;

    public:
        Data()
            : Core::JSON::Container()
            , ChannelId()
            , Str()
            , Channels()
            , Programs()
            , CurrentProgram()
            , AudioLanguages()
            , SubtitleLanguages()
            , IsParentalControlled()
            , OldPin()
            , NewPin()
            , Pin()
            , IsLocked()
            , IsParentalLocked()
            , IsScanning()
            , CurrentChannel()
        {
            Add(_T("ChannelId"), &ChannelId);
            Add(_T("Str"), &Str);
            Add(_T("PrimaryFreq"), &PrimaryFreq);
            Add(_T("SecondaryFreq"), &SecondaryFreq);
            Add(_T("Channels"), &Channels);
            Add(_T("Programs"), &Programs);
            Add(_T("CurrentProgram"), &CurrentProgram);
            Add(_T("AudioLanguages"), &AudioLanguages);
            Add(_T("SubtitleLanguages"), &SubtitleLanguages);
            Add(_T("IsParentalControlled"), &IsParentalControlled);
            Add(_T("EventId"), &EventId);
            Add(_T("OldPin"), &OldPin);
            Add(_T("NewPin"), &NewPin);
            Add(_T("Pin"), &Pin);
            Add(_T("IsLocked"), &IsLocked);
            Add(_T("IsParentalLocked"), &IsParentalLocked);
            Add(_T("IsScanning"), &IsScanning);
            Add(_T("CurrentChannel"), &CurrentChannel);

        }
        ~Data()
        {
        }

    public:
        Core::JSON::String ChannelId;
        Core::JSON::String Str;
        Core::JSON::DecUInt32 PrimaryFreq;
        Core::JSON::DecUInt32 SecondaryFreq;
        Core::JSON::ArrayType<Channel> Channels;
        Core::JSON::ArrayType<Program> Programs;
        Program CurrentProgram;
        Core::JSON::ArrayType<Core::JSON::String> AudioLanguages;
        Core::JSON::ArrayType<Core::JSON::String> SubtitleLanguages;
        Core::JSON::Boolean IsParentalControlled;
        Core::JSON::DecUInt32 EventId;
        Core::JSON::String OldPin;
        Core::JSON::String NewPin;
        Core::JSON::String Pin;
        Core::JSON::Boolean IsLocked;
        Core::JSON::Boolean IsParentalLocked;
        Core::JSON::Boolean IsScanning;
        Channel CurrentChannel;
    };

public:
#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
    TVControl()
        : _service(nullptr)
        , _tuner(nullptr)
        , _guide(nullptr)
        , _notification(this)
        , _streamingListener(nullptr)
        , _guideListener(nullptr)
    {
    }
#ifdef __WIN32__
#pragma warning(default : 4355)
#endif
    virtual ~TVControl()
    {
    }

    BEGIN_INTERFACE_MAP(TVControl)
        INTERFACE_ENTRY(IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_AGGREGATE(Exchange::IStreaming, _tuner)
        INTERFACE_AGGREGATE(Exchange::IGuide, _guide)
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
    virtual void Inbound(Web::Request&);
    virtual Core::ProxyType<Web::Response> Process(const Web::Request&);
    PluginHost::IShell* GetService() { return _service; }

private:
    void Deactivated(RPC::IRemoteProcess*);

private:
    uint32_t _skipURL;
    uint32_t _pid;
    PluginHost::IShell* _service;
    Exchange::IGuide* _guide;
    Exchange::IStreaming* _tuner;
    Core::Sink<StreamingNotification> _notification;
    TVControl::StreamingNotification* _streamingListener;
    TVControl::GuideNotification* _guideListener;
};
}
}

#endif // ___PLUGINTVCONTROL_H
