#pragma once

#include "Module.h"

#include <interfaces/IBluetooth.h>
#include <interfaces/IKeyHandler.h>
#include <interfaces/json/JsonData_BluetoothRemoteControl.h>

namespace WPEFramework {

namespace Plugin {

    class BluetoothRemoteControl : public PluginHost::IPlugin
                                 , public PluginHost::IWeb
                                 , public PluginHost::JSONRPC
                                 , public Exchange::IKeyHandler {

        struct IGattNotificationHandler {
            virtual bool Notification(const uint32_t cookie, const uint8_t buffer[], const uint16_t length) = 0;
        };

        class GATTRemote : public Bluetooth::GATTSocket {
        private:
            static constexpr uint16_t HID_UUID         = 0x1812;

            class Flow {
            public:
                Flow() = delete;
                Flow(const Flow&) = delete;
                Flow& operator=(const Flow&) = delete;
                Flow(const TCHAR formatter[], ...)
                {
                    va_list ap;
                    va_start(ap, formatter);
                    Trace::Format(_text, formatter, ap);
                    va_end(ap);
                }
                explicit Flow(const string& text)
                    : _text(Core::ToString(text))
                {
                }
                ~Flow()
                {
                }

            public:
                const char* Data() const
                {
                    return (_text.c_str());
                }
                uint16_t Length() const
                {
                    return (static_cast<uint16_t>(_text.length()));
                }

            private:
                std::string _text;
            };

            class Sink : public Exchange::IBluetooth::IDevice::ICallback {
            public:
                Sink() = delete;
                Sink(const Sink&) = delete;
                Sink& operator=(const Sink&) = delete;
                Sink(GATTRemote* parent) : _parent(*parent)
                {
                    ASSERT(parent != nullptr);
                }
                ~Sink() override
                {
                }

            public:
                void Updated() override
                {
                    _parent.Updated();
                }

                BEGIN_INTERFACE_MAP(Sink)
                    INTERFACE_ENTRY(Exchange::IBluetooth::IDevice::ICallback)
                END_INTERFACE_MAP

            private:
                GATTRemote& _parent;
            };

            static Core::NodeId Designator(const uint8_t type, const string& address)
            {
                return (Bluetooth::Address(address.c_str()).NodeId(static_cast<Bluetooth::Address::type>(type), GATTSocket::LE_ATT_CID, 0));
            }

        public:
            GATTRemote() = delete;
            GATTRemote(const GATTRemote&) = delete;
            GATTRemote& operator=(const GATTRemote&) = delete;

            class Config : public Core::JSON::Container {
            public:
                class Entry : public Core::JSON::Container {
                public:
                    Entry& operator=(const Entry&) = delete;
                    Entry()
                        : Core::JSON::Container()
                        , ServiceUuid()
                        , CharacteristicUuid()
                        , Handle(0)
                    {
                        Init();
                    }
                    Entry(const Entry& copy)
                        : Core::JSON::Container()
                        , ServiceUuid()
                        , CharacteristicUuid()
                        , Handle(0)
                    {
                        ServiceUuid = copy.ServiceUuid;
                        CharacteristicUuid = copy.CharacteristicUuid;
                        Handle = copy.Handle;
                        Init();
                    }
                    ~Entry()
                    {
                    }

                private:
                    void Init()
                    {
                        Add(_T("service"), &ServiceUuid);
                        Add(_T("characteristic"), &CharacteristicUuid);
                        Add(_T("handle"), &Handle);
                    }

                public:
                    Core::JSON::String ServiceUuid;
                    Core::JSON::String CharacteristicUuid;
                    Core::JSON::DecUInt16 Handle;
                };

                Config(const Config&) = delete;
                Config& operator=(const Config&) = delete;
                Config()
                    : Core::JSON::Container()
                    , Handles()
                {
                    Add(_T("handles"), &Handles);
                }
                ~Config()
                {
                }

            public:
                Core::JSON::ArrayType<Entry> Handles;
            };

            GATTRemote(BluetoothRemoteControl* parent, Exchange::IBluetooth::IDevice* device, uint16_t mtu = 255)
                : Bluetooth::GATTSocket(Designator(device->Type(), device->LocalId()), Designator(device->Type(), device->RemoteId()), mtu)
                , _adminLock()
                , _profile(true)
                , _command()
                , _device(device)
                , _sink(this)
                , _parent(parent)
                , _name()
                , _modelNumber()
                , _serialNumber()
                , _firmwareRevision()
                , _softwareRevision()
                , _manufacturerName()
                , _deviceInfoRead(false)
                , _reportHandles()
                , _handles()
                , _subscribers()
                , _handlers()
            {
                ASSERT(_parent != nullptr);
                ASSERT(_device != nullptr);

                _device->AddRef();
                _name = _device->Name();

                if (_device->Callback(&_sink) != Core::ERROR_NONE) {
                    TRACE(Trace::Fatal, (_T("The device is already in use. Only 1 callback allowed")));
                }
                else if (_device->IsConnected() == true) {
                    TRACE(Trace::Fatal, (_T("The device is already connected. First disconnect the device")));
                }

                _parent->RemoteCreated(*this);
            }

            virtual ~GATTRemote()
            {
                _parent->RemoteDestroyed(*this);

                if (GATTSocket::IsOpen() == true) {
                    GATTSocket::Close(Core::infinite);
                }

                if (_device != nullptr) {
                    if (_device->Callback(nullptr) != Core::ERROR_NONE) {
                        TRACE(Trace::Fatal, (_T("Could not remove the callback from the device")));
                    }

                    _device->Release();
                    _device = nullptr;
                }
            }

        public:
            void Configure(const string& settings)
            {
                Config config;
                if (config.FromString(settings) == true) {
                    _adminLock.Lock();
                    _handles.clear();
                    _handlers.clear();

                    // Reload characteristic handles
                    Core::JSON::ArrayType<Config::Entry>::Iterator index(config.Handles.Elements());
                    while (index.Next() == true) {
                        _handles.emplace_back(index.Current().ServiceUuid.Value(), index.Current().CharacteristicUuid.Value(), index.Current().Handle.Value());
                    }

                    // Recreate handlers
                    for (auto entry : _subscribers) {
                        auto it = std::find_if(_handles.begin(), _handles.end(), [&](const std::tuple<Bluetooth::UUID, Bluetooth::UUID, uint16_t> val) {
                            return ((std::get<0>(entry) == std::get<0>(val)) && (std::get<1>(entry) == std::get<1>(val)));
                        });
                        if (it != _handles.end()) {
                            _handlers.emplace(std::get<2>(*it), std::make_pair(std::get<2>(entry), std::get<3>(entry)));
                        }
                    }
                    _adminLock.Unlock();
                }
            }

            void Configuration(string& settings) const
            {
                Config config;
                for (auto& entry : _handles) {
                    Config::Entry e;
                    e.ServiceUuid = std::get<0>(entry).ToString();
                    e.CharacteristicUuid = std::get<1>(entry).ToString();
                    e.Handle = std::get<2>(entry);
                    config.Handles.Add(e);
                }
                config.ToString(settings);
            }

        public:
            uint32_t AddHandler(const Bluetooth::UUID& service, const Bluetooth::UUID& characteristic, IGattNotificationHandler* handler, const uint32_t cookie = 0)
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;
                _adminLock.Lock();
                if (std::find_if(_subscribers.begin(), _subscribers.end(),
                                        [&](const std::tuple<Bluetooth::UUID, Bluetooth::UUID, IGattNotificationHandler*, uint32_t>& val) -> bool {
                                            return ((service == std::get<0>(val)) && (characteristic == std::get<1>(val)));
                                        }
                                ) == _subscribers.end()) {
                    _subscribers.emplace_back(service, characteristic, handler, cookie);
                    result = Core::ERROR_NONE;
                }
                _adminLock.Unlock();
                return (result);
            }

            uint32_t RemoveHandler(const Bluetooth::UUID& service, const Bluetooth::UUID& characteristic, IGattNotificationHandler* handler)
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;
                _adminLock.Lock();
                auto it = std::find_if(_subscribers.begin(), _subscribers.end(),
                                        [&](const std::tuple<Bluetooth::UUID, Bluetooth::UUID, IGattNotificationHandler*, uint32_t>& val) -> bool {
                                            return ((service == std::get<0>(val)) && (characteristic == std::get<1>(val)));
                                        });
                if (it != _subscribers.end()) {
                    _subscribers.erase(it);
                    result = Core::ERROR_NONE;
                }
                _adminLock.Unlock();
                return (result);
            }

