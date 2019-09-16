#pragma once

#include "Module.h"
#include <interfaces/IBluetooth.h>

namespace WPEFramework {
namespace Plugin {

class BluetoothControl : public PluginHost::IPlugin, public PluginHost::IWeb, public Exchange::IBluetooth {
    private:
        class ManagementFlow {
        public:
            ManagementFlow() = delete;
            ManagementFlow(const ManagementFlow&) = delete;
            ManagementFlow& operator=(const ManagementFlow&) = delete;
            ManagementFlow(const TCHAR formatter[], ...)
            {
                va_list ap;
                va_start(ap, formatter);
                Trace::Format(_text, formatter, ap);
                va_end(ap);
            }
            explicit ManagementFlow(const string& text)
                : _text(Core::ToString(text))
            {
            }
            ~ManagementFlow()
            {
            }

        public:
            inline const char* Data() const
            {
                return (_text.c_str());
            }
            inline uint16_t Length() const
            {
                return (static_cast<uint16_t>(_text.length()));
            }

        private:
            std::string _text;
        };
        class ControlFlow {
        public:
            ControlFlow() = delete;
            ControlFlow(const ControlFlow&) = delete;
            ControlFlow& operator=(const ControlFlow&) = delete;
            ControlFlow(const TCHAR formatter[], ...)
            {
                va_list ap;
                va_start(ap, formatter);
                Trace::Format(_text, formatter, ap);
                va_end(ap);
            }
            explicit ControlFlow(const string& text)
                : _text(Core::ToString(text))
            {
            }
            ~ControlFlow()
            {
            }

        public:
            inline const char* Data() const
            {
                return (_text.c_str());
            }
            inline uint16_t Length() const
            {
                return (static_cast<uint16_t>(_text.length()));
            }

        private:
            std::string _text;
        };
        class DeviceFlow {
        public:
            DeviceFlow() = delete;
            DeviceFlow(const DeviceFlow&) = delete;
            DeviceFlow& operator=(const DeviceFlow&) = delete;
            DeviceFlow(const TCHAR formatter[], ...)
            {
                va_list ap;
                va_start(ap, formatter);
                Trace::Format(_text, formatter, ap);
                va_end(ap);
            }
            explicit DeviceFlow(const string& text)
                : _text(Core::ToString(text))
            {
            }
            ~DeviceFlow()
            {
            }

        public:
            inline const char* Data() const
            {
                return (_text.c_str());
            }
            inline uint16_t Length() const
            {
                return (static_cast<uint16_t>(_text.length()));
            }

        private:
            std::string _text;
        };
        class GATTFlow {
        public:
            GATTFlow() = delete;
            GATTFlow(const GATTFlow&) = delete;
            GATTFlow& operator=(const GATTFlow&) = delete;
            GATTFlow(const TCHAR formatter[], ...)
            {
                va_list ap;
                va_start(ap, formatter);
                Trace::Format(_text, formatter, ap);
                va_end(ap);
            }
            explicit GATTFlow(const string& text)
                : _text(Core::ToString(text))
            {
            }
            ~GATTFlow()
            {
            }

        public:
            inline const char* Data() const
            {
                return (_text.c_str());
            }
            inline uint16_t Length() const
            {
                return (static_cast<uint16_t>(_text.length()));
            }

        private:
            std::string _text;
        };

        class ControlSocket : public Bluetooth::HCISocket {
        private:
            class ManagementSocket : public Bluetooth::ManagementSocket {
            public:
                ManagementSocket(const ManagementSocket&) = delete;
                ManagementSocket& operator= (const ManagementSocket&) = delete;

                ManagementSocket() : Bluetooth::ManagementSocket() {
                }
                ~ManagementSocket() {
                }

