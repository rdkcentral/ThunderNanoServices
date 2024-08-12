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
 
#pragma once

#include "Module.h"

namespace Thunder {

namespace Plugin {

    class WebProxy 
        : public PluginHost::IPluginExtended
        , public PluginHost::IChannel {
    public:
        class Connector {
        private:
            class EXTERNAL CyclicDataBuffer {
            public:
                CyclicDataBuffer() = delete;
                CyclicDataBuffer(CyclicDataBuffer&&) = delete;
                CyclicDataBuffer(const CyclicDataBuffer&) = delete;
                CyclicDataBuffer& operator=(const CyclicDataBuffer&) = delete;

                CyclicDataBuffer(const uint32_t size, uint8_t buffer[])
                    : _head(0)
                    , _tail(0)
                    , _size(size)
                    , _buffer(buffer) {
                }
                ~CyclicDataBuffer() = default;

            public:
                inline uint32_t Filled() const
                {
                    return ((_head >= _tail) ? (_head - _tail) : _size - (_tail - _head));
                }
                inline uint32_t Free() const
                {
                    return ((_head >= _tail) ? (_size - (_head - _tail)) : (_tail - _head));
                }
                inline bool IsEmpty() const
                {
                    return (_head == _tail);
                }
                uint16_t Read(uint8_t* dataFrame, const uint16_t maxSendSize) const
                {
                    uint16_t result = 0;

                    if (_head != _tail) {
                        if (_tail > _head) {
                            result = ((_size - _tail) < maxSendSize ? (_size - _tail) : maxSendSize);
                            ::memcpy(dataFrame, &(_buffer[_tail]), result);
                            _tail = (_size == (_tail + result) ? 0 : (_tail + result));
                        }

                        if ((_tail < _head) && (result < maxSendSize)) {
                            uint16_t copySize = (static_cast<uint16_t>(_head - _tail) < static_cast<uint16_t>(maxSendSize - result) ? (_head - _tail) : (maxSendSize - result));
                            ::memcpy(&dataFrame[result], &(_buffer[_tail]), copySize);
                            _tail += copySize;
                            result += copySize;
                        }
                    }

                    return (result);
                }
                uint16_t Write(const uint8_t* dataFrame, const uint16_t receivedSize)
                {
                    ASSERT(receivedSize < _size);
                    uint32_t freeBuffer = Free();
                    uint32_t result = ((receivedSize + _head) > _size ? (_size - _head) : receivedSize);

                    ::memcpy(&(_buffer[_head]), dataFrame, result);
                    _head = ((_head + result) < _size ? (_head + result) : 0);

                    while (result < receivedSize) {
                        // we continue at the beginning.
                        uint32_t copySize = ((receivedSize - result) > static_cast<uint16_t>(_size) ? _size : (receivedSize - result));

                        ::memcpy(_buffer, &(dataFrame[result]), copySize);
                        _head = ((_head + copySize) < _size ? (_head + copySize) : 0);
                        result += copySize;
                    }

                    if (freeBuffer < receivedSize) {
                        // We have an override, adapt the tail.
                        _tail = ((_head + 1) < static_cast<uint16_t>(_size) ? (_head + 1) : 0);
                    }

                    return (result);
                }

            private:
                uint32_t _head;
                mutable uint32_t _tail;
                const uint32_t _size;
                uint8_t* _buffer;
            };

        public:
            Connector(Connector&&) = delete;
            Connector(const Connector&) = delete;
            Connector& operator=(const Connector&) = delete;

            Connector(PluginHost::Channel& channel, Core::IStream* link)
                : _link(link)
                , _channel(&channel)
                , _adminLock()
                , _buffer1()
                , _buffer2()
                , _channelBuffer(_buffer1.Size(), _buffer1.Buffer())
                , _socketBuffer(_buffer2.Size(), _buffer2.Buffer())
            {
            }
            virtual ~Connector() = default;

        public:
            uint32_t Id() const
            {
                uint32_t result = 0;
                _adminLock.Lock();

                if (_channel != nullptr) {
                    result = _channel->Id();
                }

                _adminLock.Unlock();

                return (result);
            }
            string RemoteId() const
            {
                return (_link->RemoteId());
            }
            bool IsClosed() const
            {
                return ((_channel == nullptr) && (_link->IsClosed()));
            }
            // Methods to extract and insert data into the socket buffers
            uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
            {
                _adminLock.Lock();

                uint16_t result = _socketBuffer.Read(dataFrame, maxSendSize);

                _adminLock.Unlock();

                return (result);
            }
            uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
            {
                _adminLock.Lock();

                bool wasEmpty = _channelBuffer.IsEmpty();

                uint16_t result = _channelBuffer.Write(dataFrame, receivedSize);

                if (wasEmpty == true) {
                    // This is new data, there was nothing pending, trigger a request for a frambuffer.
                    _channel->RequestOutbound();
                }

                _adminLock.Unlock();

                return (result);
            }
            uint16_t ChannelSend(uint8_t* dataFrame, const uint16_t maxSendSize) const
            {
                _adminLock.Lock();

                uint16_t result = _channelBuffer.Read(dataFrame, maxSendSize);

                _adminLock.Unlock();

                return (result);
            }
            uint16_t ChannelReceive(const uint8_t* dataFrame, const uint16_t receivedSize)
            {
                _adminLock.Lock();

                bool wasEmpty = _socketBuffer.IsEmpty();

                uint16_t result = _socketBuffer.Write(dataFrame, receivedSize);

                if (wasEmpty == true) {
                    // This is new data, there was nothing pending, trigger a request for a frambuffer.
                    _link->Trigger();
                }

                _adminLock.Unlock();

                return (result);
            }
            // Signal a state change, Opened, Closed or Accepted
            void StateChange()
            {
                if (_link->IsOpen() == true) {
                    TRACE(Trace::Information, (_T("Proxy connection for channel ID [%d] is Open"), Id()));
                } else if (IsClosed() == true) {
                    TRACE(Trace::Information, (_T("Proxy connection for channel ID [%d] is Closed"), Id()));
                } else {
                    TRACE(Trace::Information, (_T("Proxy connection for channel ID [%d] has reached an exceptional state"), Id()));
                }
            }
            void Attach()
            {
                _adminLock.Lock();
                _link->Open(0);
                _adminLock.Unlock();
            }
            void Detach()
            {
                _adminLock.Lock();
                _channel = nullptr;
                _link->Close(0);
                _adminLock.Unlock();
            }

        private:
            Core::IStream* _link;
            PluginHost::Channel* _channel;
            mutable Core::CriticalSection _adminLock;
            Core::ScopedStorage<8192> _buffer1;
            Core::ScopedStorage<8192> _buffer2;
            CyclicDataBuffer _channelBuffer;
            CyclicDataBuffer _socketBuffer;
        };
        class Config : public Core::JSON::Container {
        public:
            class Link : public Core::JSON::Container {
            public:
                enum enumType {
                    TCP,
                    UDP,
                    SERIAL
                };
                class Settings : public Core::JSON::Container {
                public:
                    Settings()
                        : Core::JSON::Container()
                    {
                        Add(_T("baudrate"), &Baudrate);
                        Add(_T("parity"), &Parity);
                        Add(_T("data"), &Data);
                        Add(_T("stop"), &Stop);
                    }
                    Settings(const uint32_t baudRate, const Core::SerialPort::Parity parity, const uint8_t bits, const uint8_t stopbits)
                        : Core::JSON::Container()
                    {
                        Add(_T("baudrate"), &Baudrate);
                        Add(_T("parity"), &Parity);
                        Add(_T("data"), &Data);
                        Add(_T("stop"), &Stop);

                        Baudrate = baudRate;
                        Parity = parity;
                        Data = bits;
                        Stop = stopbits;
                    }
                    Settings(const Settings& copy)
                        : Core::JSON::Container()
                        , Baudrate(copy.Baudrate)
                        , Parity(copy.Parity)
                        , Data(copy.Data)
                        , Stop(copy.Stop)
                    {
                        Add(_T("baudrate"), &Baudrate);
                        Add(_T("parity"), &Parity);
                        Add(_T("data"), &Data);
                        Add(_T("stop"), &Stop);
                    }
                    ~Settings() override = default;

                    Settings& operator=(const Settings& rhs)
                    {
                        Baudrate = rhs.Baudrate;
                        Parity = rhs.Parity;
                        Data = rhs.Data;
                        Stop = rhs.Stop;

                        return (*this);
                    }

                public:
                    Core::JSON::DecUInt32 Baudrate;
                    Core::JSON::EnumType<Core::SerialPort::Parity> Parity;
                    Core::JSON::DecUInt8 Data;
                    Core::JSON::DecUInt8 Stop;
                };

            public:
                Link()
                    : Core::JSON::Container()
                {
                    Add(_T("name"), &Name);
                    Add(_T("type"), &Type);
                    Add(_T("text"), &Text);
                    Add(_T("host"), &Host);
                    Add(_T("device"), &Device);
                    Add(_T("configuration"), &Configuration);
                }
                Link(const string& name, const enumType type, const bool text, const string host)
                    : Core::JSON::Container()
                {
                    Add(_T("name"), &Name);
                    Add(_T("type"), &Type);
                    Add(_T("text"), &Text);
                    Add(_T("host"), &Host);
                    Add(_T("device"), &Device);
                    Add(_T("configuration"), &Configuration);

                    Name = name;
                    Type = type;
                    Host = host;

                    if (text == true) {
                        Text = text;
                    }
                }
                Link(const string& name, const enumType type, const bool text, const string device, const uint32_t baudRate, const Core::SerialPort::Parity parity, const uint8_t bits, const uint8_t stopbits)
                    : Core::JSON::Container()
                {
                    Add(_T("name"), &Name);
                    Add(_T("type"), &Type);
                    Add(_T("text"), &Text);
                    Add(_T("host"), &Host);
                    Add(_T("device"), &Device);
                    Add(_T("configuration"), &Configuration);

                    Name = name;
                    Type = type;
                    Device = device;
                    Configuration = Settings(baudRate, parity, bits, stopbits);

                    if (text == true) {
                        Text = text;
                    }
                }
                Link(const Link& copy)
                    : Core::JSON::Container()
                    , Name(copy.Name)
                    , Type(copy.Type)
                    , Text(copy.Text)
                    , Host(copy.Host)
                    , Device(copy.Device)
                    , Configuration(copy.Configuration)
                {
                    Add(_T("name"), &Name);
                    Add(_T("type"), &Type);
                    Add(_T("text"), &Text);
                    Add(_T("host"), &Host);
                    Add(_T("device"), &Device);
                    Add(_T("configuration"), &Configuration);
                }
                ~Link() override = default;

                Link& operator=(const Link&) = delete;

            public:
                Core::JSON::String Name;
                Core::JSON::EnumType<enumType> Type;
                Core::JSON::Boolean Text;
                Core::JSON::String Host;
                Core::JSON::String Device;
                Settings Configuration;
            };

        public:
            Config(Config&&) = delete;
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
                , Connections(10)
            {
                Add(_T("connections"), &Connections);
                Add(_T("links"), &Links);
            }
            ~Config() override = default;

        public:
            Core::JSON::DecUInt16 Connections;
            Core::JSON::ArrayType<Link> Links;
        };

        template <typename STREAMTYPE>
        class ConnectorType : public WebProxy::Connector {
        public:
            ConnectorType() = delete;
            ConnectorType(ConnectorType<STREAMTYPE>&&) = delete;
            ConnectorType(const ConnectorType<STREAMTYPE>&) = delete;
            ConnectorType<STREAMTYPE>& operator=(const ConnectorType<STREAMTYPE>&) = delete;

            PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
            template <typename... Args>
            ConnectorType(PluginHost::Channel& channel, Args&&... args)
                : WebProxy::Connector(channel, &_streamType)
                , _streamType(*this, std::forward<Args>(args)...)
            {
            }
            POP_WARNING()

            ~ConnectorType() override = default;

        private:
            STREAMTYPE _streamType;
        };

        using Connectors = std::unordered_map<uint32_t, Connector*>;
        using Links = std::unordered_map<string, Config::Link>;

    public:
        WebProxy(WebProxy&&) = delete;
        WebProxy(const WebProxy&) = delete;
        WebProxy& operator=(const WebProxy&) = delete;

        WebProxy()
            : _prefix()
            , _maxConnections(0)
            , _connectionMap()
            , _linkInfo() {
        }
        ~WebProxy() override = default;

        BEGIN_INTERFACE_MAP(WebProxy)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IPluginExtended)
            INTERFACE_ENTRY(PluginHost::IChannel)
        END_INTERFACE_MAP

