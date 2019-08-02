#pragma once

#include "Module.h"
#include <interfaces/IBluetooth.h>

namespace WPEFramework {
namespace Plugin {

class BluetoothControl : public PluginHost::IPlugin, public PluginHost::IWeb, public Exchange::IBluetooth {
    private:
        BluetoothControl(const BluetoothControl&) = delete;
        BluetoothControl& operator=(const BluetoothControl&) = delete;

        class ControlSocket : public Bluetooth::HCISocket {
        private:
            // The bluetooth library has some unexpected behaviour. For example, the scan of NON-BLE devices
            // is a blocking call for the duration of the passed in time. Which is, I think, very intrusive
            // fo any responsive design. If a RESTFull call would start a scan, the call would last the duration
            // of the scan, which is typicall >= 10Secods which is unacceptable, so it needs to be decoupled.
            // This decoupling is done on this internal Worker thread.
            class Job : public Core::IDispatch {
            private:
                Job() = delete;
                Job(const Job&) = delete;
                Job& operator=(const Job&) = delete;

                enum scanMode {
                    LOW_ENERGY = 0x01,
                    REGULAR = 0x02,
                    PASSIVE = 0x04,
                    LIMITED = 0x08
                };

            public:
                Job(ControlSocket* parent)
                    : _parent(*parent)
                    , _mode(0)
                {
                }
                virtual ~Job()
                {
                }

            public:
                void Load(const uint16_t scanTime, const uint32_t type, const uint8_t flags)
                {
                    if (_mode == 0) {
                        _mode = REGULAR;
                        _scanTime = scanTime;
                        _type = type;
                        _flags = flags;
                        PluginHost::WorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                    }
                }
                void Load(const uint16_t scanTime, const bool limited, const bool passive)
                {
                    if (_mode == 0) {
                        _mode = LOW_ENERGY | (passive ? PASSIVE : 0) | (limited ? LIMITED : 0);
                        _scanTime = scanTime;
                        PluginHost::WorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                    }
                }

            private:
                virtual void Dispatch()
                {
                    if ((_mode & REGULAR) != 0) {
                        TRACE(Trace::Information, (_T("Start regular scan: %s"), Core::Time::Now().ToRFC1123().c_str()));
                        _parent.Run(_scanTime, _type, _flags);
                    } else {
                        TRACE(Trace::Information, (_T("Start Low Energy scan: %s"), Core::Time::Now().ToRFC1123().c_str()));
                        _parent.Run(_scanTime, ((_mode & LIMITED) != 0), ((_mode & PASSIVE) != 0));
                    }
                    TRACE(Trace::Information, (_T("Scan completed: %s"), Core::Time::Now().ToRFC1123().c_str()));
                    _mode = 0;
                }

            private:
                ControlSocket& _parent;
                uint16_t _scanTime;
                uint32_t _type;
                uint8_t _flags;
                uint8_t _mode;
            };

        public:
            ControlSocket() = delete;
            ControlSocket(const ControlSocket&) = delete;
            ControlSocket& operator=(const ControlSocket&) = delete;

            ControlSocket(BluetoothControl& parent)
                : Bluetooth::HCISocket()
                , _parent(parent)
                , _activity(Core::ProxyType<Job>::Create(this))
            {
            }
            virtual ~ControlSocket()
            {
                PluginHost::WorkerPool::Instance().Revoke(Core::ProxyType<Core::IDispatch>(_activity));
            }

        public:
            virtual void Update(const hci_event_hdr& eventData) override
            {
                _parent.Update(eventData);
            }
            virtual void Discovered(const bool lowEnergy, const Bluetooth::Address& address, const string& name) override
            {
                _parent.Discovered(lowEnergy, address, name);
            }
            void Scan(const uint16_t scanTime, const uint32_t type, const uint8_t flags)
            {
                _activity->Load(scanTime, type, flags);
            }
            void Scan(const uint16_t scanTime, const bool limited, const bool passive)
            {
                _activity->Load(scanTime, limited, passive);
            }
            uint32_t Open(const Bluetooth::Address& address)
            {
                uint32_t result = Core::ERROR_NONE;

                if (IsOpen() == false) {
                    Bluetooth::HCISocket::LocalNode(address.NodeId(HCI_CHANNEL_RAW));
                    result = Bluetooth::HCISocket::Open(Core::infinite);
                }

                return (result);
            }
            uint32_t Close()
            {
                return (Bluetooth::HCISocket::Close(Core::infinite));
            }