            private:
                void Update(const mgmt_hdr& header) override {
                    const uint8_t* data = &(reinterpret_cast<const uint8_t*>(&header)[sizeof(mgmt_hdr)]);

                    uint16_t opCode     = htobs(header.opcode);
                    uint16_t device     = htobs(header.index);
                    uint16_t packageLen = htobs(header.len);

                    switch (opCode) {
                    case MGMT_EV_CONTROLLER_ERROR: {
                         const mgmt_ev_controller_error* info = reinterpret_cast<const mgmt_ev_controller_error*>(data);
                         TRACE(ManagementFlow, (_T("MGMT_EV_CONTROLLER_ERROR [%d]"), info->error_code));
                         break;
                    }
                    case MGMT_EV_NEW_CONN_PARAM: {
                         const mgmt_ev_new_conn_param* info = reinterpret_cast<const mgmt_ev_new_conn_param*>(data);
                         TRACE(ManagementFlow, (_T("MGMT_EV_NEW_CONN_PARAM timeout [%d]"), info->timeout));
                         break;
                    }
                    case MGMT_EV_DEVICE_CONNECTED: {
                         const mgmt_ev_device_connected* info = reinterpret_cast<const mgmt_ev_device_connected*>(data);
                         TRACE(ManagementFlow, (_T("MGMT_EV_DEVICE_CONNECTED device [%s], flags [%02X]"), Bluetooth::Address(info->addr.bdaddr).ToString().c_str(), info->flags));
                         break;
                    }
                    case MGMT_EV_NEW_IRK: {
                         const mgmt_ev_new_irk* info = reinterpret_cast<const mgmt_ev_new_irk*>(data);
                         TRACE(ManagementFlow, (_T("MGMT_EV_NEW_IRK store_hint [%d]"), info->store_hint));
                         break;
                    }
                    default: 
                         TRACE(ManagementFlow, (_T("Device=%d,  OpCode=0x%04X, Length=%d"), device, opCode, packageLen));
                         string dataText;
                         Core::ToHexString(data, packageLen, dataText);
                         TRACE(ManagementFlow, (_T("Data=%s"), dataText.c_str()));
                         break;
                    }
                }
            };

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
                        TRACE(ControlFlow, (_T("Start regular scan: %s"), Core::Time::Now().ToRFC1123().c_str()));
                        _parent.Run(_scanTime, _type, _flags);
                    } else {
                        TRACE(ControlFlow, (_T("Start Low Energy scan: %s"), Core::Time::Now().ToRFC1123().c_str()));
                        _parent.Run(_scanTime, ((_mode & LIMITED) != 0), ((_mode & PASSIVE) != 0));
                    }
                    TRACE(ControlFlow, (_T("Scan completed: %s"), Core::Time::Now().ToRFC1123().c_str()));
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
            ControlSocket(const ControlSocket&) = delete;
            ControlSocket& operator=(const ControlSocket&) = delete;

            ControlSocket()
                : Bluetooth::HCISocket()
                , _parent(nullptr)
                , _activity(Core::ProxyType<Job>::Create(this))
            {
            }
            virtual ~ControlSocket()
            {
            }

        public:
            Bluetooth::ManagementSocket& Control() 
            {
                return(_administrator);
            }
            uint32_t Pair(const Bluetooth::Address& remote, const Bluetooth::Address::type type, const Bluetooth::ManagementSocket::capabilities caps) 
            {
                uint32_t result = _administrator.Discovering (true, true, true);
                if (result != Core::ERROR_NONE) {
                    TRACE(ControlFlow, (_T("Could not suspend the Discover Service. Error: %d"), result));
                }
                else {
                    result = _administrator.Pair(remote, type, caps);
                    if (_administrator.Discovering(false, true, true) != Core::ERROR_NONE) {
                        TRACE(ControlFlow, (_T("Could not resume the Discover Service. Error: %d"), result));
                    }
                }
                return (result);
            }
            uint32_t Unpair(const Bluetooth::Address& remote) 
            {
                return (_administrator.Unpair(remote));
            }
            void Scan(const uint16_t scanTime, const uint32_t type, const uint8_t flags)
            {
                if (IsOpen() == true) {
                    _activity->Load(scanTime, type, flags);
                }
            }
            void Scan(const uint16_t scanTime, const bool limited, const bool passive)
            {
                if (IsOpen() == true) {
                    _activity->Load(scanTime, limited, passive);
                }
            }
            uint32_t Open(BluetoothControl& parent)
            {
                ASSERT (IsOpen() == false);

                _parent = &parent;

                Bluetooth::HCISocket::LocalNode(Core::NodeId(_administrator.DeviceId(), HCI_CHANNEL_RAW));
                return (Bluetooth::HCISocket::Open(Core::infinite));
            }
            uint32_t Close()
            {
                uint32_t result = Bluetooth::HCISocket::Close(Core::infinite);
                PluginHost::WorkerPool::Instance().Revoke(Core::ProxyType<Core::IDispatch>(_activity));
                Bluetooth::ManagementSocket::Down(_administrator.DeviceId());
                _administrator.DeviceId(HCI_DEV_NONE);
                _parent = nullptr;
                return (result);
            }

