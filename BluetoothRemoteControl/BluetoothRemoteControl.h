#pragma once

#include "Module.h"

#include <interfaces/IBluetooth.h>
#include <interfaces/IKeyHandler.h>

namespace WPEFramework {

namespace Plugin {

    class BluetoothRemoteControl : public PluginHost::IPlugin
                                 , public PluginHost::IWeb
                                 , public Exchange::IKeyHandler {

        class GATTRemote : public Bluetooth::GATTSocket
                         , public Exchange::IKeyProducer {
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

            class Activity : public Core::IDispatch {
            public:
                Activity(const Activity &) = delete;
                Activity &operator=(const Activity &) = delete;
                Activity()
                    : _pressed(false)
                    , _scancode(0)
                    , _map()
                    , _keyHandler(nullptr)
                    , _adminLock()
                {
                }
                ~Activity() override
                {
                }

            public:
                void KeyEvent(bool pressed, uint16_t scancode, const string& map)
                {
                    _adminLock.Lock();
                    if ((scancode != 0) && (_keyHandler != nullptr)) {
                        _pressed = pressed;
                        _scancode = scancode;
                        _map = map;
                        PluginHost::WorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                    }
                    _adminLock.Unlock();
                }

                uint32_t KeyHandler(Exchange::IKeyHandler* keyHandler)
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

            private:
                bool _pressed;
                uint16_t _scancode;
                string _map;
                Exchange::IKeyHandler* _keyHandler;
                Core::CriticalSection _adminLock;
            };

            static Core::NodeId Designator(const uint8_t type, const string& address)
            {
                return (Bluetooth::Address(address.c_str()).NodeId(static_cast<Bluetooth::Address::type>(type), GATTSocket::LE_ATT_CID, 0));
            }

        public:
            GATTRemote() = delete;
            GATTRemote(const GATTRemote&) = delete;
            GATTRemote& operator=(const GATTRemote&) = delete;

            GATTRemote(BluetoothRemoteControl* parent, Exchange::IBluetooth::IDevice* device, uint16_t mtu = 255)
                : Bluetooth::GATTSocket(Designator(device->Type(), device->LocalId()), Designator(device->Type(), device->RemoteId()), mtu)
                , _adminLock()
                , _profile(true)
                , _command()
                , _device(device)
                , _handles()
                , _batteryLevel(~0)
                , _batteryLevelHandle(0)
                , _sink(this)
                , _currentKey(0)
                , _activity(Core::ProxyType<Activity>::Create())
                , _parent(parent)
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

            BEGIN_INTERFACE_MAP(BluetoothRemoteControl)
                INTERFACE_ENTRY(Exchange::IKeyProducer)
            END_INTERFACE_MAP

        public:
            // IKeyProducer methods
            const TCHAR* Name() const override
            {
                return (_name.c_str());
            }

            bool Pair() override
            {
                return (false);
            }

            bool Unpair(string bondingId) override
            {
                return (false);
            }

            uint32_t Callback(IKeyHandler* callback) override
            {
                return (_activity->KeyHandler(callback));
            }

            uint32_t Error() const override
            {
                return 0;
            }

            string MetaData() const override
            {
                return {};
            }

            void Configure(const string& settings) override
            {
            }

        public:
            string Address() const
            {
                return (_device != nullptr? _device->RemoteId() :  _T("<unassigned>"));
            }

            uint32_t SetAudioUuids(const string& service, const string& command, const string& data)
            {
                uint32_t result = Core::ERROR_GENERAL;

                Bluetooth::UUID serviceUuid(service);
                Bluetooth::UUID commandUuid(command);
                Bluetooth::UUID dataUuid(data);

                if ((serviceUuid.IsValid() == true) && (commandUuid.IsValid() == true) && (dataUuid.IsValid() == true)) {
                    AUDIO_SERVICE_UUID = serviceUuid;
                    AUDIO_COMMAND_UUID = commandUuid;
                    AUDIO_DATA_UUID = dataUuid;
                    result = Core::ERROR_NONE;
                }

                return (result);
            }

            uint8_t BatteryLevel()
            {
                printf("battery state %i\n", _batteryLevel);
                if (_batteryLevel == static_cast<uint8_t>(~0)) {
                    printf("unknown battery state\n");
                    if (IsOpen() == false && _device->IsBonded() && _device->Connect() == Core::ERROR_NONE) {
                    printf("redoing discovery\n");
                       Discovery();
                    }
                }
                return (_batteryLevel);
            }

        private:
            bool Initialize() override
            {
                return (Security(BT_SECURITY_LOW));
            }

            void Notification(const uint16_t handle, const uint8_t dataFrame[], const uint16_t length) override
            {
                if ((handle == 0x34) && (length >= 2)) {
                    uint16_t scancode = ((dataFrame[1] << 8) | dataFrame[0]);
                    bool pressed = (scancode != 0);

                    if (pressed == true) {
                        _currentKey = scancode;
                    }

                    if (_currentKey != 0) {
                        TRACE(Flow, (_T("Received a keypress notification: handle=0x%x, code=%i, state=%s, map='%s'"), handle, _currentKey, (pressed? "pressed" : "released"), _name.c_str()));
                        ASSERT(_activity != nullptr);
                        _activity->KeyEvent(pressed, _currentKey, _name);
                    }
                } else {
                    string data;
                    Core::ToHexString(dataFrame, length, data);
                    printf(_T("Unhandled notification: [handle=0x%x], %d bytes: %s\n"), handle, length, data.c_str());
                }
            }

            void Discovery()
            {
                _profile.Discover(CommunicationTimeOut * 20, *this, [&](const uint32_t result) {
                    if (result == Core::ERROR_NONE) {
                        if (_profile[Bluetooth::UUID(HID_UUID)] == nullptr) {
                            TRACE(Flow, (_T("The given bluetooth device does not support a HID service!!")));
                        }
                        else {
                            LoadReportHandles();

                            // Find battery level handle
                            const Bluetooth::Profile::Service* batteryService = _profile[Bluetooth::Profile::Service::BatteryService];
                            if (batteryService != nullptr) {
                                const Bluetooth::Profile::Service::Characteristic* characteristic = (*batteryService)[Bluetooth::Profile::Service::Characteristic::BatteryLevel];
                                const Bluetooth::Profile::Service::Characteristic::Descriptor* descriptor = (*characteristic)[Bluetooth::Profile::Service::Characteristic::Descriptor::ClientCharacteristicConfiguration];
                                if (characteristic->Length() == 2) {
                                    _batteryLevel = static_cast<const uint8_t>(btohs(*reinterpret_cast<const uint16_t*>(characteristic->Value())));
                                    printf("battery level = %i\n",_batteryLevel);
                                }
                                _batteryLevelHandle = descriptor->Handle();
                                _handles.push_back(_batteryLevelHandle);
                            }

                            if (_handles.size() > 0) {
                                uint16_t val = htobs(1);
                                _command.Write(_handles.front(), sizeof(val), reinterpret_cast<const uint8_t*>(&val));
                                Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                                    EnableEvents(cmd);
                                });
                            }
                        }
                    }
                    else {
                        TRACE(Flow, (_T("The given bluetooth device could not be read for services!!")));
                    }
                });
            }

