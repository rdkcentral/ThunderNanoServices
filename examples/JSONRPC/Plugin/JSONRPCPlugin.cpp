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
 
#include "JSONRPCPlugin.h"

 // From the source, included above (IMath.h) a precompiler will create a method that 
 // can connect a JSONRPC class to the interface implementation. This is typically done 
 // in the constructor of the plugin!
 // The method to connects the JSORPC implementation to the Interface implementation is 
 // generated and it is named as:
 // <interface name>::Register(PluginHost::JSONRPC& handler, <interface name>* implementation);
 // <interface name>::Unregister(PluginHost::JSONRPC& handler);
#include <interfaces/json/JMath.h>


namespace Thunder {

ENUM_CONVERSION_BEGIN(Data::Response::state)

    { Data::Response::ACTIVE, _TXT("Activate") },
    { Data::Response::INACTIVE, _TXT("Inactivate") },
    { Data::Response::IDLE, _TXT("Idle") },
    { Data::Response::FAILURE, _TXT("Failure") },

ENUM_CONVERSION_END(Data::Response::state)

namespace Plugin {

    namespace {

        static Metadata<JSONRPCPlugin> metadata(
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

    PluginHost::JSONRPC::classification JSONRPCPlugin::Validation(const string& token VARIABLE_IS_NOT_USED, const string& method, const string& parameters VARIABLE_IS_NOT_USED) {
        return (method != _T("checkvalidation") ? PluginHost::JSONRPC::classification::VALID : PluginHost::JSONRPC::classification::INVALID);
    }

    PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
    JSONRPCPlugin::JSONRPCPlugin()
        : PluginHost::JSONRPC({ 2, 3, 4 }, [&](const string& token, const string& method, const string& parameters) -> PluginHost::JSONRPC::classification { return (Validation(token, method, parameters)); }) // version 2, 3 and 4 of the interface, use this as the default :-)
        , _window()
        , _data()
        , _array(255)
        , _rpcServer(nullptr)
        , _jsonServer(nullptr)
        , _msgServer(nullptr)
        , _callback(this)
        , _job(*this)
    {
        RegisterAll();
    }
    POP_WARNING()
    /* virtual */ JSONRPCPlugin::~JSONRPCPlugin()
    {
        UnregisterAll();
    }

    void JSONRPCPlugin::RegisterAll()
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

        // Readonly and writeonly properties
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

        // PluginHost::JSONRPC method to register a JSONRPC method invocation that will not be alowed to execute as it  will not pass validation.
        Register<void, Core::JSON::String>(_T("checkvalidation"), &JSONRPCPlugin::checkvalidation, this);

        // Methods for a "legaccy" version of the interfaces, the last parameter makes sure that all handlers are copied from the 
        // base interface to this "legacy" one...
        Core::JSONRPC::Handler& legacyVersion = JSONRPC::CreateHandler({ 1 }, *this);

        // The only method that is really different in version "1" needs to be registered and replace the other version one. That is done by the next line.
        legacyVersion.Register<Core::JSON::String, Core::JSON::String>(_T("clueless"), &JSONRPCPlugin::clueless2, this);

        // Here the glue logic to combine the COMRPC interface with the JSONRPC interface is generated by the code generators.
        // The only thing that needs to be done is attach the COMRPC interface to the JSONRPC glue logic.
        Exchange::JMath::Register(*this, this);
    }
    void JSONRPCPlugin::UnregisterAll()
    {
        // As we connected the glue logic to combine the COMRPC interface with the JSONRPC interface it is good practise to also decouple it.
        // if we are done. The next line unlinks the JSONRPC glue logic from the actual implementation found for the IMath interface.
        Exchange::JMath::Unregister(*this);

        Unregister(_T("time"));
        Unregister(_T("clueless"));
        Unregister(_T("input"));
        Unregister(_T("extended"));
        Unregister(_T("postmessage"));
        Unregister(_T("geometry"));
        Unregister(_T("data"));
        Unregister(_T("status"));
        Unregister(_T("value"));
        Unregister(_T("swap"));
        Unregister(_T("window"));
        Unregister(_T("async"));
        Unregister(_T("send"));
        Unregister(_T("receive"));
        Unregister(_T("exchange"));
        Unregister(_T("array"));
        Unregister(_T("lookup"));
        Unregister(_T("store"));
        Unregister(_T("checkvalidation"));

        Core::JSONRPC::Handler* legacyVersion = JSONRPC::GetHandler(1);

        ASSERT (legacyVersion != nullptr);

        legacyVersion->Unregister(_T("clueless"));
    }

    /* virtual */ const string JSONRPCPlugin::Initialize(PluginHost::IShell * service)
    {
        Config config;
        config.FromString(service->ConfigLine());

        Core::NodeId source(config.Connector.Value().c_str());
        Core::ProxyType<RPC::InvokeServer> engine (Core::ProxyType<RPC::InvokeServer>::Create(&Core::IWorkerPool::Instance()));
        _rpcServer = new COMServer(Core::NodeId(source, source.PortNumber()), this, service->ProxyStubPath(), engine);
        _jsonServer = new JSONRPCChannel<Core::JSON::IElement>(Core::NodeId(source, source.PortNumber() + 1), *this);
        _msgServer = new JSONRPCChannel<Core::JSON::IMessagePack>(Core::NodeId(source, source.PortNumber() + 2), *this);
        Period(5);
        _job.Reschedule(Core::Time::Now().Add(5000));

        // On success return empty, to indicate there is no error text.
        return (string());
    }

    /* virtual */ void JSONRPCPlugin::Deinitialize(PluginHost::IShell * /* service */)
    {
        Period(0);
        _job.Revoke();
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
        Notify(_T("clock"), Data::Time(now.Hours(), now.Minutes(), now.Seconds()));
    }

    void JSONRPCPlugin::SendTime(const Core::JSONRPC::Context& channel)
    {
        Core::ProxyType<Web::JSONRPC::Body> message = PluginHost::IFactories::Instance().JSONRPC();
        Data::Response data (Core::Time::Now().Ticks(), Data::Response::FAILURE);
        string dataJSON; data.ToString(dataJSON);

        message->Result = dataJSON;
        message->Id = channel.Sequence();

        JSONRPC::Response(channel.ChannelId(), Core::ProxyType<Core::JSON::IElement>(message));
    }

    //   Exchange::IPerformance methods
    // -------------------------------------------------------------------------------------------------------
    uint32_t JSONRPCPlugin::Send(const uint16_t sendSize, const uint8_t buffer[] VARIABLE_IS_NOT_USED) /* override */
    {
        uint32_t result = sendSize;
        return (result);
    }
    uint32_t JSONRPCPlugin::Receive(uint16_t & bufferSize, uint8_t buffer[]) const /* override */ 
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
    uint32_t JSONRPCPlugin::Exchange(uint16_t & bufferSize VARIABLE_IS_NOT_USED, uint8_t buffer[], const uint16_t maxBufferSize) /* override */
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

    uint32_t JSONRPCPlugin::Add(const uint16_t A, const uint16_t B, uint16_t& sum /* @out */)  const /* override */ {
        sum = A + B;
        return (Core::ERROR_NONE);
    }
    uint32_t JSONRPCPlugin::Sub(const uint16_t A, const uint16_t B, uint16_t& sum /* @out */)  const /* override */ {
        sum = A - B;
        return (Core::ERROR_NONE);
    }
} // namespace Plugin

} // namespace Thunder