        private:
            void Discovered(const bool lowEnergy, const Bluetooth::Address& address, const string& name) override
            {
                if (_parent != nullptr) {
                    _parent->Discovered(lowEnergy, address, name);
                }
            }
            void Update(const le_advertising_info& eventData) override 
            {
                string name;
                uint8_t type = Name (eventData, name);
               
                if ( (type != 0) && (_parent != nullptr) ) {
                    _parent->Discovered(true, Bluetooth::Address(eventData.bdaddr), name);
                }
            }
            void Run(const uint16_t scanTime, const uint32_t type, const uint8_t flags)
            {
                Bluetooth::HCISocket::Scan(scanTime, type, flags);
            }
            void Run(const uint16_t scanTime, const bool limited, const bool passive)
            {
                Bluetooth::HCISocket::Scan(scanTime, limited, passive);
            }
            void Update(const hci_event_hdr& eventData) override {
                const uint8_t* data = &(reinterpret_cast<const uint8_t*>(&eventData)[sizeof(hci_event_hdr)]);

                switch (eventData.evt) {
                    case 0:
                         break;
                    case EVT_VENDOR: {
                         TRACE(ControlFlow, (_T("EVT_VENDOR vendor specific information, length [%d]"), eventData.plen));
                         break;
                    }
                    case EVT_CMD_STATUS: {
                         const evt_cmd_status* cs = reinterpret_cast<const evt_cmd_status*>(data);
                         uint16_t opcode = htobs(cs->opcode);
                         uint8_t cat = (opcode >> 10) & 0x3F;
                         uint16_t id = (opcode & 0x3FF);
                         TRACE(ControlFlow, (_T("EVT_CMD_STATUS OpCode: %02X:%03X, Status: %d"), cat, id, cs->status));
                         break;
                    }
                    case EVT_CMD_COMPLETE: {
                         const evt_cmd_complete* cc = reinterpret_cast<const evt_cmd_complete*>(data);
                         uint16_t opcode = htobs(cc->opcode);
                         uint8_t cat = (opcode >> 10) & 0x3F;
                         uint16_t id = (opcode & 0x3FF);
                         TRACE(ControlFlow, (_T("EVT_CMD_COMPLETE OpCode: %02X:%03X"), cat, id));
                         break;
                    }
                    case EVT_LE_META_EVENT: {
                         const evt_le_meta_event* input = reinterpret_cast<const evt_le_meta_event*>(data);

                         if (input->subevent == EVT_LE_CONN_COMPLETE) {
                             const evt_le_connection_complete* info = reinterpret_cast<const evt_le_connection_complete*>(input->data);
                             Bluetooth::Address host (info->peer_bdaddr);
                             TRACE(ControlFlow, (_T("EVT_LE_CONN_COMPLETE: Status=%d"), info->status));
                             
                             if ( (info->status == 0) && (_parent != nullptr) )  {
                                 DeviceImpl* entry = _parent->Find(host);
                                 if (entry != nullptr) {
                                     entry->Connection(info->handle, info->role);
                                 }
                             }
                         } else if (input->subevent == EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE) {
                             const evt_le_read_remote_used_features_complete* info = reinterpret_cast<const evt_le_read_remote_used_features_complete*>(input->data);
                             TRACE(ControlFlow, (_T("EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE: Status=%d"), info->status));

                             if ( (info->status == 0) && (_parent != nullptr) ) {
                                 DeviceImpl* entry = _parent->Find(info->handle);
                                 if (entry != nullptr) {
                                     entry->Features(sizeof(info->features), info->features);
                                 }
                             }
                         } else if (input->subevent == EVT_LE_CONN_UPDATE_COMPLETE) {
                             const evt_le_connection_update_complete* info = reinterpret_cast<const evt_le_connection_update_complete*>(input->data);
                             TRACE(ControlFlow, (_T("EVT_LE_CONN_UPDATE_COMPLETE: Status=%d"), info->status));

                             if ( (info->status == 0)  && (_parent != nullptr) ) {
                                 DeviceImpl* entry = _parent->Find(info->handle);
                                 if (entry != nullptr) {
                                     entry->Connection(info->interval, info->latency, info->supervision_timeout);
                                 }
                             }
                         } else {
                             TRACE(ControlFlow, (_T("EVT_LE_META_EVENT: unexpected subevent: %d"), input->subevent));
                         }
                         break;
                    }
                    case EVT_IO_CAPABILITY_RESPONSE: {
                         const evt_io_capability_response* info = reinterpret_cast<const evt_io_capability_response*>(data);
                         Bluetooth::Address host (info->bdaddr);
                         TRACE(ControlFlow, (_T("EVT_IO_CAPABILITY_RESPONSE: %s"), host.ToString().c_str()));
                         if (_parent != nullptr) {
                             _parent->Capabilities(host, info->capability, info->authentication, info->oob_data);
                         }
                         break;
                    }
                    default:
                         TRACE(ControlFlow, (_T("UNKNOWN_EVENT: %x"), eventData.evt));
                         break;
                }
            }


        private:
            BluetoothControl* _parent;
            Core::ProxyType<Job> _activity;
            ManagementSocket _administrator;
        };

        class GATTRemote : public Bluetooth::GATTSocket {
        private:
            static constexpr uint8_t LE_ATT_CID = 4;
            static constexpr uint8_t ATT_OP_HANDLE_NOTIFY = 0x1B;

            // UUID
            static constexpr uint16_t REPORT_UUID = 0x2a4d;
            static constexpr uint16_t PRIMARY_SERVICE_UUID = 0x2800;
            static constexpr uint16_t CHARACTERISTICS_UUID = 0x2803;
            static constexpr uint16_t HID_UUID = 0x1812;
            static constexpr uint16_t PNP_UUID = 0x2a50;
            static constexpr uint16_t DEVICE_NAME_UUID = 0x2a00;
            static constexpr uint16_t REPORT_MAP_UUID = 0x2a4b;