            void Operational() override
            {
                TRACE(Flow, (_T("The received MTU: %d"), MTU()));

                if (_device->IsBonded() == false) {
                    // No need to do service discovery if device is bonded, the notifications are already enabled
                    Discovery();
                }
            }

            void LoadReportHandles()
            {
                Bluetooth::Profile::Iterator serviceIdx = _profile.Services();
                while (serviceIdx.Next() == true) {
                    const Bluetooth::Profile::Service& service = serviceIdx.Current();
                    const bool isHidService = (service.Type() == Bluetooth::Profile::Service::HumanInterfaceDevice);
                    const bool isAudioService = (service.Type() == AUDIO_SERVICE_UUID);
                    TRACE(Flow, (_T("[0x%04X] Service: [0x%04X]         %s"), service.Handle(), service.Max(), service.Name().c_str()));

                    Bluetooth::Profile::Service::Iterator characteristicIdx = service.Characteristics();
                    while (characteristicIdx.Next() == true) {
                        const Bluetooth::Profile::Service::Characteristic& characteristic(characteristicIdx.Current());
                        const bool isHidReportCharacteristic = ((isHidService == true) && (characteristic.Type() == Bluetooth::Profile::Service::Characteristic::Report));
                        const bool isAudioCharacteristic = ((isAudioService == true) && ((characteristic.Type() == AUDIO_COMMAND_UUID) || (characteristic.Type() == AUDIO_DATA_UUID)));
                        TRACE(Flow, (_T("[0x%04X]    Characteristic [0x%02X]: %s [%d]"), characteristic.Handle(), characteristic.Rights(), characteristic.Name().c_str(), characteristic.Error()));

                        Bluetooth::Profile::Service::Characteristic::Iterator descriptorIdx = characteristic.Descriptors();
                        while (descriptorIdx.Next() == true) {
                            const Bluetooth::Profile::Service::Characteristic::Descriptor& descriptor(descriptorIdx.Current());
                            const bool isHidReportCharacteristicConfig = ((isHidReportCharacteristic == true) && (descriptor.Type() == Bluetooth::Profile::Service::Characteristic::Descriptor::ClientCharacteristicConfiguration));
                            const bool isAudioCharacteristicConfig = ((isAudioCharacteristic == true) && (descriptor.Type() == Bluetooth::Profile::Service::Characteristic::Descriptor::ClientCharacteristicConfiguration));
                            TRACE(Flow, (_T("[0x%04X]       Descriptor:         %s"), descriptor.Handle(), descriptor.Name().c_str()));

                            if ((isHidReportCharacteristicConfig == true) || (isAudioCharacteristicConfig == true)) {
                                _handles.push_back(descriptor.Handle());
                            }
                        }
                    }
                }
            }

