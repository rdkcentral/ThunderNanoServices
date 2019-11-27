#pragma once

#include "Module.h"

#include "Administrator.h"
#include "WAVRecorder.h"

#include <interfaces/IBluetooth.h>
#include <interfaces/IKeyHandler.h>
#include <interfaces/IVoiceHandler.h>
#include <interfaces/json/JsonData_BluetoothRemoteControl.h>

namespace WPEFramework {

namespace Plugin {

    class BluetoothRemoteControl : public PluginHost::IPlugin
                                 , public PluginHost::IWeb
                                 , public PluginHost::JSONRPC
                                 , public Exchange::IKeyHandler {

        class GATTRemote : public Bluetooth::GATTSocket 
                         , public Exchange::IKeyProducer
                         , public Exchange::IVoiceProducer {
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

            class Decoupling : public Core::IDispatch {
            public:
                Decoupling(const Decoupling&) = delete;
                Decoupling& operator=(const Decoupling&) = delete;
                Decoupling(GATTRemote& parent)
                    : _parent(parent)
                    , _next(0)
                    , _length(0xFF)
                    , _buffer(reinterpret_cast<uint8_t*>(_length))
                {
                    ASSERT(_parent != nullptr);
                }
                ~Decoupling() override
                {
                }

            public:
                void Submit(const uint16_t handle, const uint8_t length, const uint8_t buffer[])
                {
                    ASSERT (length > 0);

                    _adminLock.Lock();

                    if ((_next + length + 3) > _length) {
                        // Oops we need to extend our buffer
                        _length = (((_next + length + 0xFF) >> 8) << 8);
                        uint8_t newBuffer = reinterpret_cast<uint8_t*>(::malloc(_length));
                        ::memcpy(newBuffer, _buffer, _next);
                        ::free(_buffer);
                        _buffer = newBuffer;
                    }

                    _buffer[_next++] = length;
                    _buffer[_next++] = (handle >> 8) & 0xFF;
                    _buffer[_next++] = (handle & 0xFF);
                    ::memcpy(&(_buffer[_next]), buffer, length);
                    _next += length;

                    _adminLock.Unlock();
                }
                void Dispatch() override
                {
                    uint16_t offset = 0;

                    _adminLock.Lock();

                    while (offset < _next) {

                        uint8_t length = _buffer[offset++];
                        uint16_t handle = (_buffer[offset++] << 8) | (_buffer[offset++]);
                        _parent.Message(handle, length, &(_buffer[offset]));

                        offset += length;

                        ASSERT (offset <= _next);
                    }

                    ASSERT (offset == _next);

                    _next = 0;

                    _adminLock.Unlock();
                }

            private:
                GATTRemote& _parent;
                uint16_t _next;
                uint16_t _length;
                uint8_t* _buffer;
            };

            class Profile : public Bluetooth::Profile {
            private:
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
                Profile() = delete;
                Profile(const Profile&) = delete;
                Profile& operator= (const Profile&) = delete;

                Profile(const string& configuration) 
                    : Bluetooth::Profile() {
                    Config config; 
                    config.FromString(configuration);
                    VoiceService = BluetoothUUID(config.ServiceUUID.Value());
                    VoiceCommandChar = BluetoothUUID(config.CommandUUID.Value());
                    VoiceDataChar = BluetoothUUID(config.DataUUID.Value());
                }
                ~Profile() {
                }

            public:
                uint16_t FindHandle(const Bluetooth::UUID& serviceUuid, const Bluetooth::UUID& charUuid) const
                {
                    const Bluetooth::Profile::Service* service = (*_profile)[serviceUuid];
                    if (service != nullptr) {
                        const Bluetooth::Profile::Service::Characteristic* characteristic = (*service)[charUuid];
                        if (characteristic != nullptr) {
                            handle = characteristic->Handle();
                        }
                    }
                    return (handle);
                }

            public:
                Bluetooth::UUID VoiceService;
                Bluetooth::UUID VoiceCommandChar;
                Bluetooth::UUID VoiceDataChar;
            };
            class AudioProfile : Exchange::IVoiceProducer::IProfile {
            public:
                AudioProfile(const AudioProfile&) = delete;
                AudioProfile& operator= (const AudioProfile&) = delete;
                AudioProfile(const Exchange::IVoiceProducer::IProfile::codec codec, const uint8_t channels, const uint32_t sampleRate, const uint8_t resolution) 
                    : _code(codec)
                    , _channels(channels)
                    , _sampleRate(sampleRate)
                    , _resolution(resolution) {
                }
                virtual ~AudioProfile() {
                }

            public:
                Exchange::IVoiceProducer::IProfile::codec Codec() const override {
                    return (_codec);
                }
                uint8_t Channels() const override {
                    return (_channels);
                }
                uint32_t SampleRate() const override {
                    return (_sampleRate);
                }
                uint8_t Resolution() const override {
                    return (_resolution);
                }

                BEGIN_INTERFACE_MAP(AudioProfile)
                    INTERFACE_ENTRY(Exchange::IVoiceProducer::IProfile)
                END_INTERFACE_MAP

            private:
                codec _codec;
                uint8_t _channels;
                uint32_t _sampleRate;
                uint8_t _resolution;
            };


            static Core::NodeId Designator(const uint8_t type, const string& address)
            {
                return (Bluetooth::Address(address.c_str()).NodeId(static_cast<Bluetooth::Address::type>(type), GATTSocket::LE_ATT_CID, 0));
            }

        public:
            class Data : public Core::JSON::Container {
            public:
                Data(const Data&) = delete;
                Data& operator=(const Data&) = delete;
                Data()
                    : Core::JSON::Container()
                    , Name()
                    , ModelNumber()
                    , SerialNumber()
                    , FirmwareRevision()
                    , SoftwareRevision()
                    , ManufacturerName()
                    , KeysDataHandle(0)
                    , BatteryLevelHandle(0)
                    , VoiceCommandHandle(0)
                    , VoiceDataHandle(0)
                {
                    Add(_T("name"), &Name);
                    Add(_T("model"), &ModelNumber);
                    Add(_T("serial"), &SerialNumber);
                    Add(_T("firmware"), &FirmwareRevision);
                    Add(_T("software"), &SoftwareRevision);
                    Add(_T("manufacturer"), &ManufacturerName);
                    Add(_T("key"), &KeysDataHandle);
                    Add(_T("battery"), &BatteryLevelHandle);
                    Add(_T("command"), &VoiceCommandHandle);
                    Add(_T("voice"), &VoiceDataHandle);
                }
                ~Data()
                {
                }

            public:
               Core::JSON::String Name;
               Core::JSON::String ModelNumber;
               Core::JSON::String SerialNumber;
               Core::JSON::String FirmwareRevision;
               Core::JSON::String SoftwareRevision;
               Core::JSON::String ManufacturerName;

               Core::JSON::DecUInt16 KeysDataHandle;
               Core::JSON::DecUInt16 BatteryLevelHandle;
               Core::JSON::DecUInt16 VoiceCommandHandle;
               Core::JSON::DecUInt16 VoiceDataHandle;
            };

        public:
            GATTRemote() = delete;
            GATTRemote(const GATTRemote&) = delete;
            GATTRemote& operator=(const GATTRemote&) = delete;
 
            GATTRemote(BluetoothRemoteControl* parent, Exchange::IBluetooth::IDevice* device, const string& configuration)
                : Bluetooth::GATTSocket(Designator(device->Type(), device->LocalId()), Designator(device->Type(), device->RemoteId()), 255)
                , _parent(parent)
                , _adminLock()
                , _profile(new Profile(configration))
                , _command()
                , _device(device)
                , _decoupling(this)
                , _sink(this)
                , _name()
                , _modelNumber()
                , _serialNumber()
                , _firmwareRevision()
                , _softwareRevision()
                , _manufacturerName()
                , _batteryLevel(0)
                , _keysDataHandle(0)
                , _batteryLevelHandle(0)
                , _voiceDataHandle(0)
                , _voiceCommandHandle(0)
                , _inputHandler(nullptr)
                , _voiceHandler(nullptr)
 
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
            uint32_t PrefferedProfile(const string& profile) override {
            }
            uint32_t Callback(Exchange::IVoiceHandler* callback) override {
            }

            // IKeyProducer methods
            const TCHAR* Name() const override
            {
                return _name.c_str();
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
            void Notification(const uint16_t handle, const uint8_t dataFrame[], const uint16_t length) override
            {
                // Decouple the notfications from the communciator thread, they will pop-up and need to be handled
                // by the Message method!
                _decoupling.Submit(handle, static_cast<uint8_t>(length), dataFrame);
                WorkerPool::Instance::Submit(Core::ProxyType<IDispatch>(_decoupling));
            }

            void Message(const uint16_t handle, const uint8_t length, const uint8_t buffer[])
            {
                _adminLock.Lock();

                if (handle == _voiceDataHandle) {
                }
                else if ( (handle == _keyDataHandle) && (length >= 2) ) {
                    uint16_t scancode = ((buffer[1] << 8) | buffer[0]);
                    bool pressed = (scancode != 0);

                    if (pressed == true) {
                        _currentKey = scancode;
                    }

                    if (_keyhandler != nullptr) {
                        _keyhandler->KeyEvent(pressed, _scancode, _name);
                    }

                    TRACE(Trace::Information, (_T("Received a keypress notification: code=%i, state=%s, keymap='%s'"), _currentKey, (pressed? _T("pressed") : _T("released")), _name.c_str()));
                }
                else if (handle == _voiceCommandHandle) {
                }
                else if ( (handle == _batteryLevelHandle) && (length >= 1) ) {
                    _batteryLevel = dataFrame[0];
                    event_batterylevelchange(_batteryLevel);
                    TRACE(Flow, (_T("Battery level is %i%%"), _batteryLevel));
                }
                _adminLock.Unlock();
            }

            bool Initialize() override
            {
                return (Security(BT_SECURITY_LOW));
            }

            void Operational() override
            {
                TRACE(Flow, (_T("The received MTU: %d"), MTU()));

                if (_profile == nullptr) {
                    // No need to do service discovery if device knows the Handles to use. If so, DeviceDiscovery has 
                    // already been done and the only thing we need to do is get the startingvalues :-)
                    ReadBatteryLevel();
                }
                else {
                    _profile->Discover(CommunicationTimeOut * 20, *this, [&](const uint32_t result) {
                        if (result == Core::ERROR_NONE) {

                            DumpProfile();

                            if (_profile[Bluetooth::UUID(HID_UUID)] == nullptr) {
                                TRACE(Flow, (_T("The given bluetooth device does not support a HID service!!")));
                            }
                            else {
                                ReadModelNumber();
                            }
                        }
                        else {
                            TRACE(Flow, (_T("The given bluetooth device could not be read for services!!")));
                        }
                    });
                }
            }

            void ReadModelNumber()
            {
                ASSERT (_profile != nullptr);

                uint16_t modelNumberHandle = _profile->FindHandle(Bluetooth::Profile::Service::DeviceInformation, Bluetooth::Profile::Service::Characteristic::ModelNumberString);
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
                ASSERT (_profile != nullptr);

                uint16_t serialNumberHandle = _profile->FindHandle(Bluetooth::Profile::Service::DeviceInformation, Bluetooth::Profile::Service::Characteristic::SerialNumberString);
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
                ASSERT (_profile != nullptr);

                uint16_t firmwareRevisionHandle = _profile->FindHandle(Bluetooth::Profile::Service::DeviceInformation, Bluetooth::Profile::Service::Characteristic::FirmwareRevisionString);
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
                ASSERT (_profile != nullptr);

                uint16_t softwareRevisionHandle = _profile->FindHandle(Bluetooth::Profile::Service::DeviceInformation, Bluetooth::Profile::Service::Characteristic::SoftwareRevisionString);
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
                ASSERT (_profile != nullptr);

                uint16_t manufacturerNameHandle = _profile->FindHandle(Bluetooth::Profile::Service::DeviceInformation, Bluetooth::Profile::Service::Characteristic::ManufacturerNameString);
                if (manufacturerNameHandle != 0) {
                    _command.Read(manufacturerNameHandle);
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                            _manufacturerName = string(reinterpret_cast<const char*>(cmd.Result().Data()), cmd.Result().Length());
                            TRACE(Flow, (_T("ManufacturerName: %s"), _manufacturerName.c_str()));
                        } else {
                            TRACE(Flow, (_T("Failed to read manufacturer name")));
                        }
                        EnableEvents();
                    });
                } else {
                    TRACE(Flow, (_T("ManufacturerNameString characteristic not available")));
                    EnableEvents();
                }
            }

