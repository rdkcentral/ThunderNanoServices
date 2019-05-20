#include "JSONRPCPlugin.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Data::Response::state)

    { Data::Response::ACTIVE, _TXT("Activate") },
    { Data::Response::INACTIVE, _TXT("Inactivate") },
    { Data::Response::IDLE, _TXT("Idle") },
    { Data::Response::FAILURE, _TXT("Failure") },

ENUM_CONVERSION_END(Data::Response::state)

namespace Plugin
{

    SERVICE_REGISTRATION(JSONRPCPlugin, 1, 0);

    JSONRPCPlugin::JSONRPCPlugin()
        : PluginHost::JSONRPC()
        , _job(Core::ProxyType<PeriodicSync>::Create(this))
        , _window()
        , _data()
        , _rpcServer(nullptr)
    {
        // PluginHost::JSONRPC method to register a JSONRPC method invocation for the method "time".
        Register<void, Core::JSON::String>(_T("time"), &JSONRPCPlugin::time, this);
        Register<void, void>(_T("clueless"), &JSONRPCPlugin::clueless, this);
        Register<Core::JSON::String, void>(_T("input"), &JSONRPCPlugin::input, this);
        Register<Data::Parameters, Data::Response>(_T("extended"), &JSONRPCPlugin::extended, this);
        Register<Data::MessageParameters, void>(_T("postmessage"), &JSONRPCPlugin::postmessage, this);

        // PluginHost::JSONRPC method to register a JSONRPC method invocation for the method "time".
        Property<Data::Geometry>(_T("geometry"), &JSONRPCPlugin::get_geometry, &JSONRPCPlugin::set_geometry, this);
        Property<Core::JSON::String>(_T("data"), &JSONRPCPlugin::get_data, &JSONRPCPlugin::set_data, this);

        // Opaque method examples
        Register<JsonObject, JsonObject>("swap", &JSONRPCPlugin::swap, this);
        Property<JsonObject>(_T("window"), &JSONRPCPlugin::get_opaque_geometry, &JSONRPCPlugin::set_opaque_geometry, this);

        // Methods to test a-synchronpud callbacks
        Register<Core::JSON::DecUInt8>("async", &JSONRPCPlugin::async_callback, this);

        Register<Data::JSONDataBuffer, Core::JSON::DecUInt32>(_T("send"), &JSONRPCPlugin::send, this);
        Register<Core::JSON::DecUInt16, Data::JSONDataBuffer>(_T("receive"), &JSONRPCPlugin::receive, this);
        Register<Data::JSONDataBuffer, Data::JSONDataBuffer>(_T("exchange"), &JSONRPCPlugin::exchange, this);
    }

    /* virtual */ JSONRPCPlugin::~JSONRPCPlugin()
    {
    }

    /* virtual */ const string JSONRPCPlugin::Initialize(PluginHost::IShell* service)
    {
        Config config;
        config.FromString(service->ConfigLine());

		_rpcServer = new COMServer(Core::NodeId(config.Connector.Value().c_str()), this, service->ProxyStubPath());

        _job->Period(5);
        PluginHost::WorkerPool::Instance().Schedule(Core::Time::Now().Add(5000), _job);

        // On success return empty, to indicate there is no error text.
        return (string());
    }

    /* virtual */ void JSONRPCPlugin::Deinitialize(PluginHost::IShell * /* service */)
    {
        _job->Period(0);
        PluginHost::WorkerPool::Instance().Revoke(_job);
        delete _rpcServer;
	}

    /* virtual */ string JSONRPCPlugin::Information() const
    {
        // No additional info to report.
        return (string());
    }

    void JSONRPCPlugin::PostMessage(const string& recipient, const string& message)
    {
        // PluginHost::JSONRPC method to send out a JSONRPC message to all subscribers to the event "message".
        Notify(_T("message"), Core::JSON::String(message), [&](const string& designator) -> bool {
            bool sendmessage(true);
            if (recipient != "all") {
                size_t pos = designator.find('.');
                string client(designator.substr(0, pos)); // note also works if no . found
                sendmessage = client == recipient;
            }
            return sendmessage;
        });
    }

    void JSONRPCPlugin::SendTime()
    {
        Core::JSON::String currentTime;
        
		currentTime = Core::Time::Now().ToRFC1123();

        // PluginHost::JSONRPC method to send out a JSONRPC message to all subscribers to the event "clock".
        Notify(_T("clock"), currentTime);
    }

    void JSONRPCPlugin::SendTime(Core::JSONRPC::Connection & channel)
    {
        Response(channel, Data::Response(Core::Time::Now().Ticks(), Data::Response::FAILURE));
    }

    //   Exchange::IPerformance methods
    // -------------------------------------------------------------------------------------------------------
    /* virtual */ uint32_t JSONRPCPlugin::Send(const uint16_t sendSize, const uint8_t buffer[])
    {
        printf("Received a send for size: %d\n", sendSize);
        uint32_t result = 0;
        return (result);
	}

	/* virtual */ uint32_t JSONRPCPlugin::Receive(uint16_t & bufferSize, uint8_t buffer[]) const 
	{
        uint32_t result = 0;
        return (result);
    }

    /* virtual */ uint32_t JSONRPCPlugin::Exchange(uint16_t & bufferSize, uint8_t buffer[], const uint16_t maxBufferSize)
    {
        uint32_t result = 0;
        return (result);
    }

} // namespace Plugin

} // namespace WPEFramework