            enum state : uint8_t {
                UNKNOWN     = 0x00,
                PAIRING     = 0x01,
                UNPAIRING   = 0x02,
                OPERATIONAL = 0x04
            };

            class Sink : public Exchange::IBluetooth::IDevice::ICallback {
            public:
                Sink() = delete;
                Sink(const Sink&) = delete;
                Sink& operator=(const Sink&) = delete;

                Sink(GATTRemote* parent) : _parent(*parent) {
                    ASSERT(parent != nullptr);
                }
                virtual ~Sink() {
                }

            public:
                void Updated () override {
                    _parent.Updated();
                }

                BEGIN_INTERFACE_MAP(Sink)
                INTERFACE_ENTRY(IBluetooth::IDevice::ICallback)
                END_INTERFACE_MAP

            private:
                GATTRemote& _parent;
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

            static Core::NodeId Designator(const uint8_t type, const string& address) {
                return(Bluetooth::Address(address.c_str()).NodeId(static_cast<Bluetooth::Address::type>(type), LE_ATT_CID, 0));
            }

        public:
            GATTRemote() = delete;
            GATTRemote(const GATTRemote&) = delete;
            GATTRemote& operator=(const GATTRemote&) = delete;
            GATTRemote(IBluetooth::IDevice* device)
                : Bluetooth::GATTSocket(
                      // need to set local interface explicitly, "any" ends with "no route to host"
                      Designator(device->Type(), device->LocalId()),
                      // TODO: public/private needs to be configurable
                      // for cid ATT, PSM 0 is the only valid value
                      Designator(device->Type(), device->RemoteId()),
                      64)
                , _adminLock()
                , _state(UNKNOWN)
                , _inputHandler(nullptr)
                , _metadata()
                , _command()
                , _device(device)
                , _sink(this)
            {
                _device->AddRef();

                if (_device->Callback(&_sink) != Core::ERROR_NONE) {
                    TRACE(Trace::Fatal, (_T("The device is already in use. Only 1 callback allowed")));
                }
                else if (_device->IsConnected() == true) {
                    TRACE(Trace::Fatal, (_T("The device is already connected. First disconnect the device")));
                }
            }
            virtual ~GATTRemote()
            {
                if (GATTSocket::IsOpen() == true) {
                    GATTSocket::Close(Core::infinite);
                }
                if (_device != nullptr) {
                    if (_device->Callback(nullptr) != Core::ERROR_NONE) {
                        TRACE(Trace::Fatal, (_T("Could not remove the callback from the device. Scarry !!!")));
                    }
                    _device->Release();
                    _device = nullptr;
                }
            }

        public:
            uint32_t Pair () {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                _adminLock.Lock();

                if (_device != nullptr) {
                    result = Core::ERROR_INPROGRESS;
                    if ( (_state & (PAIRING|UNPAIRING)) == 0) {
                        IBluetooth::IDevice* device = _device;
                        device->AddRef();
                        _state = static_cast<state>(_state | PAIRING);
                        _adminLock.Unlock();

                        result = device->Pair(IBluetooth::IDevice::DISPLAY_ONLY);
                        device->Release();

                        _adminLock.Lock();
                        _state = static_cast<state>(_state & (~PAIRING));
                    }
                }

                _adminLock.Unlock();

                return (result);
            }
            uint32_t Unpair () {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                _adminLock.Lock();

                if (_device != nullptr) {
                    result = Core::ERROR_INPROGRESS;
                    if ( (_state & (PAIRING|UNPAIRING)) == 0) {
                        IBluetooth::IDevice* device = _device;
                        device->AddRef();
                        _state = static_cast<state>(_state | UNPAIRING);
                        _adminLock.Unlock();

                        result = device->Unpair();
                        device->Release();

                        _adminLock.Lock();
                        _state = static_cast<state>(_state & (~UNPAIRING));
		    }
                }

                _adminLock.Unlock();

                return (result);
            }