            void EnableEvents() {

                uint16_t val = htobs(1);

                ASSERT (_profile != nullptr);

                // Set all interesting handles...
                if (_keysDataHandle == static_cast<uint16_t>(~0)) {
                    _keysDataHandle = _profile->FindHandle(Bluetooth::Profile::Service::BatteryService, Bluetooth::Profile::Service::Characteristic::BatteryLevel);
                    EnableEvents(_T("Key handling"), _keyDataHandle);
                }
                else if (_batteryLevelHandle == static_cast<uint16_t>(~0)) {
                    _batteryLevelHandle = _profile->FindHandle(Bluetooth::Profile::Service::BatteryService, Bluetooth::Profile::Service::Characteristic::BatteryLevel);
                    EnableEvents(_T("Battery handling"), _batteryLevelHandle);
                }
                else if (_voiceCommandHandle == static_cast<uint16_t>(~0)) {
                    _voiceCommandHandle = _profile->FindHandle(_profile->VoiceService, _profile->VoiceCommandChar);
                    EnableEvents(_T("Voice Command handling"), _voiceCommandHandle);
                }
                else if (_voiceDataHandle == static_cast<uint16_t>(~0)) {
                    _voiceDataHandle = _profile->FindHandle(_profile->VoiceService, _profile->VoiceDataChar);
                    EnableEvents(_T("Voice Data handling"), _voiceDataHandle);
                }
                else {
                    // We are done with the profile, all is read, all is set, we hope :-) Time
                    // to register what we did.
                    ReadBatteryLevel();
                }
            }

