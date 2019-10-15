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
        : PluginHost::JSONRPC({ 2, 3, 4 }) // version 2, 3 and 4 of the interface, use this as the default :-)
        , _job(Core::ProxyType<PeriodicSync>::Create(this))
        , _window()
        , _data()
        , _array(255)
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

		//Readonly and writeonly properties
		Property<Core::JSON::String>(_T("status"), &JSONRPCPlugin::get_status, nullptr, this);
		Property<Core::JSON::String>(_T("value"), nullptr, &JSONRPCPlugin::set_value, this);

        // Opaque method examples
        Register<JsonObject, JsonObject>("swap", &JSONRPCPlugin::swap, this);
        Property<JsonObject>(_T("window"), &JSONRPCPlugin::get_opaque_geometry, &JSONRPCPlugin::set_opaque_geometry, this);

        // Methods to test a-synchronpud callbacks
        Register<Core::JSON::DecUInt8>("async", &JSONRPCPlugin::async_callback, this);
        Register<Data::JSONDataBuffer, Core::JSON::DecUInt32>(_T("send"), &JSONRPCPlugin::send, this);
        Register<Core::JSON::DecUInt16, Data::JSONDataBuffer>(_T("receive"), &JSONRPCPlugin::receive, this);
        Register<Data::JSONDataBuffer, Data::JSONDataBuffer>(_T("exchange"), &JSONRPCPlugin::exchange, this);

		// Add property wich is indexed..
        Property<Core::JSON::DecUInt32>(_T("array"), &JSONRPCPlugin::get_array_value, &JSONRPCPlugin::set_array_value, this);
        Property<Core::JSON::DecUInt32>(_T("lookup"), &JSONRPCPlugin::get_array_value, nullptr, this);
        Property<Core::JSON::DecUInt32>(_T("store"), nullptr, &JSONRPCPlugin::set_array_value, this);

        // Methods for a "legaccy" version of the interfaces, the last parameter makes sure that all handlers are copied from the 
		// base interface to this "legacy" one...
        Core::JSONRPC::Handler& legacyVersion = JSONRPC::CreateHandler({ 1 }, *this); 

		// The only method that is really differnt in version "1" needs to be registered. That is done by the next line.
        legacyVersion.Register<Core::JSON::String, Core::JSON::String>(_T("clueless"), &JSONRPCPlugin::clueless2, this);
    }

    /* virtual */ JSONRPCPlugin::~JSONRPCPlugin()
    {
    }

    /* virtual */ const string JSONRPCPlugin::Initialize(PluginHost::IShell * service)
    {
        Config config;
        config.FromString(service->ConfigLine());zag het

	Core::NodeId source(config.Connector.Value().c_str());
	    
        Core::ProxyType<RPC::InvokeServer> engine (Core::ProxyType<RPC::InvokeServer>::Create(&Core::WorkerPool::Instance()));
        _rpcServer = new COMServer(Core::NodeId(source, source.PortNumber()), this, service->ProxyStubPath(), engine);
        _jsonServer = new JSONRPCServer<Core::JSON::IElement>(Core::NodeId(source, source.PortNumber()+1));
        _msgServer = new JSONRPCServer<Core::JSON::IMessagePack>(Core::NodeId(source, source.PortNumber()+2));
        _job->Period(5);
        PluginHost::WorkerPool::Instance().Schedule(Core::Time::Now().Add(5000), Core::ProxyType<Core::IDispatch>(_job));

        // On success return empty, to indicate there is no error text.
        return (string());
    }

    /* virtual */ void JSONRPCPlugin::Deinitialize(PluginHost::IShell * /* service */)
    {
        _job->Period(0);
        PluginHost::WorkerPool::Instance().Revoke(Core::ProxyType<Core::IDispatch>(_job));
        delete _rpcServer;
        delete _jsonServer;
	delete _msgServer;
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
        Core::Time now(Core::Time::Now());
        Core::JSON::String currentTime;

        currentTime = now.ToRFC1123();

        // PluginHost::JSONRPC method to send out a JSONRPC message to all subscribers to the event "clock".
        Notify(_T("clock"), currentTime);

		// We are currently supporting more release, the old interface is expecting a bit a different response:
        GetHandler(1)->Notify(_T("clock"), Data::Time(now.Hours(), now.Minutes(), now.Seconds()));
    }

    void JSONRPCPlugin::SendTime(Core::JSONRPC::Connection & channel)
    {
        Response(channel, Data::Response(Core::Time::Now().Ticks(), Data::Response::FAILURE));
    }

    //   Exchange::IPerformance methods
    // -------------------------------------------------------------------------------------------------------
    /* virtual */ uint32_t JSONRPCPlugin::Send(const uint16_t sendSize, const uint8_t buffer[])
    {
        uint32_t result = Core::ERROR_NONE;
        return (result);
    }
    /* virtual */ uint32_t JSONRPCPlugin::Receive(uint16_t & bufferSize, uint8_t buffer[]) const
    {
        static uint8_t pattern[] = { 0x00, 0x66, 0xBB, 0xEE };
        uint32_t result = Core::ERROR_NONE;
        uint8_t patternLength = sizeof(pattern);
        uint16_t index = 0;
        uint8_t patternIndex = 0;

        while (index < bufferSize) {

            buffer[index++] = pattern[patternIndex++];

            patternIndex %= (patternLength - 1);
        }

        return (result);
    }

    /* virtual */ uint32_t JSONRPCPlugin::Exchange(uint16_t & bufferSize, uint8_t buffer[], const uint16_t maxBufferSize)
    {
        uint32_t result = Core::ERROR_NONE;
        static uint8_t pattern[] = { 0x00, 0x77, 0xCC, 0x88 };
        uint8_t patternLength = sizeof(pattern);
        uint16_t index = 0;
        uint8_t patternIndex = 0;

        while (index < maxBufferSize) {

            buffer[index++] = pattern[patternIndex++];

            patternIndex %= (patternLength - 1);
        }

        return (result);
    }

} // namespace Plugin

} // namespace WPEFramework