            string Name() const
            {
                return (_device != nullptr? _device->Name() :  _T("<unassigned>"));
            }

            string Address() const
            {
                return (_device != nullptr? _device->RemoteId() : _T("<unassigned>"));
            }

            string ModelNumber() const
            {
                return (_modelNumber);
            }

            string SerialNumber() const
            {
                return (_serialNumber);
            }

            string FirmwareRevision() const
            {
                return (_firmwareRevision);
            }

            string SoftwareRevision() const
            {
                return (_softwareRevision);
            }

            string ManufacturerName() const
            {
                return (_manufacturerName);
            }

            uint8_t BatteryLevel() const
            {
                return (_batteryLevel);
            }

        private:
            bool Initialize() override
            {
                return (Security(BT_SECURITY_LOW));
            }

            void Notification(const uint16_t handle, const uint8_t dataFrame[], const uint16_t length) override
            {
                bool handled = false;

                _adminLock.Lock();
                auto handlerIt = _handlers.find(handle);
                if (handlerIt != _handlers.end()) {
                    ASSERT((*handlerIt).second.first != nullptr);
                    if ((*handlerIt).second.first->Notification((*handlerIt).second.second, dataFrame, length) == true) {
                        handled = true;
                    }
                }
                _adminLock.Unlock();

                if (handled == false) {
                    string data;
                    Core::ToHexString(dataFrame, length, data);
                    TRACE(Trace::Information, (_T("Unhandled notification: [handle=0x%x], %d bytes: %s\n"), handle, length, data.c_str()));
                }
            }