            void EnableEvents(const TCHAR* message, const uint16_t handle)
            {
                if (_handle == 0) {
                    TRACE(Flow, (_T("There is no notification for X [%d]"), _reportHandles.front(), cmd.Result().Error()));
                    EnableEvents();
                }
                else {
                    _command.Write(handle, sizeof(val), reinterpret_cast<const uint8_t*>(&val));
                    Execute(CommunicationTimeOut, _command, [&](handle) {
                        if ((_command.Error() != Core::ERROR_NONE) || (_command.Result().Error() != 0) ) {
                            TRACE(Flow, (_T("Failed to enable reporting on handle 0x%04X [%d]"), _reportHandles.front(), cmd.Result().Error()));
                        }
                        else {
                            TRACE(Flow, (_T("Enabled reporting on handle 0x%04X"), _reportHandles.front()));
                        }
                        EnableEvents();
                   });
                }
            }

            void ReadBatteryLevel()
            {
                if (_batteryLevelHandle != 0) {
                    _command.Read(batteryLevelHandle);
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                            _batteryLevel = cmd.Result().Data()[0];
                            event_batterylevelchange(_batteryLevel);
                            TRACE(Flow, (_T("Battery level is %i%%"), _batteryLevel));
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

        private:
            BluetoothRemoteControl* _parent;
            Core::CriticalSection _adminLock;
            Profile* _profile;
            GATTSocket::Command _command;
            Exchange::IBluetooth::IDevice* _device;
            Core::ProxyStore<Decoupling> _decoupling;
            Core::Sink<Sink> _sink;
 
            // The next section of information is coming from the config that
            // was stored for this remote or it is read from the profile if
            // this is the first reistration
            string _name;
            string _modelNumber;
            string _serialNumber;
            string _firmwareRevision;
            string _softwareRevision;
            string _manufacturerName;

            // If we are already registered, no need to read the profile as all
            // is coming from the RemoteControl persistent storage file.
            // The only thing that changes, and what should be read on each
            // new creation is the bttery level as that will change over time :-)
            uint8_t _batteryLevel;

            // During the intial cretaion/subscribe of this RemoteControl, the
            // handles that report keys, battery level and Voice (if applicable)
            // are registsered. Remember these to trigger the right hadler if
            // new data comes in.
            uint16_t _keysDataHandle;
            uint16_t _batteryLevelHandle;
            uint16_t _voiceDataHandle;
            uint16_t _voiceCommandHandle;

            // If we need to forward keys, this is the handler to where we should 
            // forward them.
            PluginHost::VirtualInput* _inputHandler;
            Exchange::IVoiceHandler* _voiceHandler;
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
        string _settingsPath;
    }; // class BluetoothRemoteControl

} //namespace Plugin

}