        private:
            bool Initialize() override {
                // Security(BT_SECURITY_MEDIUM, 0);
                Security(BT_SECURITY_LOW, 0);
                return (true);
            }
            virtual uint16_t Deserialize(const uint8_t* dataFrame, const uint16_t availableData) override {
                uint32_t result = 0;
                if ( (_state != OPERATIONAL) || (dataFrame[0] != ATT_OP_HANDLE_NOTIFY) || (availableData <= 2) ) {
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
                TRACE(GATTFlow, (_T("The received MTU: %d"), MTU()));
                _command.FindByType(0x0001, 0xFFFF, GATTSocket::UUID(PRIMARY_SERVICE_UUID), HID_UUID);
                Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) { 
                    ASSERT (&cmd == &_command);
                    if (cmd.Error() == Core::ERROR_NONE) {
                        if (cmd.Result().Length() > 0) {
                            const uint8_t* data(cmd.Result().Data());
                            printf("Listing HID_UUID response (handles): ");
                            for (int i = 0; i < cmd.Result().Length(); i++) {
                                printf("%02x, ", data[i]);
                            }
                            printf("\n");
                        } else {
                            TRACE_L1("No HID handles?");
                        }

                        Version();
                    }
                    else {
                        _state = UNKNOWN;
                        TRACE(GATTFlow, (_T("The given bluetooth device is not a HID device !!")));
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
                        _state = UNKNOWN;
                        TRACE(GATTFlow, (_T("The given bluetooth device does not report a proper Version!!")));
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

                                TRACE(GATTFlow, (_T("BT device: '%s' version %i, vendor ID %02x, product ID %02x"),
                                    _metadata._name.c_str(), _metadata._version, _metadata._vendorId, _metadata._productId));

                                Descriptors();
                            }
                            else {
                                _state = UNKNOWN;
                                TRACE(GATTFlow, (_T("The given bluetooth device does not report a proper Name!!")));
                            }
                       });
                    }
                    else {
                        _state = UNKNOWN;
                        TRACE(GATTFlow, (_T("The given bluetooth device does not report a proper Name Handle!!")));
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
                            }
                    } else {
                        _inputHandler = PluginHost::InputHandler::Handler();
                        if (_inputHandler != nullptr) {
                            _state = OPERATIONAL;
                        }
                    }
                    EnableEvents();
                });
            }
            void EnableEvents() 
            {
                short val = htobs(1);
                _command.Write(REPORT_UUID, sizeof(val), (const uint8_t *) &val);
                Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) { 
                    ASSERT (&cmd == &_command);
                    if (cmd.Error() == Core::ERROR_NONE) {
                         _inputHandler = PluginHost::InputHandler::KeyHandler();
                         if (_inputHandler != nullptr) {
                            _state = OPERATIONAL;
                         }
                    }
                    else {
                        _state = UNKNOWN;
                        TRACE(GATTFlow, (_T("The given bluetooth device could not enable Event reporting!!")));
                    }
                });
            }
            void Updated () {
                _adminLock.Lock();
                if (_device != nullptr) {
                    if (((_state & PAIRING) != 0) && (_device->IsConnected() == true) && (_device->IsDiscovered() == true)) {
                        // start the discovery cycle for bonding !!!
                        uint32_t result = GATTSocket::Open(0);
                        if ((result != Core::ERROR_NONE) && (result != Core::ERROR_INPROGRESS)) {
                            TRACE(GATTFlow, (_T("Opening GATT socket [%s], failed: %i"), _device->RemoteId().c_str(), result)); 
                        }
                    }
                    else if (_device->HasDisappeared() == true) {
                        _device->Release();
                        _device = nullptr;
                    }
                }
                _adminLock.Unlock();
            }
 
        private:
            Core::CriticalSection _adminLock;
            state _state;
            PluginHost::VirtualInput* _inputHandler;
            Metadata _metadata;
            Command _command;
            IBluetooth::IDevice* _device;
            Core::Sink<Sink> _sink;
        };

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            Config()
                : Core::JSON::Container()
                , Interface(0)
                , Name(_T("Thunder BT Control"))
                , External(false)
            {
                Add(_T("interface"), &Interface);
                Add(_T("name"), &Name);
                Add(_T("external"), &External);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt16 Interface;
            Core::JSON::String Name;
            Core::JSON::Boolean External;
        };

    public:
        class EXTERNAL DeviceImpl : public IBluetooth::IDevice {
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
                PAIRING = 0x0010,
                UNPAIRING = 0x0020,
                PAIRED = 0x2000,
                LOWENERGY = 0x4000,
                PUBLIC = 0x8000
            };

            class JSON : public Core::JSON::Container {
            private:
                JSON& operator=(const JSON&);

            public:
                JSON()
                    : Core::JSON::Container()
                    , LocalId()
                    , RemoteId()
                    , Name()
                    , LowEnergy(false)
                    , Connected(false)
                    , Paired(false)
                    , Reason(0)
                {
                    Add(_T("local"), &LocalId);
                    Add(_T("remote"), &RemoteId);
                    Add(_T("name"), &Name);
                    Add(_T("le"), &LowEnergy);
                    Add(_T("connected"), &Connected);
                    Add(_T("paired"), &Paired);
                    Add(_T("reason"), &Reason);
                }
                JSON(const JSON& copy)
                    : Core::JSON::Container()
                    , LocalId()
                    , RemoteId()
                    , Name()
                    , LowEnergy(false)
                    , Connected(false)
                    , Paired(false)
                    , Reason(0)
                {
                    Add(_T("local"), &LocalId);
                    Add(_T("remote"), &RemoteId);
                    Add(_T("name"), &Name);
                    Add(_T("le"), &LowEnergy);
                    Add(_T("connected"), &Connected);
                    Add(_T("paired"), &Paired);
                    Add(_T("reason"), &Reason);
                    LocalId = copy.LocalId;
                    RemoteId = copy.RemoteId;
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
                        LocalId = source->LocalId();
                        RemoteId = source->RemoteId();
                        Name = source->Name();
                        LowEnergy = source->LowEnergy();
                        Connected = source->IsConnected();
                        Paired = source->IsPaired();
                    } else {
                        LocalId.Clear();
                        RemoteId.Clear();
                        Name.Clear();
                        LowEnergy.Clear();
                        Paired.Clear();
                        Connected.Clear();
                    }
                    return (*this);
                }
                Core::JSON::String LocalId;
                Core::JSON::String RemoteId;
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
            DeviceImpl(const Bluetooth::Address::type type, const uint16_t deviceId, const Bluetooth::Address& remote, const string& name)
                : _name(name)
                , _deviceId(deviceId)
                , _handle(~0)
                , _remote(remote)
                , _state(static_cast<state>((type == Bluetooth::Address::BREDR_ADDRESS      ? 0 : 
                                            (type == Bluetooth::Address::LE_RANDOM_ADDRESS  ? LOWENERGY : 
                                                                                              LOWENERGY|PUBLIC))))
                , _capabilities(~0)
                , _authentication(~0)
                , _oob_data(~0)
                , _interval(0)
                , _latency(0)
                , _timeout(0)
                , _callback(nullptr)
            {
                ::memset(_features, 0xFF, sizeof(_features));
            }
            ~DeviceImpl()
            {
            }

        public:
            uint32_t Callback(IBluetooth::IDevice::ICallback* callback) override {
                uint32_t result = Core::ERROR_UNAVAILABLE;
                _state.Lock();
                if (callback == nullptr) {
                    if (_callback != nullptr) {
                        _callback->Release();
                        _callback = nullptr;
                        result = Core::ERROR_NONE;
                    }
                }
                else if (_callback == nullptr) {
                    _callback = callback;
                    _callback->AddRef();
                    result = Core::ERROR_NONE;
                }
                _state.Unlock();
                return (result);
            }
            uint32_t Pair(const IBluetooth::IDevice::capabilities caps) override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(PAIRING) == Core::ERROR_NONE) {
                    Bluetooth::Address::type type((_state & LOWENERGY) != 0 ? Bluetooth::Address::LE_PUBLIC_ADDRESS : 
                                                                               Bluetooth::Address::BREDR_ADDRESS);

                    result = BluetoothControl::Connector().Pair(_remote, type, static_cast<Bluetooth::ManagementSocket::capabilities>(caps));

                    if (result == Core::ERROR_NONE) {
                        _state.SetState(static_cast<state>(_state.GetState() | PAIRED));
                    } else {
                        printf("Failed to pair!!! Error: %d\n", result);
                    }
                    ClearState(PAIRING);
                }
                return (result);
            }
            uint32_t Unpair() override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(UNPAIRING) == Core::ERROR_NONE) {
                    result = BluetoothControl::Connector().Unpair(_remote);

                    if (result == Core::ERROR_NONE) {
                        _state.SetState(static_cast<state>(_state.GetState() & (~PAIRED)));
                    }
                    else {
                        printf("Failed to unpair!!! Error: %d\n", result);
                    }
                    ClearState(UNPAIRING);
                }
                return (result);
            }
            bool HasDisappeared() const override
            {
                return ((_state & DECOUPLED) != 0);
            }
            IBluetooth::IDevice::type Type () const override {
                return ( (_state & (LOWENERGY|PUBLIC)) == 0         ? IBluetooth::IDevice::ADDRESS_BREDR     :
                         (_state & (LOWENERGY|PUBLIC)) == LOWENERGY ? IBluetooth::IDevice::ADDRESS_LE_RANDOM :
                                                                      IBluetooth::IDevice::ADDRESS_LE_PUBLIC );
            }
            string LocalId() const override
            {
                return (Bluetooth::Address(_deviceId).ToString());
            }
            string RemoteId() const override
            {
                return (_remote.ToString());
            }
            string Name() const override
            {
                return (_name);
            }
            bool IsConnected() const override
            {
                return (_handle != static_cast<uint16_t>(~0));
            }
            bool IsPaired() const override
            {
                return ((_state & PAIRED) != 0);
            }
            bool IsDiscovered() const override 
            {
                return ((_features[0] != 0xFF) && (_features[1] != 0xFF) && (_features[2] != 0xFF) && (_features[3] != 0xFF));
            }
            inline uint16_t DeviceId() const 
            {
                return (_deviceId);
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
 
            void Capabilities(const uint8_t capability, const uint8_t authentication, const uint8_t oob_data) {
                _capabilities = capability;
                _authentication = authentication;
                _oob_data = oob_data;
            }
            uint16_t ConnectionId() const {
                return (_handle);
            }

        protected:
            friend class ControlSocket;

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
            void Connection (const uint16_t handle, const uint8_t role)
            {
                IBluetooth::IDevice::ICallback* callback = nullptr;

                _state.Lock();

                if ( (_handle == static_cast<uint16_t>(~0)) ^ (handle == static_cast<uint16_t>(~0)) ) {

                    TRACE(DeviceFlow, (_T("The connection state changed to: %d"), handle));
                    // This is a proper, Connect or disconnect. A real state change..
                    _handle = handle;
                }
                else {
                    TRACE(DeviceFlow, (_T("The connection handle is changed during the runtime. from: %d to: %d"), _handle, handle));
                    _handle = handle;
                }

                if (_callback != nullptr) {
                    callback = _callback;
                    callback->AddRef();
                }
 
                _state.Unlock();

                if (callback != nullptr) {
                    callback->Updated();
                    callback->Release();
                }
            }
            void Name(const string& name)
            {
                TRACE(DeviceFlow, (_T("The device name is updated to: %s"), name.c_str()));
                _name = name;
            }
            void Connection (const uint16_t interval, const uint16_t latency, const uint16_t timeout) 
            {
                IBluetooth::IDevice::ICallback* callback = nullptr;

                _state.Lock();

                TRACE(DeviceFlow, (_T("The device connection is updated with: Interval=%d, latency=%d, timeout=%d"), interval, latency, timeout));

                if (_callback != nullptr) {
                    callback = _callback;
                    callback->AddRef();
                }

                _interval = interval;
                _latency = latency;
                _timeout = timeout;
 
                _state.Unlock();

                if (callback != nullptr) {
                    callback->Updated();
                    callback->Release();
                }
 
            }
            void Features(const uint8_t length, const uint8_t feature[])
            {
                uint8_t copyLength = std::min(length, static_cast<uint8_t>(sizeof(_features)));
                IBluetooth::IDevice::ICallback* callback = nullptr;

                _state.Lock();

                if (::memcmp(_features, feature, copyLength) != 0) {
                    if ( (_features[0] == 0xFF) && (_features[1] == 0xFF) && (_features[2] == 0xFF) && (_features[3] == 0xFF) ) {
                        TRACE(DeviceFlow, (_T("The device features are set")));
                    }
                    else {
                        TRACE(DeviceFlow, (_T("The device features are updated")));
                    }

                    ::memcpy(_features, feature, copyLength);
                }
                else {
                    TRACE(DeviceFlow, (_T("Same device features are offered again")));
                }

                    
                if (_callback != nullptr) {
                    callback = _callback;
                    callback->AddRef();
                }

                _state.Unlock();

                if (callback != nullptr) {
                    callback->Updated();
                    callback->Release();
                }
 
            }
 
        private:
            string _name;
            Bluetooth::Address::type _type;
            uint16_t _deviceId;
            uint16_t _handle;
            Bluetooth::Address _remote;
            Core::StateTrigger<state> _state;
            uint8_t _features[8];
            uint8_t _capabilities;
            uint8_t _authentication;
            uint8_t _oob_data;
            uint16_t _interval;
            uint16_t _latency;
            uint16_t _timeout;
            IBluetooth::IDevice::ICallback* _callback;
        };

        class EXTERNAL DeviceRegular : public DeviceImpl, Core::IOutbound::ICallback {
        private:
            DeviceRegular() = delete;
            DeviceRegular(const DeviceRegular&) = delete;
            DeviceRegular& operator=(const DeviceRegular&) = delete;

        public:
            DeviceRegular(const uint16_t deviceId, const Bluetooth::Address& address, const string& name)
                : DeviceImpl(Bluetooth::Address::BREDR_ADDRESS, deviceId, address, name)
            {
                _name.Clear();
                _name->bdaddr = *(Locator().Data());
                _name->pscan_mode = 0x00;
                _name->pscan_rep_mode = 0x02;
                _name->clock_offset = 0x0000;
                BluetoothControl::Connector().Send(MAX_ACTION_TIMEOUT, _name, this, &_name);
            }
            virtual ~DeviceRegular()
            {
            }

        public:
            virtual uint32_t Connect() override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(CONNECTING) == Core::ERROR_NONE) {
                    Bluetooth::HCISocket::Command::Connect connect;

                    connect.Clear();
                    connect->bdaddr = *(Locator().Data());
                    connect->pkt_type = htobs(HCI_DM1 | HCI_DM3 | HCI_DM5 | HCI_DH1 | HCI_DH3 | HCI_DH5);
                    connect->pscan_rep_mode = 0x02;
                    connect->clock_offset = 0x0000;
                    connect->role_switch = 0x01;

                    result = BluetoothControl::Connector().Exchange(MAX_ACTION_TIMEOUT, connect, connect);

                    if (result == Core::ERROR_NONE) {
                        Connection (connect.Response().handle, 0);
                    }
                    else {
                        TRACE(ControlFlow, (_T("Failed to connect. Error [%d]"), result));
                    }

                    ClearState(CONNECTING);
                }

                return (result);
            }
            virtual uint32_t Disconnect(const uint16_t reason) override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(DISCONNECTING) == Core::ERROR_NONE) {
                    Bluetooth::HCISocket::Command::Disconnect disconnect;

                    disconnect->handle = htobs(ConnectionId());
                    disconnect->reason = (reason & 0xFF);

                    result = BluetoothControl::Connector().Exchange(MAX_ACTION_TIMEOUT, disconnect, disconnect);

                    if (result == Core::ERROR_NONE) {
                        Connection(~0, 0);
                    }
                    else {
                        TRACE(ControlFlow, (_T("Failed to disconnect. Error [%d]"), result));
                    }

                    ClearState(DISCONNECTING);
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

                    Name(std::string(longName, index));
                    TRACE(ControlFlow, (_T("Loaded Long Device Name: %s"),longName));
                    ClearState(METADATA);
                }
            }

        private:
            Bluetooth::HCISocket::Command::RemoteName _name;
        };

        class EXTERNAL DeviceLowEnergy : public DeviceImpl, Core::IOutbound::ICallback {
        private:
            DeviceLowEnergy() = delete;
            DeviceLowEnergy(const DeviceLowEnergy&) = delete;
            DeviceLowEnergy& operator=(const DeviceLowEnergy&) = delete;

        public:
            DeviceLowEnergy(const uint16_t deviceId, const Bluetooth::Address& address, const string& name)
                : DeviceImpl(Bluetooth::Address::LE_PUBLIC_ADDRESS, deviceId, address, name)
            {
            }
            virtual ~DeviceLowEnergy()
            {
            }

        public:
            virtual uint32_t Connect() override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(CONNECTING) == Core::ERROR_NONE) {

                    Bluetooth::HCISocket::Command::ConnectLE connect;

                    connect.Clear();
                    connect->interval = htobs(0x0004);
                    connect->window = htobs(0x0004);
                    connect->initiator_filter = 0;
                    connect->peer_bdaddr_type = LE_PUBLIC_ADDRESS;
                    connect->peer_bdaddr = *(Locator().Data());
                    connect->own_bdaddr_type = LE_PUBLIC_ADDRESS;
                    connect->min_interval = htobs(0x000F);
                    connect->max_interval = htobs(0x000F);
                    connect->latency = htobs(0x0000);
                    connect->supervision_timeout = htobs(0x0C80);
                    connect->min_ce_length = htobs(0x0001);
                    connect->max_ce_length = htobs(0x0001);

                    result = BluetoothControl::Connector().Exchange(MAX_ACTION_TIMEOUT, connect, connect);

                    if (result == Core::ERROR_NONE) {
                        Connection(connect.Response().handle, connect.Response().role);
                    }
                    else {
                        TRACE(ControlFlow, (_T("Failed to connect. Error [%d]"), result));
                    }

                    ClearState(CONNECTING);
 
                }

                TRACE_L1("DeviceLowEnergy Connect() status <%i>\n", result);

                return (result);
            }
            virtual uint32_t Disconnect(const uint16_t reason) override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(DISCONNECTING) == Core::ERROR_NONE) {

                    Bluetooth::HCISocket::Command::Disconnect disconnect;

                    disconnect->handle = htobs(ConnectionId());
                    disconnect->reason = reason & 0xFF;

                    result = BluetoothControl::Connector().Exchange(MAX_ACTION_TIMEOUT, disconnect, disconnect);

                    if (result == Core::ERROR_NONE) {
                        Connection(~0, 0);
                    }
                    else {
                        TRACE(ControlFlow, (_T("Failed to disconnect. Error [%d]"), result));
                    }

                    ClearState(DISCONNECTING);
 
                }

                return (result);
            }

        private:
            virtual void Updated(const Core::IOutbound& data, const uint32_t error_code) override
            {
            }
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
        BluetoothControl(const BluetoothControl&) = delete;
        BluetoothControl& operator=(const BluetoothControl&) = delete;
        BluetoothControl()
            : _skipURL(0)
            , _adminLock()
            , _service(nullptr)
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

        inline static ControlSocket& Connector() {
            return(_application);
        }

    private:
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        void RemoveDevices(std::function<bool(DeviceImpl*)> filter);
        void Discovered(const bool lowEnergy, const Bluetooth::Address& address, const string& name);
        void Notification(const uint8_t subEvent, const uint16_t length, const uint8_t* dataFrame);
        DeviceImpl* Find(const Bluetooth::Address&);
        DeviceImpl* Find(const uint16_t handle);
        void Capabilities(const Bluetooth::Address& device, const uint8_t capability, const uint8_t authentication, const uint8_t oob_data);

    private:
        uint8_t _skipURL;
        Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        uint16_t _btInterface;
        Bluetooth::Address _btAddress;
        std::list<DeviceImpl*> _devices;
        std::list<IBluetooth::INotification*> _observers;
        std::list<GATTRemote> _gattRemotes;
        Config _config;
        static ControlSocket _application;
    };
} //namespace Plugin

} //namespace Solution