            void EnableEvents(const GATTSocket::Command& cmd)
            {
                if ((cmd.Error() != Core::ERROR_NONE) || (cmd.Result().Error() != 0) ) {
                    TRACE(Flow, (_T("Failed to enable reporting on handle 0x%04x [%d]"), _handles.front(), cmd.Result().Error()));
                }
                else {
                    TRACE(Flow, (_T("Enabled reporting on handle 0x%04x"), _handles.front()));
                }

                _handles.pop_front();

                if (_handles.size() > 0) {
                    uint16_t val = htobs(1);
                    _command.Write(_handles.front(), sizeof(val), reinterpret_cast<const uint8_t*>(&val));
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        EnableEvents(cmd);
                    });
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
            std::list<uint16_t> _handles;
            uint8_t _batteryLevel;
            uint16_t _batteryLevelHandle;
            Core::Sink<Sink> _sink;
            uint16_t _currentKey;
            const Core::ProxyType<Activity> _activity;
            BluetoothRemoteControl* _parent;
            string _name;

            Bluetooth::UUID AUDIO_SERVICE_UUID;
            Bluetooth::UUID AUDIO_COMMAND_UUID;
            Bluetooth::UUID AUDIO_DATA_UUID;
        };

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
        {
            ASSERT(_inputHandler != nullptr);
        }

        ~BluetoothRemoteControl() override
        {
        }

    public:
        BEGIN_INTERFACE_MAP(BluetoothRemoteControl)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
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

        virtual Exchange::IKeyProducer* Producer(const string& name) override
        {
            return (_gattRemote);
        }

    private:
        class Config : public Core::JSON::Container {
        public:
            class AudioConfig : public Core::JSON::Container {
            public:
                AudioConfig(const AudioConfig&);
                AudioConfig& operator=(const AudioConfig&);
                AudioConfig()
                    : Core::JSON::Container()
                    , ServiceUUID()
                    , CommandUUID()
                    , DataUUID()
                {
                    Add(_T("serviceuuid"), &ServiceUUID);
                    Add(_T("commanduuid"), &CommandUUID);
                    Add(_T("datauuid"), &DataUUID);
                }
                ~AudioConfig()
                {
                }

            public:
                Core::JSON::String ServiceUUID;
                Core::JSON::String CommandUUID;
                Core::JSON::String DataUUID;
            };

            Config(const Config&);
            Config& operator=(const Config&);
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
            AudioConfig Audio;
        };

        class Settings : public Core::JSON::Container {
        public:
            Settings(const Settings&);
            Settings& operator=(const Settings&);
            Settings()
                : Core::JSON::Container()
                , Name()
                , Address()
            {
                Init();
            }
            Settings(const string& name, const string& address)
                : Core::JSON::Container()
                , Name()
                , Address()
            {
                Name = name;
                Address = address;
                Init();
            }
            ~Settings()
            {
            }

        private:
            void Init()
            {
                Add(_T("name"), &Name);
                Add(_T("address"), &Address);
            }

        public:
            Core::JSON::String Name;
            Core::JSON::String Address;
        };

    private:
        // Bluetooth Remote Control implementation
        uint32_t Assign(const string& address);
        uint32_t Revoke();
        uint32_t Properties();
        uint8_t BatteryLevel() const;
        uint32_t LoadKeymap(GATTRemote& remote, const string& name);
        uint32_t LoadSettings(Settings& settings) const;
        uint32_t SaveSettings(const Settings& settings) const;
        void RemoteCreated(GATTRemote& remote);
        void RemoteDestroyed(GATTRemote& remote);

    private:
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& request);

    private:
        uint8_t _skipURL;
        Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        Config _config;
        GATTRemote* _gattRemote;
        PluginHost::VirtualInput* _inputHandler;
        string _settingsPath;

    }; // class BluetoothRemoteControl

} //namespace Plugin

}
