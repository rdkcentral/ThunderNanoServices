#ifndef __STREAMERPLUGIN_H
#define __STREAMERPLUGIN_H

#include "Module.h"
#include "Geometry.h"

namespace WPEFramework {
namespace Plugin {

class Streamer : public PluginHost::IPlugin, public PluginHost::IWeb {
private:
    Streamer(const Streamer&) = delete;
    Streamer& operator=(const Streamer&) = delete;

    typedef std::map<uint8_t, Exchange::IStream*> Streams;
    typedef std::map<uint8_t, Exchange::IStream::IControl*> Controls;

    class StreamSink : public Exchange::IStream::ICallback {
    private:
        StreamSink() = delete;
        StreamSink(const StreamSink&) = delete;
        StreamSink& operator= (const StreamSink&) = delete;
    public:
        StreamSink(Streamer* parent)
            : _parent(*parent)
        {
            ASSERT(parent != nullptr);
        }
        virtual ~StreamSink()
        {
        }
    public:
        virtual void DRM(uint32_t state)
        {
        }
        virtual void StateChange(Exchange::IStream::state state)
        {
        }

        BEGIN_INTERFACE_MAP(StreamSink)
            INTERFACE_ENTRY(Exchange::IStream::ICallback)
        END_INTERFACE_MAP

    private:
        Streamer& _parent;
    };

    class StreamControlSink : public Exchange::IStream::IControl::ICallback {
    private:
        StreamControlSink() = delete;
        StreamControlSink(const StreamControlSink&) = delete;
        StreamControlSink& operator= (const StreamControlSink&) = delete;
    public:
        StreamControlSink(Streamer* parent)
            : _parent(*parent)
        {
            ASSERT(parent != nullptr);
        }
        virtual ~StreamControlSink()
        {
        }
    public:
        virtual void TimeUpdate(uint64_t position)
        {
        }

        BEGIN_INTERFACE_MAP(StreamControlSink)
            INTERFACE_ENTRY(Exchange::IStream::IControl::ICallback)
        END_INTERFACE_MAP

    private:
        Streamer& _parent;
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

public:
    class Data : public Core::JSON::Container {
    private:
        Data(const Data&) = delete;
        Data& operator=(const Data&) = delete;

    public:
        Data()
            : Core::JSON::Container()
            , X()
            , Y()
            , Z()
            , Width()
            , Height()
            , Speed()
            , Position()
            , Type()
            , DRM()
            , State()
            , Url()
            , Begin()
            , End()
            , AbsoluteTime()
            , Id(~0)
            , Metadata(false)
            , Ids()
        {
            Add(_T("url"), &Url);
            Add(_T("x"), &X);
            Add(_T("y"), &Y);
            Add(_T("z"), &Z);
            Add(_T("width"), &Width);
            Add(_T("height"), &Height);
            Add(_T("speed"), &Speed);
            Add(_T("position"), &Position);
            Add(_T("type"), &Type);
            Add(_T("drm"), &DRM);
            Add(_T("state"), &State);
            Add(_T("begin"), &Begin);
            Add(_T("end"), &End);
            Add(_T("absolutetime"), &AbsoluteTime);
            Add(_T("id"), &Id);
            Add(_T("metadata"), &Metadata);
            Add(_T("ids"), &Ids);
        }
        ~Data()
        {
        }

    public:
        Core::JSON::DecUInt32 X;
        Core::JSON::DecUInt32 Y;
        Core::JSON::DecUInt32 Z;
        Core::JSON::DecUInt32 Width;
        Core::JSON::DecUInt32 Height;

        Core::JSON::DecSInt32 Speed;
        Core::JSON::DecUInt64 Position;
        Core::JSON::DecUInt32 Type;
        Core::JSON::DecUInt32 DRM;
        Core::JSON::DecUInt32 State;

        Core::JSON::String Url;
        Core::JSON::DecUInt64 Begin;
        Core::JSON::DecUInt64 End;
        Core::JSON::DecUInt64 AbsoluteTime;
        Core::JSON::DecUInt8 Id;

        Core::JSON::String Metadata;

        Core::JSON::ArrayType< Core::JSON::DecUInt8 > Ids;
    };

public:
#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
    Streamer()
        : _skipURL(0)
        , _pid(0)
        , _service(nullptr)
        , _player(nullptr)
        , _streams()
        , _controls()
    {
    }
#ifdef __WIN32__
#pragma warning(default : 4355)
#endif
    virtual ~Streamer()
    {
    }

public:
    BEGIN_INTERFACE_MAP(Streamer)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
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
    Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index);
    Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
    Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);
    Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index);
    void Deactivated(RPC::IRemoteProcess* process);

private:
    uint32_t _skipURL;
    uint32_t _pid;
    PluginHost::IShell* _service;

    Exchange::IPlayer* _player;

    // Stream and StreamControl holding areas for the RESTFull API.
    Streams _streams;
    Controls _controls;

};
} //namespace Plugin
} //namespace WPEFramework

#endif // __STREAMERPLUGIN_H