    public:
        //	IPluginExtended methods
        // -------------------------------------------------------------------------------------------------------

        // First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
        // information and services for this particular plugin. The Service object contains configuration information that
        // can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
        // If there is an error, return a string describing the issue why the initialisation failed.
        // The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
        // The lifetime of the Service object is guaranteed till the deinitialize method is called.
        const string Initialize(PluginHost::IShell* service) override;

        // The plugin is unloaded from Thunder. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        void Deinitialize(PluginHost::IShell* service) override;

        // Whenever a Channel (WebSocket connection) is created to the plugin that will be reported via the Attach.
        // Whenever the channel is closed, it is reported via the detach method.
        bool Attach(PluginHost::Channel& channel) override;
        void Detach(PluginHost::Channel& channel) override;

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        string Information() const override;

        //	IChannel methods
        // -------------------------------------------------------------------------------------------------------
        // Whenever a WebSocket is opened with a locator (URL) pointing to this plugin, it is capable of receiving
        // raw data for the plugin. Raw data received on this link will be exposed to the plugin via this interface.
        uint32_t Inbound(const uint32_t ID, const uint8_t data[], const uint16_t length) override;

        // Whenever a WebSocket is opened with a locator (URL) pointing to this plugin, it is capable of sending
        // raw data to the initiator of the websocket.
        uint32_t Outbound(const uint32_t ID, uint8_t data[], const uint16_t length) const override;

    private:
        Connector* CreateConnector(PluginHost::Channel& channel) const;

    private:
        string _prefix;
        uint32_t _maxConnections;
        Connectors _connectionMap;
        Links _linkInfo;
    };
}
}
