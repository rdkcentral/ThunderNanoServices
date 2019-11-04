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

            class HandlesConfig : public Core::JSON::Container {
            public:
                HandlesConfig()
                    : Core::JSON::Container()
                    , BatteryLevel(0)
                    , ModelNumber(0)
                    , SerialNumber(0)
                    , FirmwareRevision(0)
                    , SoftwareRevision(0)
                    , ManufacturerName(0)
                    , HidReport(0)
                    , AudioCommand(0)
                    , AudioData(0)
                {
                    Add(_T("batterylevel"), &BatteryLevel);
                    Add(_T("modelnumber"), &ModelNumber);
                    Add(_T("serialnumber"), &SerialNumber);
                    Add(_T("firmwarerevision"), &FirmwareRevision);
                    Add(_T("softwarerevision"), &SoftwareRevision);
                    Add(_T("manufacturername"), &ManufacturerName);
                    Add(_T("hidreport"), &HidReport);
                    Add(_T("audiocommand"), &AudioCommand);
                    Add(_T("audiodata"), &AudioData);
                }
                ~HandlesConfig()
                {
                }

                Core::JSON::DecUInt16 BatteryLevel;
                Core::JSON::DecUInt16 ModelNumber;
                Core::JSON::DecUInt16 SerialNumber;
                Core::JSON::DecUInt16 FirmwareRevision;
                Core::JSON::DecUInt16 SoftwareRevision;
                Core::JSON::DecUInt16 ManufacturerName;
                Core::JSON::DecUInt16 HidReport;
                Core::JSON::DecUInt16 AudioCommand;
                Core::JSON::DecUInt16 AudioData;
            };

            GATTRemote(BluetoothRemoteControl* parent, Exchange::IBluetooth::IDevice* device, uint16_t mtu = 255)
                : Bluetooth::GATTSocket(Designator(device->Type(), device->LocalId()), Designator(device->Type(), device->RemoteId()), mtu)
                , _adminLock()
                , _profile(true)
                , _command()
                , _device(device)
                , _handles()
                , _sink(this)
                , _currentKey(0)
                , _activity(Core::ProxyType<Activity>::Create())
                , _parent(parent)
                , _name()
                , _modelNumber()
                , _serialNumber()
                , _firmwareRevision()
                , _softwareRevision()
                , _manufacturerName()
                , _batteryLevel(~0)
                , _deviceInfoRead(false)
                , _modelNumberHandle(0)
                , _serialNumberHandle(0)
                , _firmwareRevisionHandle(0)
                , _softwareRevisionHandle(0)
                , _manufacturerNameHandle(0)
                , _hidReportHandle(0)
                , _batteryLevelHandle(0)
                , _audioCommandHandle(0)
                , _audioDataHandle(0)
                , _audioHandler(_parent)
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
                return (Core::ERROR_NONE);
            }

            string MetaData() const override
            {
                return {};
            }

            void Configure(const string& settings) override
            {
                HandlesConfig handles;
                if (handles.FromString(settings) == true) {
                    _modelNumberHandle = handles.ModelNumber.Value();
                    _serialNumberHandle = handles.SerialNumber.Value();
                    _firmwareRevisionHandle = handles.FirmwareRevision.Value();
                    _softwareRevisionHandle = handles.SoftwareRevision.Value();
                    _manufacturerNameHandle = handles.ManufacturerName.Value();
                    _batteryLevelHandle = handles.BatteryLevel.Value();
                    _hidReportHandle = handles.HidReport.Value();
                    _audioCommandHandle = handles.AudioCommand.Value();
                    _audioDataHandle = handles.AudioData.Value();
                    _audioHandler.Configure(_audioCommandHandle, _audioDataHandle);
                }
            }

            void Configuration(string& settings)
            {
                HandlesConfig handles;
                handles.ModelNumber = _modelNumberHandle;
                handles.SerialNumber = _serialNumberHandle;
                handles.FirmwareRevision = _firmwareRevisionHandle;
                handles.ManufacturerName = _manufacturerNameHandle;
                handles.BatteryLevel = _batteryLevelHandle;
                handles.HidReport = _hidReportHandle;
                handles.AudioCommand = _audioCommandHandle;
                handles.AudioData = _audioDataHandle;
                handles.ToString(settings);
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

            uint8_t BatteryLevel() const
            {
                return (_batteryLevel);
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

        private:
            bool Initialize() override
            {
                return (Security(BT_SECURITY_LOW));
            }

            void Notification(const uint16_t handle, const uint8_t dataFrame[], const uint16_t length) override
            {
                if ((_hidReportHandle != 0) && (handle == _hidReportHandle) && (length >= 2)) {
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
                } else if ((_batteryLevelHandle != 0) && (handle == _batteryLevelHandle) && (length >= 1)) {
                    BatteryLevel(dataFrame[0]);
                }
                else if (_audioHandler.Notification(handle, dataFrame, length) == false) {
                    string data;
                    Core::ToHexString(dataFrame, length, data);
                    printf(_T("Unhandled notification: [handle=0x%x], %d bytes: %s\n"), handle, length, data.c_str());
                }
            }

            uint16_t FindHandle(const Bluetooth::UUID& serviceUuid, const Bluetooth::UUID& charUuid)
            {
                const Bluetooth::Profile::Service* service = _profile[serviceUuid];
                if (service != nullptr) {
                    const Bluetooth::Profile::Service::Characteristic* charact = (*service)[charUuid];
                    if (charact != nullptr) {
                        return charact->Handle();
                    }
                }
                return (0);
            }

            void Operational() override
            {
                TRACE(Flow, (_T("The received MTU: %d"), MTU()));

                if ((_device->IsBonded() == false) || (_hidReportHandle == 0)) {
                    // No need to do service discovery if device is bonded, the notifications are already enabled
                    _profile.Discover(CommunicationTimeOut * 20, *this, [&](const uint32_t result) {
                        if (result == Core::ERROR_NONE) {
                            if (_profile[Bluetooth::UUID(HID_UUID)] == nullptr) {
                                TRACE(Flow, (_T("The given bluetooth device does not support a HID service!!")));
                            }
                            else {
                                LoadReportHandles();

                                using Service = Bluetooth::Profile::Service;
                                _modelNumberHandle = FindHandle(Service::DeviceInformation, Service::Characteristic::ModelNumberString);
                                _serialNumberHandle = FindHandle(Service::DeviceInformation, Service::Characteristic::SerialNumberString);
                                _firmwareRevisionHandle = FindHandle(Service::DeviceInformation, Service::Characteristic::FirmwareRevisionString);
                                _softwareRevisionHandle = FindHandle(Service::DeviceInformation, Service::Characteristic::SoftwareRevisionString);
                                _manufacturerNameHandle = FindHandle(Service::DeviceInformation, Service::Characteristic::ManufacturerNameString);
                                _batteryLevelHandle = FindHandle(Service::BatteryService, Service::Characteristic::BatteryLevel);
                                _hidReportHandle = FindHandle(Service::HumanInterfaceDevice, Service::Characteristic::Report);
                                _audioCommandHandle = FindHandle(AUDIO_SERVICE_UUID, AUDIO_COMMAND_UUID);
                                _audioDataHandle = FindHandle(AUDIO_SERVICE_UUID, AUDIO_DATA_UUID);
                                _audioHandler.Configure(_audioCommandHandle, _audioDataHandle);

                                _parent->RemoteBonded(*this);

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
                } else {
                    if (_deviceInfoRead == false) {
                        // No full service discovery now, just re-read the device information
                        _deviceInfoRead = true;
                        ReadModelNumber();
                    } else {
                        // Just quickly pick up the battery level
                        ReadBatteryLevel();
                    }
                }
            }

            void LoadReportHandles()
            {
                Bluetooth::Profile::Iterator serviceIdx = _profile.Services();
                while (serviceIdx.Next() == true) {
                    const Bluetooth::Profile::Service& service = serviceIdx.Current();
                    const bool isHidService = (service.Type() == Bluetooth::Profile::Service::HumanInterfaceDevice);
                    const bool isAudioService = (service.Type() == AUDIO_SERVICE_UUID);
                    const bool isBatteryService = (service.Type() == Bluetooth::Profile::Service::BatteryService);
                    TRACE(Flow, (_T("[0x%04X] Service: [0x%04X]         %s"), service.Handle(), service.Max(), service.Name().c_str()));

                    Bluetooth::Profile::Service::Iterator characteristicIdx = service.Characteristics();
                    while (characteristicIdx.Next() == true) {
                        const Bluetooth::Profile::Service::Characteristic& characteristic(characteristicIdx.Current());
                        const bool isHidReportCharacteristic = ((isHidService == true) && (characteristic.Type() == Bluetooth::Profile::Service::Characteristic::Report));
                        const bool isAudioCharacteristic = ((isAudioService == true) && ((characteristic.Type() == AUDIO_COMMAND_UUID) || (characteristic.Type() == AUDIO_DATA_UUID)));
                        const bool isBatteryLevelCharactersitic = ((isBatteryService == true) && (characteristic.Type() == Bluetooth::Profile::Service::Characteristic::BatteryLevel));
                        TRACE(Flow, (_T("[0x%04X]    Characteristic [0x%02X]: %s [%d]"), characteristic.Handle(), characteristic.Rights(), characteristic.Name().c_str(), characteristic.Error()));

                        Bluetooth::Profile::Service::Characteristic::Iterator descriptorIdx = characteristic.Descriptors();
                        while (descriptorIdx.Next() == true) {
                            const Bluetooth::Profile::Service::Characteristic::Descriptor& descriptor(descriptorIdx.Current());
                            const bool isHidReportCharacteristicConfig = ((isHidReportCharacteristic == true) && (descriptor.Type() == Bluetooth::Profile::Service::Characteristic::Descriptor::ClientCharacteristicConfiguration));
                            const bool isAudioCharacteristicConfig = ((isAudioCharacteristic == true) && (descriptor.Type() == Bluetooth::Profile::Service::Characteristic::Descriptor::ClientCharacteristicConfiguration));
                            const bool isBatteryLevelCharacteristicConfig = ((isBatteryLevelCharactersitic == true) && (descriptor.Type() == Bluetooth::Profile::Service::Characteristic::Descriptor::ClientCharacteristicConfiguration));
                            TRACE(Flow, (_T("[0x%04X]       Descriptor:         %s"), descriptor.Handle(), descriptor.Name().c_str()));

                            if ((isHidReportCharacteristicConfig == true) || (isAudioCharacteristicConfig == true) || (isBatteryLevelCharacteristicConfig == true)) {
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
                } else {
                    ReadModelNumber();
                }
            }

            void ReadModelNumber()
            {
                if (_modelNumberHandle != 0) {
                    _command.Read(_modelNumberHandle);
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                            _modelNumber = string(reinterpret_cast<const char*>(cmd.Result().Data()), cmd.Result().Length());
                            TRACE(Flow, (_T("ModelNumber: %s"), _modelNumber.c_str()));
                        } else {
                            TRACE(Flow, (_T("Failed to read model name")));
                        }

                        ReadSerialNumber();
                    });
                } else {
                    ReadSerialNumber();
                }
            }

            void ReadSerialNumber()
            {
                if (_serialNumberHandle != 0) {
                    _command.Read(_serialNumberHandle);
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
                    ReadFirmwareRevision();
                }
            }

            void ReadFirmwareRevision()
            {
                if (_firmwareRevisionHandle != 0) {
                    _command.Read(_firmwareRevisionHandle);
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                            _firmwareRevision = string(reinterpret_cast<const char*>(cmd.Result().Data()), cmd.Result().Length());
                            TRACE(Flow, (_T("FirmwareRevision: %s"), _firmwareRevision.c_str()));
                        } else {
                            TRACE(Flow, (_T("Failed to read firmware revision")));
                        }

                        ReadSoftwareRevision();
                    });
                } else {
                    ReadSoftwareRevision();
                }
            }

            void ReadSoftwareRevision()
            {
                if (_softwareRevisionHandle != 0) {
                    _command.Read(_softwareRevisionHandle);
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
                    ReadManufacturerName();
                }
            }

            void ReadManufacturerName()
            {
                if (_manufacturerNameHandle != 0) {
                    _command.Read(_manufacturerNameHandle);
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
                    ReadBatteryLevel();
                }
            }

            void ReadBatteryLevel()
            {
                if (_batteryLevelHandle != 0) {
                    _command.Read(_batteryLevelHandle);
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                            BatteryLevel(cmd.Result().Data()[0]);
                        } else {
                            TRACE(Flow, (_T("Failed to retrieve battery level")));
                        }
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

            void BatteryLevel(uint8_t level)
            {
                TRACE(Trace::Information, (_T("Remote control battery level is %i%%"), level));
                _batteryLevel = level;
                _parent->event_batterylevelchange(level);
            }

        private:
            class AudioHandler {
            public:
                AudioHandler(BluetoothRemoteControl* parent)
                    : _parent(parent)
                    , _commandHandle(0)
                    , _dataHandle(0)
                    , _seq(0)
                    , _stepIdx(0)
                    , _pred(0)
                    , _intSeq(0)
                    , _skip(true)
                    , _eot(true)
                {
                    ASSERT(parent != nullptr)
                }
                ~AudioHandler()
                {
                }

                void Configure(const uint16_t commandHandle, const uint16_t dataHandle)
                {
                    _commandHandle = commandHandle;
                    _dataHandle = dataHandle;
                }

                bool Notification(const uint16_t handle, const uint8_t dataFrame[], const uint16_t length)
                {
                    bool handled = false;

                    if ((handle == _dataHandle) && (length == 5)) {
                        const AudioHeader* hdr = reinterpret_cast<const AudioHeader*>(dataFrame);
                        handled = true;
                        _skip = false;
                        _seq = hdr->seq;
                        _stepIdx = hdr->step;
                        _pred = btohs(hdr->pred);
                        _compression = hdr->compression;
                        if ((_seq == 0) || (_eot == true)) {
                            _eot = false;
                            string profile = (_compression? "admpcm-hq" : "pcm");
                            // Audio transmission start event is delayed until the first header arrives
                            _parent->event_audiotransmission(profile);
                        } else if (_seq != (_intSeq + 1)) {
                            printf(_T("audio: sequence mismatch %i != %i, lost %i frames\n"), _seq, (_intSeq + 1), (_seq - (_intSeq + 1)));
                        }
                       _intSeq = _seq;
                       _skip = false;
                    } else if ((handle == _dataHandle) && (length == 1)) {
                        handled = true;
                        uint8_t seq = dataFrame[0];
                        if (seq != _intSeq) {
                            printf(_T("audio: missing footer!\n"));
                        }
                    } else if ((handle == _dataHandle) && (length != 5) && (length != 1)) {
                        handled = true;
                        if (_skip == false) {
                            uint8_t buf[length + 3];
                            buf[0] = _stepIdx;
                            buf[1] = (_pred & 0xF);
                            buf[2] = (_pred << 8);
                            // Prepending the buffer with the prediction values from the header notification
                            ::memcpy(buf + 3, dataFrame, length);
                            string frame;
                            Core::ToString(buf, length + 3, true, frame);
                            _parent->event_audioframe(_seq, frame);
                            _skip = false;
                        } else {
                            printf(_T("audio: missing header, skipping frame!\n"));
                        }
                    } else if ((handle == _commandHandle) && (length >= 1)) {
                        handled = true;
                        if (dataFrame[0] == 0) {
                            _eot = true;
                            _parent->event_audiotransmission();
                        }
                    }

                    return (handled);
                }

            private:
                struct AudioHeader {
                    uint8_t seq;
                    uint8_t step;
                    uint16_t pred;
                    uint8_t compression;
                };

            private:
                BluetoothRemoteControl* _parent;
                uint16_t _commandHandle;
                uint16_t _dataHandle;
                uint8_t _seq;
                uint8_t _stepIdx;
                uint8_t _pred;
                uint8_t _compression;
                uint8_t _intSeq;
                bool _skip;
                bool _eot;
            };

            Core::CriticalSection _adminLock;
            Bluetooth::Profile _profile;
            Command _command;
            Exchange::IBluetooth::IDevice* _device;
            std::list<uint16_t> _handles;
            Core::Sink<Sink> _sink;
            uint16_t _currentKey;
            const Core::ProxyType<Activity> _activity;
            BluetoothRemoteControl* _parent;
            string _name;
            string _modelNumber;
            string _serialNumber;
            string _firmwareRevision;
            string _softwareRevision;
            string _manufacturerName;
            uint8_t _batteryLevel;
            bool _deviceInfoRead;
            uint16_t _modelNumberHandle;
            uint16_t _serialNumberHandle;
            uint16_t _firmwareRevisionHandle;
            uint16_t _softwareRevisionHandle;
            uint16_t _manufacturerNameHandle;
            uint16_t _hidReportHandle;
            uint16_t _batteryLevelHandle;
            uint16_t _audioCommandHandle;
            uint16_t _audioDataHandle;

            AudioHandler _audioHandler;
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
                , Handles()
            {
                Init();
            }
            Settings(const string& name, const string& address, const string& handles)
                : Core::JSON::Container()
                , Name()
                , Address()
                , Handles()
            {
                Name = name;
                Address = address;
                Handles = handles;
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
                Add(_T("handles"), &Handles);
            }

        public:
            Core::JSON::String Name;
            Core::JSON::String Address;
            Core::JSON::String Handles;
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
        Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        Config _config;
        GATTRemote* _gattRemote;
        PluginHost::VirtualInput* _inputHandler;
        string _settingsPath;

    }; // class BluetoothRemoteControl

} //namespace Plugin

}