        private:
            void Run(const uint16_t scanTime, const uint32_t type, const uint8_t flags)
            {
                Bluetooth::HCISocket::Scan(scanTime, type, flags);
            }
            void Run(const uint16_t scanTime, const bool limited, const bool passive)
            {
                Bluetooth::HCISocket::Scan(scanTime, limited, passive);
            }

        private:
            BluetoothControl& _parent;
            Core::ProxyType<Job> _activity;
        };

        class GATTRemote : public Bluetooth::GATTSocket {
        private:
            static constexpr uint8_t LE_ATT_CID = 4;
            static constexpr uint8_t ATT_OP_HANDLE_NOTIFY = 0x1B;

            GATTRemote() = delete;
            GATTRemote(const GATTRemote&) = delete;
            GATTRemote& operator=(const GATTRemote&) = delete;

            // UUID
            static constexpr uint16_t REPORT_UUID = 0x2a4d;
            static constexpr uint16_t PRIMARY_SERVICE_UUID = 0x2800;
            static constexpr uint16_t CHARACTERISTICS_UUID = 0x2803;
            static constexpr uint16_t HID_UUID = 0x1812;
            static constexpr uint16_t PNP_UUID = 0x2a50;
            static constexpr uint16_t DEVICE_NAME_UUID = 0x2a00;
            static constexpr uint16_t REPORT_MAP_UUID = 0x2a4b;

            enum state {
                REMOTE_OPERATIONAL,
                REMOTE_ERROR
            };
 
            class Metadata {
            public:
                Metadata(const Metadata&) = delete;
                Metadata& operator=(const Metadata&) = delete;

                Metadata()
                    : _vendorId(0)
                    , _productId(0)
                    , _version(0)
                    , _name()
                    , _blob()
                {
                }
                ~Metadata()
                {
                }

            public:
                inline uint16_t VendorId() const
                {
                    return (_vendorId);
                }
                inline uint16_t ProductId() const
                {
                    return (_productId);
                }
                inline uint16_t Version() const
                {
                    return (_version);
                }
                inline const string& Name() const
                {
                    return (_name);
                }
                inline const uint8_t* Blob() const
                {
                    return (_blob);
                }
                inline uint16_t Length() const
                {
                    return (sizeof(_blob));
                }
                inline uint16_t Country() const
                {
                    return (0);
                }

            private:
                friend class GATTRemote;

                uint16_t _vendorId;
                uint16_t _productId;
                uint16_t _version;
                string _name;
                uint8_t _blob[8 * 22];
            };

        public:
            GATTRemote(const Bluetooth::Address& remoteNode)
                : Bluetooth::GATTSocket(
                      Bluetooth::Address().AnyInterface().NodeId(Bluetooth::Address::LE_PUBLIC_ADDRESS, LE_ATT_CID, 0),
                      remoteNode.NodeId(Bluetooth::Address::LE_PUBLIC_ADDRESS, LE_ATT_CID, 0),
                      64)
                , _state(REMOTE_ERROR)
                , _inputHandler(nullptr)
                , _metadata()
                , _command()
            {
                GATTSocket::Open(1000);
            }
            virtual ~GATTRemote()
            {
                if (GATTSocket::IsOpen() == true) {
                    GATTSocket::Close(Core::infinite);
                }
            }

