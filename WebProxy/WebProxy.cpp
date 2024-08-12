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
 
#include "WebProxy.h"

namespace Thunder {

ENUM_CONVERSION_BEGIN(Plugin::WebProxy::Config::Link::enumType){ Plugin::WebProxy::Config::Link::UDP, _TXT("udp") },
    { Plugin::WebProxy::Config::Link::TCP, _TXT("tcp") },
    { Plugin::WebProxy::Config::Link::SERIAL, _TXT("serial") },
ENUM_CONVERSION_END(Plugin::WebProxy::Config::Link::enumType);

ENUM_CONVERSION_BEGIN(Core::SerialPort::Parity){ Core::SerialPort::EVEN, _TXT("even") },
    { Core::SerialPort::ODD, _TXT("odd") },
    { Core::SerialPort::NONE, _TXT("none") },
ENUM_CONVERSION_END(Core::SerialPort::Parity);

namespace Plugin {

    namespace {

        static Metadata<WebProxy> metadata(
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

    class StreamChannel : public Core::StreamType<Core::SocketStream> {
    public:
        StreamChannel() = delete;
        StreamChannel(StreamChannel&&) = delete;
        StreamChannel(const StreamChannel&) = delete;
        StreamChannel& operator=(const StreamChannel&) = delete;

        StreamChannel(WebProxy::Connector& parent, const uint32_t bufferSize, const Core::NodeId& remote)
            : Core::StreamType<Core::SocketStream>(false, remote.AnyInterface(), remote, bufferSize, bufferSize)
            , _parent(parent)
        {
        }
        ~StreamChannel() override = default;

    public:
        uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
        {
            return (_parent.SendData(dataFrame, maxSendSize));
        }
        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override
        {
            return (_parent.ReceiveData(dataFrame, receivedSize));
        }
        void StateChange() override
        {
            return (_parent.StateChange());
        }

    private:
        WebProxy::Connector& _parent;
    };
    class DatagramChannel : public Core::StreamType<Core::SocketDatagram> {
    private:
        using BaseClass = Core::StreamType<Core::SocketDatagram>;

    public:
        DatagramChannel() = delete;
        DatagramChannel(DatagramChannel&&) = delete;
        DatagramChannel(const DatagramChannel&) = delete;
        DatagramChannel& operator=(const DatagramChannel&) = delete;

        DatagramChannel(WebProxy::Connector& parent, const uint32_t bufferSize, const Core::NodeId& remote)
            : Core::StreamType<Core::SocketDatagram>(false, remote.AnyInterface(), remote, bufferSize, bufferSize)
            , _parent(parent)
        {
        }
        ~DatagramChannel() override = default;

    public:
        uint32_t Open(const uint32_t waitTime) override
        {
            uint32_t result = BaseClass::Open(waitTime);

            if (BaseClass::RemoteNode().IsMulticast() == true) {
                BaseClass::Join(BaseClass::RemoteNode());
            }

            return (result);
        }
        uint32_t Close(const uint32_t waitTime) override
        {
            if (BaseClass::RemoteNode().IsMulticast() == true) {
                BaseClass::Leave(BaseClass::RemoteNode());
            }

            return (BaseClass::Close(waitTime));
        }
        uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
        {
            return (_parent.SendData(dataFrame, maxSendSize));
        }
        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override
        {
            return (_parent.ReceiveData(dataFrame, receivedSize));
        }
        void StateChange() override
        {
            return (_parent.StateChange());
        }

    private:
        WebProxy::Connector& _parent;
    };
    class DeviceChannel : public Core::StreamType<Core::SerialPort> {
    public:
        DeviceChannel() = delete;
        DeviceChannel(DeviceChannel&&) = delete;
        DeviceChannel(const DeviceChannel&) = delete;
        DeviceChannel& operator=(const DeviceChannel&) = delete;

        DeviceChannel(
            WebProxy::Connector& parent,
            const uint32_t bufferSize,
            const string& deviceName,
            const Core::SerialPort::BaudRate baudrate,
            const Core::SerialPort::Parity parityE,
            const Core::SerialPort::DataBits dataBits,
            const Core::SerialPort::StopBits stopBits,
            const Core::SerialPort::FlowControl flowControl)
            : Core::StreamType<Core::SerialPort>(deviceName, baudrate, parityE, dataBits, stopBits, flowControl, bufferSize, bufferSize)
            , _parent(parent)
        {
        }
        ~DeviceChannel() override = default;

    public:
        uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
        {
            return (_parent.SendData(dataFrame, maxSendSize));
        }
        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override
        {
            return (_parent.ReceiveData(dataFrame, receivedSize));
        }
        void StateChange() override
        {
            return (_parent.StateChange());
        }

    private:
        WebProxy::Connector& _parent;
    };

    const string WebProxy::Initialize(PluginHost::IShell* service) /* override */ 
    {
        WebProxy::Config config;
        config.FromString(service->ConfigLine());

        _maxConnections = config.Connections.Value();

        // Copy all predefined links...
        if ((config.Links.IsSet() == true) && (config.Links.Length() != 0)) {
            // Build the device info array..
            Core::JSON::ArrayType<Config::Link>::Iterator index(config.Links.Elements());

            while (index.Next() == true) {
                if (index.Current().Name.IsSet() == true) {
                    _linkInfo.emplace(
                        std::piecewise_construct,
                            std::forward_as_tuple(index.Current().Name.Value()),
                            std::forward_as_tuple(index.Current()));
                }
            }
        }

        service->EnableWebServer(_T("UI"), EMPTY_STRING);

        // On succes return nullptr, to indicate there is no error text.
        return (_T(""));
    }

    void WebProxy::Deinitialize(PluginHost::IShell* service) /* override */
    {
        service->DisableWebServer();

        for (std::pair<const uint32_t, Connector*>& connection : _connectionMap) {
            connection.second->Detach();
            delete connection.second;
        }
        _connectionMap.clear();
    }

    // Whenever a Channel (WebSocket connection) is created to the plugin that will be reported via the Attach.
    // Whenever the channel is closed, it is reported via the detach method.
    bool WebProxy::Attach(PluginHost::Channel& channel) /* override */
    {
        bool added = false;
        Core::NodeId nodeId;

        // First do a cleanup of all "completely" closed channels.
        Connectors::iterator connection(_connectionMap.begin());

        while (connection != _connectionMap.end()) {
            if (connection->second->IsClosed() == true) {
                delete connection->second;
                connection = _connectionMap.erase(connection);
            } else {
                connection++;
            }
        }

        // See if we are still allowed to create a new connection..
        if (_connectionMap.size() < _maxConnections) {
            Connector* newLink = CreateConnector(channel);

            if (newLink != nullptr) {
                _connectionMap.emplace(
                    std::piecewise_construct,
                        std::forward_as_tuple(channel.Id()),
                        std::forward_as_tuple(newLink));

                TRACE(Trace::Information, (Core::Format(_T("Proxy connection channel ID [%d] to %s"), channel.Id(), newLink->RemoteId().c_str()).c_str()));
                added = true;

                newLink->Attach();
            }
        }

        return (added);
    }

    void WebProxy::Detach(PluginHost::Channel& channel) /* override */
    {
        // See if we can forward this info..
        Connectors::iterator connection = _connectionMap.find(channel.Id());

        if (connection != _connectionMap.end()) {
            connection->second->Detach();
        }
    }

    string WebProxy::Information() const /* override */
    {
        // No additional info to report.
        return (string());
    }

    // IChannel methods
    // -------------------------------------------------------------------------------------------------------
    // Whenever a WebSocket is opened with a locator (URL) pointing to this plugin, it is capable of receiving
    // raw data for the plugin. Raw data received on this link will be exposed to the plugin via this interface.
    uint32_t WebProxy::Inbound(const uint32_t ID, const uint8_t data[], const uint16_t length) /* override */
    {
        uint32_t result = length;

        // See if we can forward this info..
        Connectors::iterator connection = _connectionMap.find(ID);

        if (connection != _connectionMap.end()) {
            result = connection->second->ChannelReceive(data, length);
        }

        return (result);
    }

    // Whenever a WebSocket is opened with a locator (URL) pointing to this plugin, it is capable of sending
    // raw data to the initiator of the websocket.
    uint32_t WebProxy::Outbound(const uint32_t ID, uint8_t data[], const uint16_t length) const /* override */
    {
        uint32_t result = 0;

        // See if we can forward this info..
        Connectors::const_iterator connection = _connectionMap.find(ID);

        if (connection != _connectionMap.end()) {
            result = connection->second->ChannelSend(data, length);
        }

        return (result);
    }

    WebProxy::Connector* WebProxy::CreateConnector(PluginHost::Channel& channel) const
    {
        Core::TextFragment host;
        Core::TextFragment device;
        WebProxy::Connector* result(nullptr);
        Core::SerialPort::BaudRate baudRate(Core::SerialPort::BaudRate::BAUDRATE_115200);
        Core::SerialPort::Parity parity(Core::SerialPort::NONE);
        Core::SerialPort::DataBits dataBits(Core::SerialPort::DataBits::BITS_8);
        Core::SerialPort::StopBits stopBits(Core::SerialPort::StopBits::BITS_1);
        Core::SerialPort::FlowControl flowControl(Core::SerialPort::FlowControl::OFF);
        bool datagram(false);
        bool text(false);
        const string& options(channel.Query());

        if (options.empty() == false) {
            Core::URL::KeyValue keys(options);

            text     = (keys.HasKey(_T("Text"), true) == Core::URL::KeyValue::status::KEY_ONLY);
            datagram = (keys.HasKey(_T("datagram"), true) == Core::URL::KeyValue::status::KEY_ONLY);
            if (keys.HasKey(_T("host"), true) == Core::URL::KeyValue::status::KEY_VALUE) {
                host = keys.Value(_T("host"), true);
            }
            if (keys.HasKey(_T("device"), true) == Core::URL::KeyValue::status::KEY_VALUE) {
                device = keys.Value(_T("device"), true);
            }
        }
        else if (channel.Path().empty() == false) {
            // See of this name is registered ?
            Links::const_iterator index(_linkInfo.find(channel.Path()));

            if (index != _linkInfo.end()) {
                const Config::Link& linkInfo(index->second);

                // Seems like we have a valid entry, get the config.
                text = ((linkInfo.Text.IsSet() == true) && (linkInfo.Text.Value() == true));
                host = Core::TextFragment(linkInfo.Host.Value());
                device = Core::TextFragment(linkInfo.Device.Value());
                datagram = ((linkInfo.Type.IsSet() == true) && (linkInfo.Type.Value() == Config::Link::UDP));

                if (linkInfo.Configuration.IsSet() == true) {
                    const Config::Link::Settings& configInfo(linkInfo.Configuration);

                    parity = (configInfo.Parity.Value());
                    stopBits = (configInfo.Stop.Value() == 2 ? Core::SerialPort::StopBits::BITS_2 : Core::SerialPort::StopBits::BITS_1);
                    baudRate = (configInfo.Baudrate.Value() == 110    ? Core::SerialPort::BaudRate::BAUDRATE_110    : 
                                configInfo.Baudrate.Value() == 300    ? Core::SerialPort::BaudRate::BAUDRATE_300    : 
                                configInfo.Baudrate.Value() == 600    ? Core::SerialPort::BaudRate::BAUDRATE_600    : 
                                configInfo.Baudrate.Value() == 1200   ? Core::SerialPort::BaudRate::BAUDRATE_1200   : 
                                configInfo.Baudrate.Value() == 2400   ? Core::SerialPort::BaudRate::BAUDRATE_2400   : 
                                configInfo.Baudrate.Value() == 4800   ? Core::SerialPort::BaudRate::BAUDRATE_4800   : 
                                configInfo.Baudrate.Value() == 9600   ? Core::SerialPort::BaudRate::BAUDRATE_9600   : 
                                configInfo.Baudrate.Value() == 19200  ? Core::SerialPort::BaudRate::BAUDRATE_19200  : 
                                configInfo.Baudrate.Value() == 38400  ? Core::SerialPort::BaudRate::BAUDRATE_38400  : 
                                configInfo.Baudrate.Value() == 57600  ? Core::SerialPort::BaudRate::BAUDRATE_57600  : 
                                configInfo.Baudrate.Value() == 115200 ? Core::SerialPort::BaudRate::BAUDRATE_115200 : 
                                                                    Core::SerialPort::BaudRate::BAUDRATE_9600);
                    dataBits = (configInfo.Data.Value() == 5 ? Core::SerialPort::DataBits::BITS_5 : 
                                configInfo.Data.Value() == 6 ? Core::SerialPort::DataBits::BITS_6 : 
                                configInfo.Data.Value() == 7 ? Core::SerialPort::DataBits::BITS_7 : 
                                Core::SerialPort::DataBits::BITS_8);
                }
            }
        }

        if ((host.Length() > 0) && (device.Length() == 0)) {
            Core::NodeId remote(host.Text().c_str());

            if (datagram == true) {
                result = new ConnectorType<DatagramChannel>(channel, 1024, remote);
            } else {
                result = new ConnectorType<StreamChannel>(channel, 1024, remote);
            }
        } else if ((device.Length() > 0) && (host.Length() == 0)) {
            result = new ConnectorType<DeviceChannel>(channel, 1024, device.Text(), baudRate, parity, dataBits, stopBits, flowControl);
        }

        if ((result != nullptr) && (text == true)) {
            channel.Binary(false);
        }

        return (result);
    }
}
}