            void Operational() override
            {
                TRACE(Flow, (_T("The received MTU: %d"), MTU()));

                if ((_device->IsBonded() == false) || (_handles.empty() == true)) {
                    // No need to do service discovery if device is bonded, the notifications are already enabled
                    _profile.Discover(CommunicationTimeOut * 20, *this, [&](const uint32_t result) {
                        if (result == Core::ERROR_NONE) {
                            if (_profile[Bluetooth::UUID(HID_UUID)] == nullptr) {
                                TRACE(Flow, (_T("The given bluetooth device does not support a HID service!!")));
                            }
                            else {
                                DumpProfile();

                                using Service = Bluetooth::Profile::Service;
                                const std::pair<uint16_t, uint16_t> uuidsList[] = {
                                    { Service::DeviceInformation, Service::Characteristic::ModelNumberString },
                                    { Service::DeviceInformation, Service::Characteristic::SerialNumberString },
                                    { Service::DeviceInformation, Service::Characteristic::FirmwareRevisionString },
                                    { Service::DeviceInformation, Service::Characteristic::SoftwareRevisionString },
                                    { Service::DeviceInformation, Service::Characteristic::ManufacturerNameString },
                                    { Service::BatteryService, Service::Characteristic::BatteryLevel }
                                };

                                _adminLock.Lock();

                                _reportHandles.clear();
                                _handles.clear();
                                _handlers.clear();

                                for (auto uuids : uuidsList) {
                                    Bluetooth::UUID serviceUuid = Bluetooth::UUID(uuids.first);
                                    Bluetooth::UUID characteristicUuid = Bluetooth::UUID(uuids.second);
                                    const Bluetooth::Profile::Service* service = _profile[serviceUuid];
                                    if (service != nullptr) {
                                        Bluetooth::Profile::Service::Iterator characteristicIdx = service->Characteristics();
                                        while (characteristicIdx.Next() == true) {
                                            const Bluetooth::Profile::Service::Characteristic& characteristic(characteristicIdx.Current());
                                            if (characteristic == characteristicUuid) {
                                                _handles.emplace_back(serviceUuid, characteristicUuid, characteristic.Handle());
                                                TRACE(Flow, (_T("Characteristic '%s' available at handle %04x"), characteristic.Name().c_str(), characteristic.Handle()));
                                            }
                                        }
                                    }
                                }

                                for (auto entry : _subscribers) {
                                    const Bluetooth::Profile::Service* service = _profile[std::get<0>(entry)];
                                    if (service != nullptr) {
                                        Bluetooth::Profile::Service::Iterator characteristicIdx = service->Characteristics();
                                        while (characteristicIdx.Next() == true) {
                                            const Bluetooth::Profile::Service::Characteristic& characteristic(characteristicIdx.Current());
                                            if (characteristic == std::get<1>(entry)) {
                                                TRACE(Flow, (_T("Subscriber requested characteristic '%s' available at handle %04X"), characteristic.Name().c_str(), characteristic.Handle()));
                                                using Descriptor = Bluetooth::Profile::Service::Characteristic::Descriptor;
                                                const Bluetooth::Profile::Service::Characteristic::Descriptor* desc = characteristic[Descriptor::ClientCharacteristicConfiguration];
                                                if (desc != nullptr) {
                                                    _handlers.emplace(characteristic.Handle(), std::make_pair(std::get<2>(entry), std::get<3>(entry)));
                                                    _reportHandles.push_back(desc->Handle());
                                                    if (std::find(_handles.begin(), _handles.end(), std::make_tuple(std::get<0>(entry), std::get<1>(entry), characteristic.Handle())) == _handles.end()) {
                                                        _handles.emplace_back(std::get<0>(entry), std::get<1>(entry), characteristic.Handle());
                                                    }
                                                } else {
                                                    TRACE(Flow, (_T("Characteristic '%s' does not include a CCCD"), characteristic.Name().c_str()));
                                                }
                                            }
                                        }
                                    }
                                }

                                _adminLock.Unlock();

                                if (_reportHandles.size() > 0) {
                                    uint16_t val = htobs(1);
                                    _command.Write(_reportHandles.front(), sizeof(val), reinterpret_cast<const uint8_t*>(&val));
                                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                                        EnableEvents(cmd);
                                    });
                                }

                                _parent->RemoteBonded(*this);
                            }
                        }
                        else {
                            TRACE(Flow, (_T("The given bluetooth device could not be read for services!!")));
                        }
                    });
                } else {
                    if (_deviceInfoRead == false) {
                        _deviceInfoRead = true;
                        ReadModelNumber();
                    } else {
                        ReadBatteryLevel();
                    }
                }
            }

            void DumpProfile() const
            {
                Bluetooth::Profile::Iterator serviceIdx = _profile.Services();
                while (serviceIdx.Next() == true) {
                    const Bluetooth::Profile::Service& service = serviceIdx.Current();
                    TRACE(Flow, (_T("[0x%04X] Service: [0x%04X]         %s"), service.Handle(), service.Max(), service.Name().c_str()));

                    Bluetooth::Profile::Service::Iterator characteristicIdx = service.Characteristics();
                    while (characteristicIdx.Next() == true) {
                        const Bluetooth::Profile::Service::Characteristic& characteristic(characteristicIdx.Current());
                        TRACE(Flow, (_T("[0x%04X]    Characteristic [0x%02X]: %s [%d]"), characteristic.Handle(), characteristic.Rights(), characteristic.Name().c_str(), characteristic.Error()));

                        Bluetooth::Profile::Service::Characteristic::Iterator descriptorIdx = characteristic.Descriptors();
                        while (descriptorIdx.Next() == true) {
                            const Bluetooth::Profile::Service::Characteristic::Descriptor& descriptor(descriptorIdx.Current());
                            TRACE(Flow, (_T("[0x%04X]       Descriptor:         %s"), descriptor.Handle(), descriptor.Name().c_str()));
                        }
                    }
                }
            }

            void EnableEvents(const GATTSocket::Command& cmd)
            {
                if ((cmd.Error() != Core::ERROR_NONE) || (cmd.Result().Error() != 0) ) {
                    TRACE(Flow, (_T("Failed to enable reporting on handle 0x%04X [%d]"), _reportHandles.front(), cmd.Result().Error()));
                }
                else {
                    TRACE(Flow, (_T("Enabled reporting on handle 0x%04X"), _reportHandles.front()));
                }

                _reportHandles.pop_front();

                if (_reportHandles.size() > 0) {
                    uint16_t val = htobs(1);
                    _command.Write(_reportHandles.front(), sizeof(val), reinterpret_cast<const uint8_t*>(&val));
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        EnableEvents(cmd);
                    });
                } else {
                    ReadModelNumber();
                }
            }


            uint16_t FindHandle(const Bluetooth::UUID& serviceUuid, const Bluetooth::UUID& charUuid) const
            {
                uint16_t handle = 0;
                auto it = std::find_if(_handles.begin(), _handles.end(), [&](const std::tuple<Bluetooth::UUID, Bluetooth::UUID, uint16_t>& val) {
                    return ((serviceUuid == std::get<0>(val)) && (charUuid == std::get<1>(val)));
                });
                if (it != _handles.end()) {
                    handle = std::get<2>(*it);
                }
                return (handle);
            };

            void ReadModelNumber()
            {
                uint16_t modelNumberHandle = FindHandle(Bluetooth::Profile::Service::DeviceInformation, Bluetooth::Profile::Service::Characteristic::ModelNumberString);
                if (modelNumberHandle != 0) {
                    _command.Read(modelNumberHandle);
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                            _modelNumber = string(reinterpret_cast<const char*>(cmd.Result().Data()), cmd.Result().Length());
                            TRACE(Trace::Information, (_T("ModelNumber: %s"), _modelNumber.c_str()));
                        } else {
                            TRACE(Flow, (_T("Failed to read model name")));
                        }

                        ReadSerialNumber();
                    });
                } else {
                    TRACE(Flow, (_T("ModelNumberString characteristic not available")));
                    ReadSerialNumber();
                }
            }

            void ReadSerialNumber()
            {
                uint16_t serialNumberHandle = FindHandle(Bluetooth::Profile::Service::DeviceInformation, Bluetooth::Profile::Service::Characteristic::SerialNumberString);
                if (serialNumberHandle != 0) {
                    _command.Read(serialNumberHandle);
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                            Core::ToHexString(cmd.Result().Data(), cmd.Result().Length(), _serialNumber);
                            TRACE(Flow, (_T("SerialNumber: %s"), _serialNumber.c_str()));
                        } else {
                            TRACE(Flow, (_T("Failed to read serial number")));
                        }

                        ReadFirmwareRevision();
                    });
                } else {
                    TRACE(Flow, (_T("SerialNumberString characteristic not available")));
                    ReadFirmwareRevision();
                }
            }

            void ReadFirmwareRevision()
            {
                uint16_t firmwareRevisionHandle = FindHandle(Bluetooth::Profile::Service::DeviceInformation, Bluetooth::Profile::Service::Characteristic::FirmwareRevisionString);
                if (firmwareRevisionHandle != 0) {
                    _command.Read(firmwareRevisionHandle);
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                            _firmwareRevision = string(reinterpret_cast<const char*>(cmd.Result().Data()), cmd.Result().Length());
                            TRACE(Trace::Information, (_T("FirmwareRevision: %s"), _firmwareRevision.c_str()));
                        } else {
                            TRACE(Flow, (_T("Failed to read firmware revision")));
                        }

                        ReadSoftwareRevision();
                    });
                } else {
                    TRACE(Flow, (_T("FirmwareRevisionString characteristic not available")));
                    ReadSoftwareRevision();
                }
            }

            void ReadSoftwareRevision()
            {
                uint16_t softwareRevisionHandle = FindHandle(Bluetooth::Profile::Service::DeviceInformation, Bluetooth::Profile::Service::Characteristic::SoftwareRevisionString);
                if (softwareRevisionHandle != 0) {
                    _command.Read(softwareRevisionHandle);
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                            _softwareRevision = string(reinterpret_cast<const char*>(cmd.Result().Data()), cmd.Result().Length());
                            TRACE(Flow, (_T("SoftwareRevision: %s"), _softwareRevision.c_str()));
                        } else {
                            TRACE(Flow, (_T("Failed to read software revision")));
                        }

                        ReadManufacturerName();
                    });
                } else {
                    TRACE(Flow, (_T("SoftwareRevisionString characteristic not available")));
                    ReadManufacturerName();
                }
            }

            void ReadManufacturerName()
            {
                uint16_t manufacturerNameHandle = FindHandle(Bluetooth::Profile::Service::DeviceInformation, Bluetooth::Profile::Service::Characteristic::ManufacturerNameString);
                if (manufacturerNameHandle != 0) {
                    _command.Read(manufacturerNameHandle);
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                            _manufacturerName = string(reinterpret_cast<const char*>(cmd.Result().Data()), cmd.Result().Length());
                            TRACE(Flow, (_T("ManufacturerName: %s"), _manufacturerName.c_str()));
                        } else {
                            TRACE(Flow, (_T("Failed to read manufacturer name")));
                        }

                        ReadBatteryLevel();
                    });
                } else {
                    TRACE(Flow, (_T("ManufacturerNameString characteristic not available")));
                    ReadBatteryLevel();
                }
            }

            void ReadBatteryLevel()
            {
                uint16_t batteryLevelHandle = FindHandle(Bluetooth::Profile::Service::BatteryService, Bluetooth::Profile::Service::Characteristic::BatteryLevel);
                if (batteryLevelHandle != 0) {
                    _command.Read(batteryLevelHandle);
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                            _batteryLevel = cmd.Result().Data()[0];
                            TRACE(Flow, (_T("Battery level is %i%%"), _batteryLevel));
                            _parent->event_batterylevelchange(_batteryLevel);
                        } else {
                            TRACE(Flow, (_T("Failed to retrieve battery level")));
                        }
                    });
                } else {
                    TRACE(Flow, (_T("BatteryLevel characteristic not available")));
                }
            }

            void Updated()
            {
                _adminLock.Lock();
                if (_device != nullptr) {
                    if ((_device->IsConnected() == true) && (IsOpen() == false)) {
                        TRACE(Trace::Information, (_T("Connecting GATT socket [%s]"), _device->RemoteId().c_str()));
                        uint32_t result = GATTSocket::Open(5000);
                        if (result != Core::ERROR_NONE) {
                            TRACE(Trace::Error, (_T("Failed to open GATT socket [%s]"), _device->RemoteId().c_str()));
                        }
                    } else if (_device->IsValid() == true) {
                        if (IsOpen() == true) {
                            TRACE(Trace::Information, (_T("Disconnecting GATT socket [%s]"), _device->RemoteId().c_str()));
                            uint32_t result = GATTSocket::Close(Core::infinite);
                            if (result != Core::ERROR_NONE) {
                                TRACE(Trace::Error, (_T("Failed to close GATT socket [%s]"), _device->RemoteId().c_str()));
                            }
                        }
                    } else {
                        TRACE(Flow, (_T("Releasing device")));
                        _device->Release();
                        _device = nullptr;
                    }
                }
                _adminLock.Unlock();
            }

        private:
            Core::CriticalSection _adminLock;
            Bluetooth::Profile _profile;
            Command _command;
            Exchange::IBluetooth::IDevice* _device;
            Core::Sink<Sink> _sink;
            BluetoothRemoteControl* _parent;
            string _name;
            string _modelNumber;
            string _serialNumber;
            string _firmwareRevision;
            string _softwareRevision;
            string _manufacturerName;
            uint8_t _batteryLevel;
            bool _deviceInfoRead;
            std::list<uint16_t> _reportHandles;
            std::list<std::tuple<Bluetooth::UUID, Bluetooth::UUID, uint16_t>> _handles;
            std::list<std::tuple<Bluetooth::UUID, Bluetooth::UUID, IGattNotificationHandler*, uint32_t>> _subscribers;
            std::map<uint16_t, std::pair<IGattNotificationHandler*, uint32_t>> _handlers;
        }; // class GATTRemote

        class HidHandler : public Core::IDispatch
                         , public Exchange::IKeyProducer
                         , public IGattNotificationHandler {
        public:
            HidHandler(const HidHandler &) = delete;
            HidHandler &operator=(const HidHandler &) = delete;
            HidHandler(BluetoothRemoteControl* parent)
                : _parent(parent)
                , _remote(nullptr)
                , _pressed(false)
                , _scancode(0)
                , _currentKey(0)
                , _map()
                , _keyHandler(nullptr)
                , _adminLock()
            {
                ASSERT(_parent != nullptr);
            }
            ~HidHandler() override
            {
            }

        public:
            uint32_t Initialize(GATTRemote* remote)
            {
                uint32_t result = Core::ERROR_GENERAL;
                _adminLock.Lock();
                ASSERT(remote != nullptr);
                ASSERT(_remote == nullptr);
                if ((remote != nullptr) && (_remote == nullptr)) {
                    _remote = remote;
                    _remote->AddHandler(Bluetooth::Profile::Service::HumanInterfaceDevice, Bluetooth::Profile::Service::Characteristic::Report, this);
                    _map = _remote->Name();
                    result = Core::ERROR_NONE;
                }
                _adminLock.Unlock();
                return (result);
            }

            uint32_t Deinitialize()
            {
                uint32_t result = Core::ERROR_GENERAL;
                _adminLock.Lock();
                ASSERT(_remote != nullptr);
                if (_remote != nullptr) {
                    _remote->AddHandler(Bluetooth::Profile::Service::HumanInterfaceDevice, Bluetooth::Profile::Service::Characteristic::Report, this);
                    _remote = nullptr;
                    result = Core::ERROR_NONE;
                }
                _adminLock.Unlock();
                return (result);
            }

            bool Notification(const uint32_t cookie, const uint8_t buffer[], const uint16_t length) override
            {
                if (length >= 2) {
                    uint16_t scancode = ((buffer[1] << 8) | buffer[0]);
                    bool pressed = (scancode != 0);

                    if (pressed == true) {
                        _currentKey = scancode;
                    }

                    if (_currentKey != 0) {
                        KeyEvent(pressed, _currentKey);
                    }

                    return (true);
                }

                return (false);
            }

        public:
            // IKeyProducer methods
            const TCHAR* Name() const override
            {
                return _map.c_str();
            }

            bool Pair() override
            {
                return (false);
            }

            bool Unpair(string bondingId) override
            {
                return (false);
            }

            uint32_t Callback(IKeyHandler* keyHandler) override
            {
                _adminLock.Lock();
                if (_keyHandler != nullptr) {
                    _keyHandler->Release();
                }
                if (keyHandler != nullptr) {
                    keyHandler->AddRef();
                }
                _keyHandler = keyHandler;
                _adminLock.Unlock();

                return (Core::ERROR_NONE);
            }

            uint32_t Error() const override
            {
                return (Core::ERROR_NONE);
            }

            string MetaData() const override
            {
                return {};
            }

            void Configure(const string& settings) override
            {
            }

            void Dispatch() override
            {
                Exchange::IKeyHandler* keyhandler = nullptr;

                _adminLock.Lock();
                if (_keyHandler != nullptr) {
                    keyhandler = _keyHandler;
                    keyhandler->AddRef();
                }
                _adminLock.Unlock();

                if (keyhandler != nullptr) {
                    keyhandler->KeyEvent(_pressed, _scancode, _map);
                    keyhandler->Release();
                }
            }

            BEGIN_INTERFACE_MAP(HidHandler)
                INTERFACE_ENTRY(Exchange::IKeyProducer)
            END_INTERFACE_MAP

        private:
            void KeyEvent(bool pressed, uint16_t scancode)
            {
                TRACE(Trace::Information, (_T("Received a keypress notification: code=%i, state=%s, keymap='%s'"), _currentKey, (pressed? _T("pressed") : _T("released")), _map.c_str()));
                _adminLock.Lock();
                if ((scancode != 0) && (_keyHandler != nullptr)) {
                    _pressed = pressed;
                    _scancode = scancode;
                    PluginHost::WorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                }
                _adminLock.Unlock();
            }

        private:
            BluetoothRemoteControl* _parent;
            GATTRemote* _remote;
            bool _pressed;
            uint16_t _scancode;
            uint16_t _currentKey;
            string _map;
            Exchange::IKeyHandler* _keyHandler;
            mutable Core::CriticalSection _adminLock;
        }; // class HidHandler

        class BatteryLevelHandler : public IGattNotificationHandler {
        public:
            BatteryLevelHandler(const BatteryLevelHandler &) = delete;
            BatteryLevelHandler &operator=(const BatteryLevelHandler &) = delete;
            BatteryLevelHandler(BluetoothRemoteControl* parent)
                : _adminLock()
                , _parent(parent)
                , _remote(nullptr)
                , _level(~0)
            {
                ASSERT(_parent != nullptr);
            }
            ~BatteryLevelHandler()
            {
            }

        public:
            uint32_t Initialize(GATTRemote* remote)
            {
                uint32_t result = Core::ERROR_GENERAL;
                _adminLock.Lock();
                ASSERT(remote != nullptr);
                ASSERT(_remote == nullptr);
                if ((remote != nullptr) && (_remote == nullptr)) {
                    _remote = remote;
                    _remote->AddHandler(Bluetooth::Profile::Service::BatteryService, Bluetooth::Profile::Service::Characteristic::BatteryLevel, this);
                    result = Core::ERROR_NONE;
                }
                _adminLock.Unlock();
                return (result);
            }

            uint32_t Deinitialize()
            {
                uint32_t result = Core::ERROR_GENERAL;
                _adminLock.Lock();
                ASSERT(_remote != nullptr);
                if (_remote != nullptr) {
                    _remote->RemoveHandler(Bluetooth::Profile::Service::BatteryService, Bluetooth::Profile::Service::Characteristic::BatteryLevel, this);
                    _remote = nullptr;
                    result = Core::ERROR_NONE;
                }
                _adminLock.Unlock();
                return (result);
            }

            void Level(const uint8_t level)
            {
                TRACE(Trace::Information, (_T("Remote control battery level is %i%%"), level));
                _level = level;
                _parent->event_batterylevelchange(level);
            }

            uint8_t Level() const
            {
                if (_level == static_cast<uint8_t>(~0)) {
                    return (_remote->BatteryLevel());
                } else {
                    return (_level);
                }
            }

            bool Notification(const uint32_t cookie, const uint8_t buffer[], const uint16_t length) override
            {
                if (length >= 1) {
                    Level(buffer[0]);
                    return (true);
                }
                return (false);
            }

        private:
            mutable Core::CriticalSection _adminLock;
            BluetoothRemoteControl* _parent;
            GATTRemote* _remote;
            uint8_t _level;
        }; // class BatteryLevelHandler

        class AudioHandler : public IGattNotificationHandler {
        private:
            enum : uint32_t {
                COMMAND_EVENT,
                DATA_EVENT
            };

            class Config : public Core::JSON::Container {
            public:
                Config(const Config&) = delete;
                Config& operator=(const Config&) = delete;
                Config()
                    : Core::JSON::Container()
                    , ServiceUUID()
                    , CommandUUID()
                    , DataUUID()
                {
                    Add(_T("serviceuuid"), &ServiceUUID);
                    Add(_T("commanduuid"), &CommandUUID);
                    Add(_T("datauuid"), &DataUUID);
                }
                ~Config()
                {
                }

            public:
                Core::JSON::String ServiceUUID;
                Core::JSON::String CommandUUID;
                Core::JSON::String DataUUID;
            };

        public:
            AudioHandler(const AudioHandler &) = delete;
            AudioHandler &operator=(const AudioHandler &) = delete;
            AudioHandler(BluetoothRemoteControl* parent)
                : _adminLock()
                , _parent(parent)
                , _remote(nullptr)
                , _commandHandle(0)
                , _dataHandle(0)
                , _seq(0)
                , _stepIdx(0)
                , _pred(0)
                , _intSeq(0)
                , _skip(true)
                , _eot(true)
            {
                ASSERT(parent != nullptr);
            }
            ~AudioHandler()
            {
            }

        public:
            uint32_t Initialize(GATTRemote* remote, const string& configuration)
            {
                uint32_t result = Core::ERROR_GENERAL;
                _adminLock.Lock();
                ASSERT(remote != nullptr);
                ASSERT(_remote == nullptr);
                if ((remote != nullptr) && (_remote == nullptr)) {
                    _remote = remote;

                    Config config;
                    if (config.FromString(configuration) == true) {
                        AUDIO_SERVICE_UUID = Bluetooth::UUID(config.ServiceUUID.Value());
                        AUDIO_COMMAND_UUID = Bluetooth::UUID(config.CommandUUID.Value());
                        AUDIO_DATA_UUID = Bluetooth::UUID(config.DataUUID.Value());

                        if ((AUDIO_SERVICE_UUID.IsValid() == true) && (AUDIO_COMMAND_UUID.IsValid() == true) && (AUDIO_DATA_UUID.IsValid() == true)) {
                            _remote->AddHandler(AUDIO_SERVICE_UUID, AUDIO_COMMAND_UUID, this, COMMAND_EVENT);
                            _remote->AddHandler(AUDIO_SERVICE_UUID, AUDIO_DATA_UUID, this, DATA_EVENT);
                        }
                    }

                    result = Core::ERROR_NONE;
                }
                _adminLock.Unlock();
                return (result);
            }

            bool Deinitialize()
            {
                uint32_t result = Core::ERROR_GENERAL;
                _adminLock.Lock();
                ASSERT(_remote != nullptr);
                if (_remote != nullptr) {
                    _remote->RemoveHandler(AUDIO_SERVICE_UUID, AUDIO_DATA_UUID, this);
                    _remote->RemoveHandler(AUDIO_SERVICE_UUID, AUDIO_COMMAND_UUID, this);
                    _remote = nullptr;
                    result = Core::ERROR_NONE;
                }
                _adminLock.Unlock();
                return (result);
            }

            bool Notification(const uint32_t event, const uint8_t dataFrame[], const uint16_t length) override
            {
                bool handled = false;

                if ((event == DATA_EVENT) && (length == 5)) {
                    const Header* hdr = reinterpret_cast<const Header*>(dataFrame);
                    handled = true;
                    _skip = false;
                    _seq = hdr->seq;
                    _stepIdx = hdr->step;
                    _pred = btohs(hdr->pred);
                    _compression = hdr->compression;
                    if ((_seq == 0) || (_eot == true)) {
                        _eot = false;
                        string profile = (_compression? "adpcm-hq" : "pcm");
                        // Audio transmission start event is delayed until the first header arrives
                        AudioTransmission(profile);
                    }
                    _intSeq = _seq;
                    _skip = false;
                } else if ((event == DATA_EVENT) && (length == 1)) {
                    handled = true;
                    // uint8_t seq = dataFrame[0];
                } else if ((event == DATA_EVENT) && (length != 5) && (length != 1)) {
                    handled = true;
                    if (_skip == false) {
                        uint8_t *buf = reinterpret_cast<uint8_t*>(ALLOCA(length + sizeof(Preamble)));
                        Preamble *preamble = reinterpret_cast<Preamble*>(buf);
                        preamble->step = _stepIdx;
                        preamble->pred = _pred;
                        preamble->pad = 0;
                        // Prepending the buffer with the preamble built from the header notification
                        ::memcpy(buf + sizeof(Preamble), dataFrame, length);
                        AudioFrame(_seq, buf, length + sizeof(Preamble));
                        _skip = false;
                    }
                } else if ((event == COMMAND_EVENT) && (length >= 1)) {
                    handled = true;
                    if (dataFrame[0] == 0) {
                        _eot = true;
                        AudioTransmission();
                    }
                }

                return (handled);
            }

            void AudioTransmission(const string& profile = "") const
            {
                TRACE(Trace::Information, (_T("Audio transmission: %s"), profile.empty() == false? profile.c_str() : "end"));
                _parent->event_audiotransmission(profile);
            }

            void AudioFrame(const uint16_t seq, const uint8_t buffer[], const uint16_t length) const
            {
                // send original compressed data
                string frame;
                Core::ToString(buffer, length, true, frame);
                _parent->event_audioframe(seq, frame);
            }

        private:
            struct __attribute__((packed)) Header {
                uint8_t seq;
                uint8_t step;
                uint16_t pred;
                uint8_t compression;
            };

            struct __attribute__((packed)) Preamble {
                // Conforms to MS IMA ADPCM preamble
                uint16_t pred; // little-endian
                uint8_t step;
                uint8_t pad;
            };

        private:
            mutable Core::CriticalSection _adminLock;
            BluetoothRemoteControl* _parent;
            GATTRemote* _remote;
            uint16_t _commandHandle;
            uint16_t _dataHandle;
            uint8_t _seq;
            uint8_t _stepIdx;
            uint8_t _pred;
            uint8_t _compression;
            uint8_t _intSeq;
            bool _skip;
            bool _eot;

            Bluetooth::UUID AUDIO_SERVICE_UUID;
            Bluetooth::UUID AUDIO_COMMAND_UUID;
            Bluetooth::UUID AUDIO_DATA_UUID;
        }; //class AudioHandler

    public:
        BluetoothRemoteControl(const BluetoothRemoteControl&) = delete;
        BluetoothRemoteControl& operator=(const BluetoothRemoteControl&) = delete;
        BluetoothRemoteControl()
            : _skipURL(0)
            , _adminLock()
            , _service(nullptr)
            , _config()
            , _gattRemote(nullptr)
            , _inputHandler(PluginHost::InputHandler::Handler())
            , _settingsPath()
            , _hidHandler(Core::ProxyType<HidHandler>::Create(this))
            , _batteryLevelHandler(this)
            , _audioHandler(this)

        {
            ASSERT(_inputHandler != nullptr);
            RegisterAll();
        }

        ~BluetoothRemoteControl() override
        {
            UnregisterAll();
        }

    public:
        BEGIN_INTERFACE_MAP(BluetoothRemoteControl)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(Exchange::IKeyHandler)
        END_INTERFACE_MAP

    public:
        //  IPlugin methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        //  IWeb methods
        void Inbound(Web::Request& request) override;
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

        // IKeyHandler methods
        uint32_t KeyEvent(const bool pressed, const uint32_t code, const string& mapName) override
        {
            return (_inputHandler->KeyEvent(pressed, code, mapName));
        }

        Exchange::IKeyProducer* Producer(const string& name) override
        {
            return (&(*_hidHandler));
        }

        void ProducerEvent(const string& /*producerName*/, const Exchange::ProducerEvents /*event*/) override
        {
        }

    private:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config()
                : Core::JSON::Container()
                , BtCtlCallsign("BluetoothControl")
                , DefaultKeymap("bluetooth")
                , MTU(0)
            {
                Add(_T("btctl"), &BtCtlCallsign);
                Add(_T("defaultkeymap"), &DefaultKeymap);
                Add(_T("mtu"), &MTU);
                Add(_T("audio"), &Audio);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String BtCtlCallsign;
            Core::JSON::String DefaultKeymap;
            Core::JSON::DecUInt16 MTU;
            Core::JSON::String Audio;
        };

        class Settings : public Core::JSON::Container {
        public:
            Settings(const Settings&) = delete;
            Settings& operator=(const Settings&) = delete;
            Settings()
                : Core::JSON::Container()
                , Name()
                , Address()
                , Gatt()
            {
                Init();
            }
            Settings(const string& name, const string& address, const string& gatt)
                : Core::JSON::Container()
                , Name()
                , Address()
                , Gatt()
            {
                Init();
                Name = name;
                Address = address;
                Gatt = gatt;
            }
            ~Settings()
            {
            }

        private:
            void Init()
            {
                Gatt.SetQuoted(false);
                Add(_T("name"), &Name);
                Add(_T("address"), &Address);
                Add(_T("gatt"), &Gatt);
            }

        public:
            Core::JSON::String Name;
            Core::JSON::String Address;
            Core::JSON::String Gatt;
        };

    private:
        // Bluetooth Remote Control implementation
        uint32_t Assign(const string& address);
        uint32_t Revoke();
        uint32_t Properties() const;
        uint8_t BatteryLevel() const;
        string Name() const;
        string Address() const;

        uint32_t LoadKeymap(GATTRemote& remote, const string& name);
        uint32_t LoadSettings(Settings& settings) const;
        uint32_t SaveSettings(const Settings& settings) const;
        void RemoteCreated(GATTRemote& remote);
        void RemoteDestroyed(GATTRemote& remote);
        void RemoteBonded(GATTRemote& remote);
        void RemoteUnbonded(GATTRemote& remote);

    private:
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& request);

    private:
        // JSON-RPC
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_assign(const JsonData::BluetoothRemoteControl::AssignParamsData& params);
        uint32_t endpoint_revoke();
        uint32_t get_name(Core::JSON::String& response) const;
        uint32_t get_address(Core::JSON::String& response) const;
        uint32_t get_info(JsonData::BluetoothRemoteControl::InfoData& response) const;
        uint32_t get_batterylevel(Core::JSON::DecUInt8& response) const;
        uint32_t get_audioprofiles(Core::JSON::ArrayType<Core::JSON::String>& response) const;
        uint32_t get_audioprofile(const string& index, JsonData::BluetoothRemoteControl::AudioprofileData& response) const;
        void event_audiotransmission(const string& profile = "");
        void event_audioframe(const uint32_t& seq, const string& data);
        void event_batterylevelchange(const uint8_t& level);

    private:
        uint8_t _skipURL;
        mutable Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        Config _config;
        GATTRemote* _gattRemote;
        PluginHost::VirtualInput* _inputHandler;
        string _settingsPath;
        const Core::ProxyType<HidHandler> _hidHandler;
        BatteryLevelHandler _batteryLevelHandler;
        AudioHandler _audioHandler;
    }; // class BluetoothRemoteControl

} //namespace Plugin

}