        private:
            virtual uint16_t Deserialize(const uint8_t* dataFrame, const uint16_t availableData) override {
                uint32_t result = 0;
                if ( (_state != REMOTE_OPERATIONAL) || (dataFrame[0] != ATT_OP_HANDLE_NOTIFY) || (availableData <= 2) ) {
                    result = GATTSocket::Deserialize(dataFrame, availableData);
                }
                else {
                    // We got a key press.. where to ?
                    printf ("Key type/mode: %d, value: %d", availableData - 2, dataFrame[2]);
                    //    _device.Send(availableData - 2, &(dataFrame[2]));
                    result = availableData;
                }
                return (result);
            }
            virtual void Operational() override
            {
                Security(BT_SECURITY_MEDIUM, 0);
                _command.FindByType(0x0001, 0xFFFF, GATTSocket::UUID(PRIMARY_SERVICE_UUID), HID_UUID);
                Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) { 
                    ASSERT (&cmd == &_command);
                    if (cmd.Error() == Core::ERROR_NONE) {
                        Version();
                    }
                    else {
                        _state = REMOTE_ERROR;
                        TRACE(Trace::Information, (_T("The given bluetooth device is not a HID device !!")));
                    }
                });
            }
            void Version () 
            {
                _command.ReadByType(0x0001, 0xFFFF, GATTSocket::UUID(PNP_UUID));
                Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) { 
                    ASSERT (&cmd == &_command);
                    if ( (cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Length() >= 6) ) {
                        const uint8_t* data(cmd.Result().Data());
                        _metadata._vendorId = (data[0] << 8) | data[1];
                        _metadata._productId = (data[2] << 8) | data[3];
                        _metadata._version = (data[4] << 8) | data[5];

                        Name();
                    }
                    else {
                        _state = REMOTE_ERROR;
                        TRACE(Trace::Information, (_T("The given bluetooth device does not report a proper Version!!")));
                    }
                });
            }
            void Name() 
            {
                _command.ReadByType(0x0001, 0xFFFF, GATTSocket::UUID(DEVICE_NAME_UUID));
                Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) { 
                    ASSERT (&cmd == &_command);
                    Command::Response& response (_command.Result());
                    if ( (cmd.Error() == Core::ERROR_NONE) && (response.Next() == true) ) {
                        _command.ReadBlob(response.Handle());
                        Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                            uint16_t length = cmd.Result().Length();
                            if ( (cmd.Error() == Core::ERROR_NONE) && (length >= 0) ) {
                                _metadata._name = string(reinterpret_cast<const char*>(cmd.Result().Data()), length);
                                Descriptors();
                            }
                            else {
                                _state = REMOTE_ERROR;
                                TRACE(Trace::Information, (_T("The given bluetooth device does not report a proper Name!!")));
                            }
                       });
                    }
                    else {
                        _state = REMOTE_ERROR;
                        TRACE(Trace::Information, (_T("The given bluetooth device does not report a proper Name Handle!!")));
                    }
                });
            }
            void Descriptors() 
            {
                _command.ReadByType(0x0001, 0xFFFF, GATTSocket::UUID(REPORT_MAP_UUID));
                Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) { 
                    ASSERT (&cmd == &_command);
                    Command::Response& response (_command.Result());
                    if ( (cmd.Error() == Core::ERROR_NONE) && (response.Next() == true) ) {
                        _command.ReadBlob(response.Handle());
                        Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                            uint16_t length = cmd.Result().Length();
                            if ( (cmd.Error() == Core::ERROR_NONE) && (length >= 0) ) {
                                uint16_t copyLength = std::min(length, static_cast<uint16_t>(sizeof(_metadata._blob)));
                                ::memcpy(_metadata._blob, cmd.Result().Data(), copyLength);
                                EnableEvents();
                            }
                            else {
                                _state = REMOTE_ERROR;
                                TRACE(Trace::Information, (_T("The given bluetooth device does not report proper descriptors!!")));
                            }
                       });
                    }
                    else {
                        _state = REMOTE_ERROR;
                        TRACE(Trace::Information, (_T("The given bluetooth device does not report a proper Descriptor Handles!!")));
                    }
                });
            }
            void EnableEvents() 
            {
                GATTSocket::UUID dataBlob(htobs(1));
                _command.WriteByType(0x0001, 0xFFFF, GATTSocket::UUID(REPORT_UUID), dataBlob.Length(), dataBlob.Data());
                Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) { 
                    ASSERT (&cmd == &_command);
                    if (cmd.Error() == Core::ERROR_NONE) {
                         _inputHandler = PluginHost::InputHandler::KeyHandler();
                         if (_inputHandler != nullptr) {
                            _state = REMOTE_OPERATIONAL;
                         }
                    }
                    else {
                        _state = REMOTE_ERROR;
                        TRACE(Trace::Information, (_T("The given bluetooth device could not enable Event reporting!!")));
                    }
                });
            }
 
        private:
            state _state;
            PluginHost::VirtualInput* _inputHandler;
            Metadata _metadata;
            Command _command;
        };

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            Config()
                : Core::JSON::Container()
                , Interface(0)
            {
                Add(_T("interface"), &Interface);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt16 Interface;
        };

    public:
        class EXTERNAL DeviceImpl : public IBluetooth::IDevice, public Bluetooth::HCISocket {
        private:
            DeviceImpl() = delete;
            DeviceImpl(const DeviceImpl&) = delete;
            DeviceImpl& operator=(const DeviceImpl&) = delete;

            static constexpr uint16_t ACTION_MASK = 0x0FFF;

        public:
            static constexpr uint32_t MAX_ACTION_TIMEOUT = 2000; /* 2S to setup a connection ? */

            enum state : uint16_t {
                DECOUPLED = 0x0001,
                METADATA = 0x0002,
                CONNECTING = 0x0004,
                DISCONNECTING = 0x0008,
                CONNECTED = 0x2000,
                PAIRED = 0x4000,
                LOWENERGY = 0x8000
            };

            class JSON : public Core::JSON::Container {
            private:
                JSON& operator=(const JSON&);

            public:
                JSON()
                    : Core::JSON::Container()
                    , Address()
                    , Name()
                    , LowEnergy(false)
                    , Connected(false)
                    , Paired(false)
                    , Reason(0)
                {
                    Add(_T("address"), &Address);
                    Add(_T("name"), &Name);
                    Add(_T("le"), &LowEnergy);
                    Add(_T("connected"), &Connected);
                    Add(_T("paired"), &Paired);
                    Add(_T("reason"), &Reason);
                }
                JSON(const JSON& copy)
                    : Core::JSON::Container()
                    , Address()
                    , Name()
                    , LowEnergy(false)
                    , Connected(false)
                    , Paired(false)
                    , Reason(0)
                {
                    Add(_T("address"), &Address);
                    Add(_T("name"), &Name);
                    Add(_T("le"), &LowEnergy);
                    Add(_T("connected"), &Connected);
                    Add(_T("paired"), &Paired);
                    Add(_T("reason"), &Reason);
                    Address = copy.Address;
                    Name = copy.Name;
                    LowEnergy = copy.LowEnergy;
                    Connected = copy.Connected;
                    Paired = copy.Paired;
                    Reason = copy.Reason;
                }
                virtual ~JSON()
                {
                }

            public:
                JSON& Set(const DeviceImpl* source)
                {
                    if (source != nullptr) {
                        Address = source->Address();
                        Name = source->Name();
                        LowEnergy = source->LowEnergy();
                        Connected = source->IsConnected();
                        Paired = source->IsPaired();
                    } else {
                        Address.Clear();
                        Name.Clear();
                        LowEnergy.Clear();
                        Paired.Clear();
                        Connected.Clear();
                    }
                    return (*this);
                }
                Core::JSON::String Address;
                Core::JSON::String Name;
                Core::JSON::Boolean LowEnergy;
                Core::JSON::Boolean Connected;
                Core::JSON::Boolean Paired;
                Core::JSON::DecUInt16 Reason;
            };

            class IteratorImpl : public IBluetooth::IDevice::IIterator {
            private:
                IteratorImpl() = delete;
                IteratorImpl(const IteratorImpl&) = delete;
                IteratorImpl& operator=(const IteratorImpl&) = delete;

            public:
                IteratorImpl(const std::list<DeviceImpl*>& container)
                {
                    std::list<DeviceImpl*>::const_iterator index = container.begin();
                    while (index != container.end()) {
                        IBluetooth::IDevice* element = (*index);
                        element->AddRef();
                        _list.push_back(element);
                        index++;
                    }
                }
                virtual ~IteratorImpl()
                {
                    while (_list.size() != 0) {
                        _list.front()->Release();
                        _list.pop_front();
                    }
                }

            public:
                virtual void Reset() override
                {
                    _index = 0;
                }
                virtual bool IsValid() const override
                {
                    return ((_index != 0) && (_index <= _list.size()));
                }
                virtual bool Next() override
                {
                    if (_index == 0) {
                        _index = 1;
                        _iterator = _list.begin();
                    } else if (_index <= _list.size()) {
                        _index++;
                        _iterator++;
                    }
                    return (IsValid());
                }
                virtual IBluetooth::IDevice* Current()
                {
                    ASSERT(IsValid() == true);
                    IBluetooth::IDevice* result = nullptr;
                    result = (*_iterator);
                    ASSERT(result != nullptr);
                    result->AddRef();
                    return (result);
                }

                BEGIN_INTERFACE_MAP(IteratorImpl)
                INTERFACE_ENTRY(IBluetooth::IDevice::IIterator)
                END_INTERFACE_MAP

            private:
                uint32_t _index;
                std::list<IBluetooth::IDevice*> _list;
                std::list<IBluetooth::IDevice*>::iterator _iterator;
            };

        public:
            DeviceImpl(const bool lowEnergy, const uint16_t deviceId, const Bluetooth::Address& remote, const string& name)
                : HCISocket(Core::NodeId(deviceId, HCI_CHANNEL_RAW))
                , _name(name)
                , _remote(remote)
                , _state(static_cast<state>(lowEnergy ? LOWENERGY : 0))
            {
                SocketPort::RemoteNode(SocketPort::LocalNode());
            }
            ~DeviceImpl()
            {
            }

        public:
            virtual uint32_t Pair() override
            {
                _state.SetState(PAIRED);
                uint8_t type((_state & LOWENERGY) != 0 ? Bluetooth::Address::LE_PUBLIC_ADDRESS : Bluetooth::Address::BREDR_ADDRESS);

                return (BluetoothControl::Administrator().Pair(_remote, type, Bluetooth::HCISocket::capabilities::NO_INPUT_NO_OUTPUT));
            }
            virtual uint32_t Unpair() override
            {
                return (BluetoothControl::Administrator().Unpair(_remote));
            }
            virtual string Address() const override
            {
                return (_remote.ToString());
            }
            virtual string Name() const override
            {
                return (_name);
            }
            virtual bool IsDiscovered() const override
            {
                return ((_state & (METADATA | DECOUPLED)) == 0);
            }
            virtual bool IsPaired() const override
            {
                return ((_state & PAIRED) != 0);
            }
            inline bool LowEnergy() const
            {
                return ((_state & LOWENERGY) != 0);
            }
            inline void Clear()
            {
                _state.Lock();
                if ((IsConnected() == false) && (IsPaired() == false) && ((_state & ACTION_MASK) == 0)) {
                    _state.SetState(static_cast<state>(_state.GetState() | DECOUPLED));
                }
                _state.Unlock();
            }
            inline void Discovered()
            {
                _state.Lock();
                if ((_state & ACTION_MASK) == DECOUPLED) {
                    _state.SetState(static_cast<state>(_state.GetState() & (~DECOUPLED)));
                }
                _state.Unlock();
            }
            inline bool operator==(const Bluetooth::Address& rhs) const
            {
                return (_remote == rhs);
            }
            inline bool operator!=(const Bluetooth::Address& rhs) const
            {
                return (!operator==(rhs));
            }

            BEGIN_INTERFACE_MAP(DeviceImpl)
            INTERFACE_ENTRY(IBluetooth::IDevice)
            END_INTERFACE_MAP

            uint32_t WaitState(const uint32_t state, const uint32_t waitTime)
            {
                return (_state.WaitState(state, waitTime));
            }
            Bluetooth::HCISocket::FeatureIterator Features() const
            {
                return (Bluetooth::HCISocket::FeatureIterator(static_cast<uint8_t>(sizeof(_features)), _features));
            }
            const Bluetooth::Address& Locator() const 
            {
                return(_remote);
            }
 
        protected:
            void SetFeatures(const uint8_t length, const uint8_t feature[])
            {
                uint8_t copyLength = std::min(length, static_cast<uint8_t>(sizeof(_features)));
                ::memcpy(_features, feature, copyLength);
            }
            template<typename MESSAGE>
            uint32_t Send(const uint32_t waitTime, Core::IOutbound::ICallback* callback, const state value, MESSAGE& message)
            {
                uint32_t result = Core::ERROR_ALREADY_RELEASED;

                _state.Lock();

                if (SetState(value) == Core::ERROR_NONE) {
                    Bluetooth::HCISocket::Send(waitTime, message, callback, &message);
                    result = Core::ERROR_NONE;
                } else if ((_state & ACTION_MASK) == value) {
                    _state.SetState(static_cast<state>(_state.GetState() & (~value)));
                }

                _state.Unlock();

                return (result);
            }
            uint32_t SetState(const state value)
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                _state.Lock();

                if ((_state & ACTION_MASK) == 0) {

                    result = Core::ERROR_NONE;

                    _state.SetState(static_cast<state>(_state.GetState() | value));
                }

                _state.Unlock();

                return (result);
            }
            uint32_t ClearState(const state value)
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                _state.Lock();

                if ((_state & ACTION_MASK) == value) {

                    result = Core::ERROR_NONE;

                    _state.SetState(static_cast<state>(_state.GetState() & (~value)));
                }

                _state.Unlock();

                return (result);
            }
            void SetName(const string& name)
            {
                _name = name;
            }

        private:
            string _name;
            Bluetooth::Address _remote;
            Core::StateTrigger<state> _state;
            uint8_t _features[8];
        };

        class EXTERNAL DeviceRegular : public DeviceImpl, Core::IOutbound::ICallback {
        private:
            DeviceRegular() = delete;
            DeviceRegular(const DeviceRegular&) = delete;
            DeviceRegular& operator=(const DeviceRegular&) = delete;

        public:
            DeviceRegular(const uint16_t deviceId, const Bluetooth::Address& address, const string& name)
                : DeviceImpl(false, deviceId, address, name)
                , _handle(~0)
            {

                if (SetState(METADATA) == Core::ERROR_NONE) {

                    _name.Clear();
                    _name->bdaddr = *(Locator().Data());
                    _name->pscan_mode = 0x00;
                    _name->pscan_rep_mode = 0x02;
                    _name->clock_offset = 0x0000;
                    Send(MAX_ACTION_TIMEOUT, this, METADATA, _name);
                }
            }
            virtual ~DeviceRegular()
            {
            }

        public:
            virtual bool IsConnected() const override
            {
                return (_handle != static_cast<uint16_t>(~0));
            }
            virtual uint32_t Connect() override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(CONNECTING) == Core::ERROR_NONE) {

                    _connect.Clear();
                    _connect->bdaddr = *(Locator().Data());
                    _connect->pkt_type = htobs(HCI_DM1 | HCI_DM3 | HCI_DM5 | HCI_DH1 | HCI_DH3 | HCI_DH5);
                    _connect->pscan_rep_mode = 0x02;
                    _connect->clock_offset = 0x0000;
                    _connect->role_switch = 0x01;
                    result = Send(MAX_ACTION_TIMEOUT, this, CONNECTING, _connect);
                }

                return (result);
            }
            virtual uint32_t Disconnect(const uint16_t reason) override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(DISCONNECTING) == Core::ERROR_NONE) {
                    _disconnect->handle = htobs(_handle);
                    _disconnect->reason = (reason & 0xFF);
                    result = Send(MAX_ACTION_TIMEOUT, this, DISCONNECTING, _disconnect);
                }

                return (result);
            }

        private:
            virtual void Updated(const Core::IOutbound& data, const uint32_t error_code) override
            {
                if (&_name == &data) {
                    // Metadata is flowing in, handle it..
                    // _cmds.name.Response().bdaddr;
                    const char* longName = reinterpret_cast<const char*>(_name.Response().name);
                    uint8_t index = 0;
                    printf("UPDATED => ");
                    while (index < HCI_MAX_NAME_LENGTH) {
                        printf("%c", ::isprint(longName[index]) ? longName[index] : '.');
                        index++;
                    }
                    printf("\n");
                    index = 0;
                    while ((index < HCI_MAX_NAME_LENGTH) && (::isprint(longName[index]) != 0)) {
                        index++;
                    }

                    SetName(std::string(longName, index));
                    TRACE(Trace::Information, (_T("Loaded Long Device Name: %s"),longName));
                    ClearState(METADATA);
                } else if (&_connect == &data) {
                    TRACE(Trace::Information, (_T("Connected")));
                    // looks like we are connected..
                    _handle = _connect.Response().handle;
                    ClearState(CONNECTING);
                } else if (&_disconnect == &data) {
                    if (error_code == Core::ERROR_NONE) {
                        TRACE(Trace::Information, (_T("Disconnected")));
                        ClearState(DISCONNECTING);
                        _handle = ~0;
                    } else {
                        TRACE(Trace::Error, (_T("Disconnected Failed!")));
                    }
                    // Seems no need to clear the state, since the Disconnected Failed.
                    //ClearState(DISCONNECTING);
                }
            }

        private:
            uint16_t _handle;
            Bluetooth::HCISocket::Command::Connect _connect;
            Bluetooth::HCISocket::Command::Disconnect _disconnect;
            Bluetooth::HCISocket::Command::RemoteName _name;
        };

        class EXTERNAL DeviceLowEnergy : public DeviceImpl, Core::IOutbound::ICallback {
        private:
            DeviceLowEnergy() = delete;
            DeviceLowEnergy(const DeviceLowEnergy&) = delete;
            DeviceLowEnergy& operator=(const DeviceLowEnergy&) = delete;

        public:
            DeviceLowEnergy(const uint16_t deviceId, const Bluetooth::Address& address, const string& name)
                : DeviceImpl(true, deviceId, address, name)
                , _handle(~0)
            {
                Connect();
            }
            virtual ~DeviceLowEnergy()
            {
                Disconnect(0);
            }

        public:
            virtual bool IsConnected() const override
            {
                return (_handle != static_cast<uint16_t>(~0));
            }

        private:
            virtual uint32_t Connect() override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(CONNECTING) == Core::ERROR_NONE) {

                    _connect.Clear();
                    _connect->interval = htobs(0x0004);
                    _connect->window = htobs(0x0004);
                    _connect->initiator_filter = 0;
                    _connect->peer_bdaddr_type = LE_PUBLIC_ADDRESS;
                    _connect->peer_bdaddr = *(Locator().Data());
                    _connect->own_bdaddr_type = LE_PUBLIC_ADDRESS;
                    _connect->min_interval = htobs(0x000F);
                    _connect->max_interval = htobs(0x000F);
                    _connect->latency = htobs(0x0000);
                    _connect->supervision_timeout = htobs(0x0C80);
                    _connect->min_ce_length = htobs(0x0001);
                    _connect->max_ce_length = htobs(0x0001);
                    result = Send(MAX_ACTION_TIMEOUT, this, CONNECTING, _connect);
                }

                return (result);
            }
            virtual uint32_t Disconnect(const uint16_t reason) override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(DISCONNECTING) == Core::ERROR_NONE) {

                    _disconnect->handle = htobs(_handle);
                    _disconnect->reason = reason & 0xFF;
                    result = Send(MAX_ACTION_TIMEOUT, this, DISCONNECTING, _disconnect);
                }

                return (result);
            }
            virtual void Update(const hci_event_hdr& eventData) override
            {
                if ((eventData.evt == EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE) && (eventData.plen >= sizeof(evt_read_remote_features_complete))) {
                    const evt_read_remote_features_complete& info = reinterpret_cast<const evt_read_remote_features_complete&>(reinterpret_cast<const uint8_t*>(&eventData)[sizeof(hci_event_hdr&)]);
                    if (_handle == info.handle) {
                        SetFeatures(sizeof(info.features), info.features);
                    }
                }
            }
            virtual void Updated(const Core::IOutbound& data, const uint32_t error_code) override
            {
                if (&data == &_connect) {
                    if ((error_code == Core::ERROR_NONE) && (_connect.Response().status == 0)) {
                        _handle = _connect.Response().handle;
                        ClearState(CONNECTING);
                        TRACE(Trace::Information, (_T("Connected using handle: %d"),_handle));
                    } else {
                        TRACE(Trace::Error, (_T("Connec Failed!")));
                    }
                } else if (&data == &_disconnect) {
                    if (error_code == Core::ERROR_NONE) {
                        TRACE(Trace::Information, (_T("Disconnected")));
                        _handle = ~0;
                        ClearState(DISCONNECTING);
                    } else {
                        TRACE(Trace::Error, (_T("Disconnected Failed!")));
                    }
                }
            }

        private:
            Bluetooth::HCISocket::Command::ConnectLE _connect;
            Bluetooth::HCISocket::Command::Disconnect _disconnect;
            uint16_t _handle;
        };

        class EXTERNAL Status : public Core::JSON::Container {
        private:
            Status(const Status&) = delete;
            Status& operator=(const Status&) = delete;

        public:
            Status()
                : Scanning()
                , Devices()
            {
                Add(_T("scanning"), &Scanning);
                Add(_T("devices"), &Devices);
            }
            virtual ~Status()
            {
            }

        public:
            Core::JSON::Boolean Scanning;
            Core::JSON::ArrayType<DeviceImpl::JSON> Devices;
        };

    public:
        BluetoothControl()
            : _skipURL(0)
            , _adminLock()
            , _service(nullptr)
            , _application(*this)
            , _btInterface(0)
            , _btAddress()
            , _devices()
            , _observers()
            , _gattRemotes()
        {
        }
        virtual ~BluetoothControl()
        {
        }

    public:
        BEGIN_INTERFACE_MAP(BluetoothControl)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(Exchange::IBluetooth)
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

        // The plugin is unloaded from the webbridge. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After this call, the lifetime of the Service object ends.
        virtual void Deinitialize(PluginHost::IShell* service);

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetaData plugin to publish this information to the outside world.
        virtual string Information() const;

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request);
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request);

        //  IBluetooth methods
        // -------------------------------------------------------------------------------------------------------
        virtual bool IsScanning() const override;
        virtual uint32_t Register(IBluetooth::INotification* notification) override;
        virtual uint32_t Unregister(IBluetooth::INotification* notification) override;

        virtual bool Scan(const bool enable) override;
        virtual IBluetooth::IDevice* Device(const string&) override;
        virtual IBluetooth::IDevice::IIterator* Devices() override;

        inline static Bluetooth::HCISocket& Administrator() {
            return(_administrator);
        }

    private:
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        void RemoveDevices(std::function<bool(DeviceImpl*)> filter);
        void Discovered(const bool lowEnergy, const Bluetooth::Address& address, const string& name);
        void Update(const hci_event_hdr& eventData);
        void Notification(const uint8_t subEvent, const uint16_t length, const uint8_t* dataFrame);
        DeviceImpl* Find(const string&);

    private:
        uint8_t _skipURL;
        Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        ControlSocket _application;
        uint16_t _btInterface;
        Bluetooth::Address _btAddress;
        std::list<DeviceImpl*> _devices;
        std::list<IBluetooth::INotification*> _observers;
        std::list<GATTRemote> _gattRemotes;
        Config _config;

        static Bluetooth::HCISocket _administrator;
    };
} //namespace Plugin

} //namespace Solution
