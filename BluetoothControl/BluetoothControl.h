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
#include <interfaces/IBluetooth.h>
#include <interfaces/json/JBluetoothControl.h>

#include "Tracing.h"

namespace Thunder {

namespace Plugin {

using namespace Tracing;

namespace JBluetoothControl {
    using adaptertype = Exchange::JSONRPC::IBluetoothControl::adaptertype;
    using scantype = Exchange::JSONRPC::IBluetoothControl::scantype;
    using scanmode = Exchange::JSONRPC::IBluetoothControl::scanmode;
    using devicetype = Exchange::JSONRPC::IBluetoothControl::devicetype;
    using devicestate = Exchange::JSONRPC::IBluetoothControl::devicestate;
    using disconnectreason = Exchange::JSONRPC::IBluetoothControl::disconnectreason;
}

class BluetoothControl : public PluginHost::IPlugin
                       , public PluginHost::IPlugin::INotification
                       , public PluginHost::IWeb
                       , public PluginHost::JSONRPCSupportsEventStatus
                       , public Exchange::IBluetooth {
    public:
        class Config : public Core::JSON::Container {
        public:
            class LowEnergyConfig : public Core::JSON::Container {
            public:
                // Default settings for LE connections
                LowEnergyConfig()
                    : Core::JSON::Container()
                    , MinInterval(8)
                    , MaxInterval(8)
                    , Timeout(3000 /* ms */)
                    , Latency(0)
                {
                    Add(_T("mininterval"), &MinInterval);
                    Add(_T("maxinterval"), &MaxInterval);
                    Add(_T("timeout"), &Timeout);
                    Add(_T("latency"), &Latency);
                }
                ~LowEnergyConfig()
                {
                }

                LowEnergyConfig(const LowEnergyConfig&) = delete;
                LowEnergyConfig(LowEnergyConfig&&) = delete;
                LowEnergyConfig& operator=(const LowEnergyConfig&) = delete;
                LowEnergyConfig& operator=(LowEnergyConfig&&) = delete;

            public:
                Core::JSON::DecUInt16 MinInterval;
                Core::JSON::DecUInt16 MaxInterval;
                Core::JSON::DecUInt16 Timeout;
                Core::JSON::DecUInt16 Latency;
            };

            class UUIDConfig : public Core::JSON::Container {
            public:
                UUIDConfig& operator=(const UUIDConfig&) = delete;
                UUIDConfig()
                    : Core::JSON::Container()
                    , UUID()
                    , Service(0)
                    , Callsign()
                {
                    Init();
                }
                UUIDConfig(const UUIDConfig& other)
                    : Core::JSON::Container()
                    , UUID(other.UUID)
                    , Service(other.Service)
                    , Callsign(other.Callsign)
                {
                    Init();
                }
                ~UUIDConfig()  = default;

            private:
                void Init()
                {
                    Add(_T("uuid"), &UUID);
                    Add(_T("service"), &Service);
                    Add(_T("callsign"), &Callsign);
                }

            public:
                Core::JSON::String UUID;
                Core::JSON::HexUInt8 Service;
                Core::JSON::String Callsign;
            };

        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config()
                : Core::JSON::Container()
                , Interface(0)
                , Type(JBluetoothControl::adaptertype::DUAL)
                , Name(_T("Thunder BT Control"))
                , ShortName(Name)
                , Class(0)
                , ContinuousBackgroundScan()
                , AutoPasskeyConfirm(false)
                , PersistMAC(false)
            {
                Add(_T("interface"), &Interface);
                Add(_T("type"), &Type);
                Add(_T("name"), &Name);
                Add(_T("shortname"), &ShortName);
                Add(_T("class"), &Class);
                Add(_T("continuousbackgroundscan"), &ContinuousBackgroundScan);
                Add(_T("autopasskeyconfirm"), &AutoPasskeyConfirm);
                Add(_T("persistmac"), &PersistMAC);
                Add(_T("uuids"), &UUIDs);
                Add(_T("lowenergy"), &LowEnergy);
            }
            ~Config() = default;

        public:
            Core::JSON::DecUInt16 Interface;
            Core::JSON::EnumType<JBluetoothControl::adaptertype> Type;
            Core::JSON::String Name;
            Core::JSON::String ShortName;
            Core::JSON::HexUInt32 Class;
            Core::JSON::Boolean ContinuousBackgroundScan;
            Core::JSON::Boolean AutoPasskeyConfirm;
            Core::JSON::Boolean PersistMAC;
            Core::JSON::ArrayType<UUIDConfig> UUIDs;
            LowEnergyConfig LowEnergy;
        }; // class Config

        class Data : public Core::JSON::Container {
        public:
            Data(const Data&) = delete;
            Data& operator=(const Data&) = delete;
            Data()
                : Core::JSON::Container()
                , MAC()
            {
                Add(_T("mac"), &MAC);
            }
            ~Data() = default;

        public:
            Core::JSON::String MAC;
        }; // class Data

    private:
        class DecoupledJob : private Core::WorkerPool::JobType<DecoupledJob&> {
            friend class Core::ThreadPool::JobType<DecoupledJob&>;

        public:
            using Job = std::function<void()>;

            DecoupledJob(const DecoupledJob&) = delete;
            DecoupledJob& operator=(const DecoupledJob&) = delete;

            DecoupledJob()
                : Core::WorkerPool::JobType<DecoupledJob&>(*this)
                , _lock()
                , _job(nullptr)
            {
            }

            ~DecoupledJob() = default;

        public:
            void Submit(const Job& job)
            {
                _lock.Lock();
                _job = job;
                JobType::Submit();
                ASSERT(JobType::IsIdle() == false);
                _lock.Unlock();
            }
            void Schedule(const Job& job, const uint32_t defer)
            {
                _lock.Lock();
                _job = job;
                JobType::Reschedule(Core::Time::Now().Add(defer));
                ASSERT(JobType::IsIdle() == false);
                _lock.Unlock();
            }
            void Revoke()
            {
                _lock.Lock();
                JobType::Revoke();
                _job = nullptr;
                _lock.Unlock();
            }

        private:
            void Dispatch()
            {
                _lock.Lock();
                Job DoTheJob = _job;
                _job = nullptr;
                _lock.Unlock();

                if (DoTheJob != nullptr) {
                    DoTheJob();
                }
            }

        private:
            Core::CriticalSection _lock;
            Job _job;
        }; // class DecoupledJob

    private:
        static uint32_t UnpackDeviceClass(const uint8_t buffer[3])
        {
            ASSERT(buffer != nullptr);
            return ((buffer[2] << 16) | (buffer[1] << 8) | (buffer[0]));
        }
        static JBluetoothControl::devicetype DeviceType(const Exchange::IBluetooth::IDevice::type type)
        {
            return (static_cast<JBluetoothControl::devicetype>(type));
        }
        static JBluetoothControl::scanmode ScanMode(const bool limited)
        {
            return (limited? JBluetoothControl::scanmode::LIMITED : JBluetoothControl::scanmode::GENERAL);
        }

    private:
        class ControlSocket : public Bluetooth::HCISocket {
        private:
            class ManagementSocket : public Bluetooth::ManagementSocket {
            public:
                ManagementSocket(const ManagementSocket&) = delete;
                ManagementSocket& operator= (const ManagementSocket&) = delete;

                ManagementSocket(ControlSocket& parent)
                    : Bluetooth::ManagementSocket()
                    , _parent(parent)
                {
                }
                ~ManagementSocket() = default;

            private:
                void Update(const mgmt_hdr& header) override
                {
                    const uint8_t* data = &(reinterpret_cast<const uint8_t*>(&header)[sizeof(mgmt_hdr)]);
                    switch (btohs(header.opcode)) {
                    case MGMT_EV_NEW_SETTINGS:
                        Update(_parent.Settings().Actuals());
                        break;
                    case MGMT_EV_CMD_COMPLETE:
                        Update(*reinterpret_cast<const mgmt_ev_cmd_complete*>(data));
                        break;
                    case MGMT_EV_CONTROLLER_ERROR:
                        Update(*reinterpret_cast<const mgmt_ev_controller_error*>(data));
                        break;
                    case MGMT_EV_AUTH_FAILED:
                        Update(*reinterpret_cast<const mgmt_ev_auth_failed*>(data));
                        break;
                    case MGMT_EV_NEW_CONN_PARAM:
                        Update(*reinterpret_cast<const mgmt_ev_new_conn_param*>(data));
                        break;
                    case MGMT_EV_DEVICE_CONNECTED:
                        Update(*reinterpret_cast<const mgmt_ev_device_connected*>(data));
                        break;
                    case MGMT_EV_DEVICE_DISCONNECTED:
                        Update(*reinterpret_cast<const mgmt_ev_device_disconnected*>(data));
                        break;
                    case MGMT_EV_PIN_CODE_REQUEST:
                        Update(*reinterpret_cast<const mgmt_ev_pin_code_request*>(data));
                        break;
                    case MGMT_EV_USER_PASSKEY_REQUEST:
                        Update(*reinterpret_cast<const mgmt_ev_user_passkey_request*>(data));
                        break;
                    case MGMT_EV_USER_CONFIRM_REQUEST:
                        Update(*reinterpret_cast<const mgmt_ev_user_confirm_request*>(data));
                        break;
                    case MGMT_EV_NEW_LINK_KEY:
                        Update(*reinterpret_cast<const mgmt_ev_new_link_key*>(data));
                        break;
                    case MGMT_EV_NEW_IRK:
                        Update(*reinterpret_cast<const mgmt_ev_new_irk*>(data));
                        break;
                    case MGMT_EV_NEW_CSRK:
                        Update(*reinterpret_cast<const mgmt_ev_new_csrk*>(data));
                        break;
                    case MGMT_EV_NEW_LONG_TERM_KEY:
                        Update(*reinterpret_cast<const mgmt_ev_new_long_term_key*>(data));
                        break;
                    case MGMT_EV_ADVERTISING_REMOVED:
                        Update(*reinterpret_cast<const mgmt_ev_advertising_removed*>(data));
                        break;
                    default:
                        BT_TRACE(ManagementFlow, header, data);
                        break;
                    }
                }

            public:
                void Update(const ManagementSocket::Info::Properties& props)
                {
                    BT_TRACE(ManagementFlow, props);
                    _parent.SettingsChanged(props);
                }
                void Update(const mgmt_ev_cmd_complete& info)
                {
                    BT_TRACE(ManagementFlow, info);

                    if (info.opcode == MGMT_OP_PAIR_DEVICE) {
                        const mgmt_rp_pair_device* payload = reinterpret_cast<const mgmt_rp_pair_device*>(info.data);
                        _parent.PairingComplete(payload->addr.bdaddr, static_cast<Bluetooth::Address::type>(payload->addr.type), info.status);
                    }
                }
                void Update(const mgmt_ev_controller_error& info)
                {
                    BT_TRACE(ManagementFlow, info);
                }
                void Update(const mgmt_ev_auth_failed& info)
                {
                    BT_TRACE(ManagementFlow, info);
                }
                void Update(const mgmt_ev_new_conn_param& info)
                {
                    BT_TRACE(ManagementFlow, info);
                }
                void Update(const mgmt_ev_device_connected& info)
                {
                    BT_TRACE(ManagementFlow, info);
                }
                void Update(const mgmt_ev_device_disconnected& info)
                {
                    BT_TRACE(ManagementFlow, info);
                }
                void Update(const mgmt_ev_pin_code_request& info)
                {
                    BT_TRACE(ManagementFlow, info);
                    _parent.RequestPINCode(info.addr.bdaddr, static_cast<Bluetooth::Address::type>(info.addr.type));
                }
                void Update(const mgmt_ev_user_passkey_request& info)
                {
                    BT_TRACE(ManagementFlow, info);
                    _parent.RequestPasskey(info.addr.bdaddr, static_cast<Bluetooth::Address::type>(info.addr.type));
                }
                void Update(const mgmt_ev_user_confirm_request& info)
                {
                    BT_TRACE(ManagementFlow, info);
                    _parent.RequestPasskeyConfirm(info.addr.bdaddr, static_cast<Bluetooth::Address::type>(info.addr.type), btohl(info.value));
                }
                void Update(const mgmt_ev_advertising_added& info)
                {
                    BT_TRACE(ManagementFlow, info);
                }
                void Update(const mgmt_ev_advertising_removed& info)
                {
                    BT_TRACE(ManagementFlow, info);
                    _parent.AdvertisingRemoved(info.instance);
                }
                void Update(const mgmt_ev_new_link_key& info)
                {
                    BT_TRACE(ManagementFlow, info);

                    if (info.store_hint != 0) {
                        if (_parent.SecurityKey(info.key.addr.bdaddr, static_cast<Bluetooth::Address::type>(info.key.addr.type),
                                                Bluetooth::LinkKey(info.key.addr.bdaddr, info.key.val, info.key.pin_len, info.key.addr.type)) != Core::ERROR_NONE) {
                            TRACE(Trace::Error, (_T("Failed to store link key")));
                        }
                    } else {
                        TRACE(ManagementFlow, (_T("Management hints not to store the link key")));
                    }
                }
                void Update(const mgmt_ev_new_irk& info)
                {
                    BT_TRACE(ManagementFlow, info);

                    if (info.store_hint != 0) {
                        if (_parent.SecurityKey(info.key.addr.bdaddr, static_cast<Bluetooth::Address::type>(info.key.addr.type),
                                                Bluetooth::IdentityKey(info.key.addr.bdaddr, info.key.addr.type, info.key.val)) != Core::ERROR_NONE) {
                            TRACE(Trace::Error, (_T("Failed to store IRK")));
                        }
                    } else {
                        TRACE(ManagementFlow, (_T("Management hints not to store IRK")));
                    }
                }
                void Update(const mgmt_ev_new_csrk& info)
                {
                    BT_TRACE(ManagementFlow, info);
                    // For signed GATT writes, not required yet.
                }
                void Update(const mgmt_ev_new_long_term_key& info)
                {
                    BT_TRACE(ManagementFlow, info);

                    if (info.store_hint != 0) {
                        if (_parent.SecurityKey(info.key.addr.bdaddr, static_cast<Bluetooth::Address::type>(info.key.addr.type),
                                                Bluetooth::LongTermKey(info.key.addr.bdaddr, info.key.addr.type, info.key.type,
#ifdef BLUEZ_HAS_NO_INCLUSIVE_LANGUAGE
                                                                       info.key.central,
#else
                                                                       info.key.master,
#endif
                                                                       info.key.enc_size, btohs(info.key.ediv),
                                                                       btohll(info.key.rand), info.key.val)) != Core::ERROR_NONE) {
                            TRACE(Trace::Error, (_T("Failed to store LTK")));
                        }
                    } else {
                        TRACE(ManagementFlow, (_T("Management hints not to store LTK")));
                    }
                }

            private:
                ControlSocket& _parent;
            }; // class ManagementSocket

        public:
            static constexpr uint16_t LIMITED_SHIFT = 8;
            static constexpr uint16_t CONNECTABLE_SHIFT = (LIMITED_SHIFT + 4 /* four states */);

            enum state : uint16_t {
                IDLE = 0,
                SCANNING = 1,
                INQUIRING = 2,
                ADVERTISING = 4,
                DISCOVERABLE = 8
            };

            enum class adapter : uint8_t {
                BREDR = 0,
                LE = 1
            };

        private:
            static JBluetoothControl::scantype ScanType(const adapter type)
            {
                return (type == adapter::LE? JBluetoothControl::scantype::LOW_ENERGY : JBluetoothControl::scantype::CLASSIC);
            }

        public:
            ControlSocket(const ControlSocket&) = delete;
            ControlSocket& operator=(const ControlSocket&) = delete;
            ControlSocket()
                : Bluetooth::HCISocket()
                , _parent(nullptr)
                , _administrator(*this)
                , _state(IDLE)
                , _continuousBackgroundScan(false)
                , _scanJob()
                , _inquiryJob()
                , _reconnectJob()
            {
#if defined(USE_KERNEL_CONNECTION_CONTROL)
                // Scanning will fail if autoconnection is enabled with kernel connection control,
                // so at least allow picking up nearby devices during the background scan.
                // This will also work for non-kernel connection mode, but by default it's disabled,
                // since active scan will be then available.
                _continuousBackgroundScan = true;
#endif
            }
            ~ControlSocket() = default;

        public:
            BluetoothControl* Application() {
                return (_parent);
            }
            ManagementSocket& Administrator() {
                return (_administrator);
            }

        public:
            uint32_t Open(BluetoothControl& parent)
            {
                TRACE(ControlFlow, (_T("Opening HCI socket...")));

                ASSERT(HCISocket::IsOpen() == false);

                _parent = &parent;
                HCISocket::LocalNode(Core::NodeId(_administrator.DeviceId(), HCI_CHANNEL_RAW));

                return (HCISocket::Open(Core::infinite));
            }
            uint32_t Close()
            {
                TRACE(ControlFlow, (_T("Closing HCI socket...")));

                uint32_t result = Core::ERROR_ALREADY_RELEASED;

                if (HCISocket::IsOpen() == true) {

                    StopInquiry();
                    StopScanning();
                    StopDiscoverable();
                    StopAdvertising();

                    _scanJob.Revoke();
                    _inquiryJob.Revoke();
                    _reconnectJob.Revoke();

                    result = HCISocket::Close(Core::infinite);
                }

                _parent = nullptr;

                return (result);
            }

        public:
            Bluetooth::ManagementSocket::Info Settings() const
            {
                return (_administrator.Settings());
            }
            uint32_t SetDiscoverable(const bool limited, const uint16_t discoverableTime)
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if ((discoverableTime == 0) && (limited == true)) {
                    TRACE(Trace::Error, (_T("Invalid discoverable parameters for BR/EDR interface")));
                    result = Core::ERROR_BAD_REQUEST;
                }
                else {
                    if ((IsOpen() == true) && (SetState(DISCOVERABLE, limited) == Core::ERROR_NONE)) {

                        result = _administrator.Discoverable(true, limited, discoverableTime);

                        if (result == Core::ERROR_NONE) {
                            DiscoverableChanged(true, adapter::BREDR, limited);
                        }
                        else {
                            ClearState(DISCOVERABLE);
                        }
                    }
                }

                return (result);
            }
            uint32_t StopDiscoverable()
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                if ((IsOpen() == true) && ((GetState() & DISCOVERABLE) != 0)) {

                    result = _administrator.Discoverable(false);

                    // No kernel event for a forced stop, so notify now.
                    DiscoverableChanged(false, adapter::BREDR);
                    ClearState(DISCOVERABLE);
                }

                return (result);
            }
            bool IsDiscoverable(bool& limited) const
            {
                return (GetState(DISCOVERABLE, limited));
            }
            void AddUUID(const Bluetooth::UUID& uuid, const uint8_t serviceBits)
            {
                ASSERT(uuid.IsValid() == true);

                if (IsOpen() == true) {
                    TRACE(Trace::Information, (_T("Adding UUID %s to EIR"), uuid.ToString().c_str()));
                    _administrator.AddUUID(uuid, serviceBits);
                }
            }
            void RemoveUUID(const Bluetooth::UUID& uuid)
            {
                ASSERT(uuid.IsValid() == true);

                if (IsOpen() == true) {
                    TRACE(Trace::Information, (_T("Adding UUID %s from EIR"), uuid.ToString().c_str()));
                    _administrator.RemoveUUID(uuid);
                }
            }
            uint32_t Advertise(const bool limited, const bool connectable, const uint16_t discoverableTime)
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if ((IsOpen() == true) && (SetState(ADVERTISING, limited, connectable) == Core::ERROR_NONE)) {

                    VARIABLE_IS_NOT_USED uint8_t instance = 0;
                    // Only support one advertising instance.

                    result = _administrator.AddAdvertising(instance, limited, connectable, discoverableTime);

                    ASSERT(instance == 1);

                    if (result == Core::ERROR_NONE) {
                        DiscoverableChanged(true, adapter::LE, limited, connectable);
                    }
                    else {
                        ClearState(ADVERTISING);
                    }
                }

                return (result);
            }
            uint32_t StopAdvertising()
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                if ((IsOpen() == true) && ((GetState() & ADVERTISING) != 0)) {

                    result = _administrator.RemoveAdvertising();

                    // No kernel event for a forced stop, so notify now.
                    DiscoverableChanged(false, adapter::LE);
                    ClearState(ADVERTISING);
                }

                return (result);
            }
            bool IsAdvertising(bool& limited, bool& connectable) const
            {
                return (GetState(ADVERTISING, limited, connectable));
            }
            uint32_t Inquiry(const uint16_t scanTime, const bool limited)
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if ((IsOpen() == true) && (SetState(INQUIRING, limited) == Core::ERROR_NONE)) {

                    ScanningChanged(true, adapter::BREDR, limited);

                    _parent->RemoveDevices([](DeviceImpl* device) -> bool {
                        if ((device->IsPaired() == false) && (device->IsConnected() == false)
                                && (device->AddressType() == Bluetooth::Address::BREDR_ADDRESS))
                        {
                            device->ClearState();
                            return (true);
                        }
                        return (false);
                    });

                    result = Core::ERROR_NONE;

                    _inquiryJob.Submit([this, scanTime, limited]() {
                        BackgroundScan(false);
                        HCISocket::Inquiry(scanTime, limited); // blocking call

                        BackgroundScan(true);
                        ScanningChanged(false, adapter::BREDR);
                        ClearState(INQUIRING);
                    });
                }

                return (result);
            }
            uint32_t StopInquiry()
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                if ((IsOpen() == true) && ((GetState() & INQUIRING) != 0)) {
                    result = HCISocket::AbortInquiry(); // makes HCISocket::Inquiry() return
                }

                return (result);
            }
            bool IsInquiring(bool& limited) const
            {
                return (GetState(INQUIRING, limited));
            }
            uint32_t Scan(const uint16_t scanTime, const bool limited, const bool passive)
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if ((IsOpen() == true) && (SetState(SCANNING, limited) == Core::ERROR_NONE)) {

                    ScanningChanged(true, adapter::LE, limited);

                    _parent->RemoveDevices([](DeviceImpl* device) -> bool {
                        if ((device->IsPaired() == false) && (device->IsConnected() == false) && (device->AddressType() != Bluetooth::Address::BREDR_ADDRESS))
                        {
                            device->ClearState();
                            return (true);
                        }
                        return (false);
                    });

                    result = Core::ERROR_NONE;

                    _scanJob.Submit([this, scanTime, limited, passive]() {
                        BackgroundScan(false);
                        HCISocket::Scan(scanTime, limited, passive); // blocking call

                        BackgroundScan(true);
                        ScanningChanged(false, adapter::LE);
                        ClearState(SCANNING);
                    });
                }

                return (result);
            }
            uint32_t StopScanning()
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                if ((IsOpen() == true) && ((GetState() & SCANNING) != 0)) {
                    result = HCISocket::AbortScan(); // makes HCISocket::Scan() return
                }

                return (result);
            }
            bool IsScanning(bool& limited) const
            {
                return (GetState(SCANNING, limited));
            }

        public:
            void ContinuousBackgroundScan(const bool enable)
            {
                _continuousBackgroundScan = enable;
            }
            void BackgroundScan(const bool evaluate)
            {
#if !defined(USE_KERNEL_CONNECTION_CONTROL)
                if (IsOpen() == true) {
                    bool start = false;

                    if (evaluate == true) {
                        if (_continuousBackgroundScan == true) {
                            start = true;
                        } else {
                            _parent->IterateDevices([this, &start](DeviceImpl& device) {
                                if ((device.IsAutoConnectable() == true) && (device.IsConnected() == false)) {
                                    start = true;
                                    return (true);
                                }
                                return (false);
                            });
                        }
                    }

                    TRACE(ControlFlow, (_T("%s background scan..."),  (start? _T("Restarting") : _T("Stopping"))));

                    uint32_t result = HCISocket::Discovery(start);

                    if ((result != Core::ERROR_NONE) && (result != Core::ERROR_ILLEGAL_STATE)) {
                        TRACE(Trace::Error, (_T("BLE discovery %s failed"), _T("start"), _T("stop")));
                    }
                }
#endif // !defined(USE_KERNEL_CONNECTION_CONTROL)
            }

        public:
            uint32_t Pair(const Bluetooth::Address& remote, const Bluetooth::Address::type type, const Bluetooth::ManagementSocket::capabilities caps)
            {
                return (_administrator.Pair(remote, type, caps));
            }
            uint32_t PairAbort(const Bluetooth::Address& remote, const Bluetooth::Address::type type)
            {
                return (_administrator.PairAbort(remote, type));
            }
            uint32_t Unpair(const Bluetooth::Address& remote, const Bluetooth::Address::type type)
            {
                return (_administrator.Unpair(remote, type));
            }
            void PINCodeReply(const Bluetooth::Address& remote, const Bluetooth::Address::type type, const string& pincode)
            {
                _administrator.UserPINCodeReply(remote, type, pincode);
            }
            void PasskeyReply(const Bluetooth::Address& remote, const Bluetooth::Address::type type, const uint32_t passkey)
            {
                _administrator.UserPasskeyReply(remote, type, passkey);
            }
            void PasskeyConfirmReply(const Bluetooth::Address& remote, const Bluetooth::Address::type type, const bool confirm)
            {
                _administrator.UserPasskeyConfirmReply(remote, type, confirm);
            }

        private:
            void PairingComplete(const Bluetooth::Address& remote, const Bluetooth::Address::type type, const uint8_t status)
            {
                // Pairing is considered successful only if the appropriate keys are exchanged.
                if (status != 0) {
                    // ...but notify failed paring attempt
                    UpdateDevice<DeviceImpl>(remote, (type != Bluetooth::Address::BREDR_ADDRESS), [&](DeviceImpl* device) {
                        device->PairingComplete(status);
                    });
                }
            }
            template<typename KEYTYPE>
            uint32_t SecurityKey(const Bluetooth::Address& remote, const Bluetooth::Address::type type, const KEYTYPE& key)
            {
                uint32_t result = Core::ERROR_GENERAL;
                UpdateDevice<DeviceImpl>(remote, (type != Bluetooth::Address::BREDR_ADDRESS), [&](DeviceImpl* device) {
                    result = device->SecurityKey(key);
                });
                return (result);
            }
            void RequestPINCode(const Bluetooth::Address& remote, const Bluetooth::Address::type /* type */)
            {
                // Look up in classic devices only
                UpdateDevice<DeviceRegular>(remote, [&](DeviceRegular* device) {
                    device->RequestPINCode();
                });
            }
            void RequestPasskey(const Bluetooth::Address& remote, const Bluetooth::Address::type type)
            {
                UpdateDevice<DeviceImpl>(remote, (type != Bluetooth::Address::BREDR_ADDRESS), [&](DeviceImpl* device) {
                    device->RequestPasskey();
                });
            }
            void RequestPasskeyConfirm(const Bluetooth::Address& remote, const Bluetooth::Address::type type, const uint32_t passkey)
            {
                UpdateDevice<DeviceImpl>(remote, (type != Bluetooth::Address::BREDR_ADDRESS), [&](DeviceImpl* device) {
                    device->RequestPasskeyConfirm(passkey);
                });
            }

        private:
            uint32_t GetState() const
            {
                return (_state.GetState());
            }
            bool GetState(const state value, bool& limited) const
            {
                const uint16_t v = static_cast<uint16_t>(value);
                const uint32_t state = GetState();

                ASSERT(value != IDLE);

                limited = (state & (v << LIMITED_SHIFT));

                return (state & value);
            }
            bool GetState(const state value, bool& limited, bool& connectable) const
            {
                const uint16_t v = static_cast<uint16_t>(value);
                const uint32_t state = GetState();

                ASSERT(value != IDLE);

                limited = (state & (v << CONNECTABLE_SHIFT));
                connectable = (state & (v << LIMITED_SHIFT));

                return (state & value);
            }
            uint32_t SetState(const state value, const bool limited = false, const bool connectable = false)
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                ASSERT(value != IDLE);

                _state.Lock();

                if ((_state & value) == 0) {
                    result = Core::ERROR_NONE;
                    const auto v = static_cast<uint16_t>(value);
                    _state.SetState(static_cast<state>(_state.GetState() | v
                            | (limited? (v << LIMITED_SHIFT) : 0) | (connectable? (CONNECTABLE_SHIFT << 12) : 0)));
                }

                _state.Unlock();

                return (result);
            }
            uint32_t ClearState(const state value)
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                ASSERT(value != IDLE);

                _state.Lock();

                if ((_state & value) == value) {
                    result = Core::ERROR_NONE;
                    _state.SetState(static_cast<state>(_state.GetState() & (~value)
                                & (~(value << LIMITED_SHIFT)) & (~(CONNECTABLE_SHIFT << 12))));
                }

                _state.Unlock();

                return (result);
            }

        private:
            template<typename DEVICE, typename LOCATOR>
            void UpdateDevice(const LOCATOR& locator, DEVICE* device, const std::function<void(DEVICE* device)>& action)
            {
                struct ToString {
                    static string Locator(const uint16_t& connection)
                    {
                        return (Core::ToString(connection));
                    }
                    static string Locator(const Bluetooth::Address& address)
                    {
                        return (address.ToString());
                    }
                };

                if (device != nullptr) {
                    action(device);
                } else {
                    TRACE(ControlFlow, (_T("Can't update device: unknown locator '%s'"), ToString::Locator(locator).c_str()));
                }
            }
            template<typename DEVICE, typename LOCATOR>
            void UpdateDevice(const LOCATOR& locator, const std::function<void(DEVICE* device)>& action)
            {
                if ((Application() != nullptr) ) {
                    UpdateDevice(locator, Application()->Find<DEVICE>(locator), action);
                }
            }
            template<typename DEVICE>
            void UpdateDevice(const Bluetooth::Address& locator, const bool lowEnergy,
                              const std::function<void(DEVICE* device)>& action)
            {
                if ((Application() != nullptr) ) {
                    UpdateDevice(locator, Application()->Find(locator, lowEnergy), action);
                }
            }

        private:
            void SettingsChanged(const ManagementSocket::Info::Properties& settings)
            {
                /* This event will fire if a BR/EDR discovery scan is complete by timeout.
                   But will not fire if the discovery is stopped by a command (!). */
                if ((settings.IsDiscoverable() == false) && (GetState() & DISCOVERABLE)) {
                    ClearState(DISCOVERABLE);
                    DiscoverableChanged(false, adapter::BREDR);
                }
            }
            void AdvertisingRemoved(const uint8_t instance VARIABLE_IS_NOT_USED)
            {
                /* This event will fire if a BLE discovery scan is complete by timeout.
                   But will not fire is the discovery is stopped by a command (!). */

                /* We limit ourselves to one advertising instance at a time. */
                ASSERT(instance == 1);

                ClearState(ADVERTISING);
                DiscoverableChanged(false, adapter::LE);
            }

        public:
            void DiscoverableChanged(const bool started, const adapter type, const bool limited = false, const bool connectable = true)
            {
                TRACE(ControlFlow, (_T("%s discoverable state %s (%s)"), ((type == adapter::LE)? "BLE" : "BR/EDR"),
                                        (started? "started" : "ended"), Core::Time::Now().ToRFC1123().c_str()));

                Application()->Update();

                if (started == true) {
                    _parent->JSONRPC().DiscoverableStarted(ScanType(type), ScanMode(limited), connectable);
                }
                else {
                    _parent->JSONRPC().DiscoverableComplete(ScanType(type));
                }
            }
            void ScanningChanged(const bool started, const adapter type, const bool limited = false)
            {
                TRACE(ControlFlow, (_T("%s scan %s (%s)"), ((type == adapter::LE)? "BLE" : "BR/EDR"),
                                        (started? "started" : "ended"), Core::Time::Now().ToRFC1123().c_str()));

                Application()->Update();

                if (started == true) {
                    _parent->JSONRPC().ScanStarted(ScanType(type), ScanMode(limited));
                }
                else {
                    _parent->JSONRPC().ScanComplete(ScanType(type));
                }
            }

        public:
            void Update(const inquiry_info& info) override
            {
                BT_TRACE(ControlFlow, info);

                if (Application() != nullptr) {
                    const Bluetooth::Address address(info.bdaddr);
                    DeviceImpl* device = Application()->Find(address, false);

                    if (device == nullptr) {
                        device = Application()->Discovered(false, info.bdaddr);
                    }

                    if (device != nullptr) {
                        device->Class(UnpackDeviceClass(info.dev_class));
                    }
                }
            }
            void Update(const inquiry_info_with_rssi& info) override
            {
                BT_TRACE(ControlFlow, info);

                if (Application() != nullptr) {
                    const Bluetooth::Address address(info.bdaddr);
                    DeviceImpl* device = Application()->Find(address, false);

                    if (device == nullptr) {
                        device = Application()->Discovered(false, info.bdaddr);
                    }

                    if (device != nullptr) {
                        device->Class(UnpackDeviceClass(info.dev_class));
                    }
                }
            }
            void Update(const extended_inquiry_info& info) override
            {
                BT_TRACE(ControlFlow, info);

                if (Application() != nullptr) {
                    const Bluetooth::Address address(info.bdaddr);
                    DeviceImpl* device = Application()->Find(address, false);

                    if (device == nullptr) {
                        device = Application()->Discovered(false, info.bdaddr);
                    }

                    if (device != nullptr) {
                        Bluetooth::EIR eir(info.data, sizeof(info.data));
                        device->Update(eir);
                        device->Class(UnpackDeviceClass(info.dev_class));
                    }
                }
            }
            void Update(const le_advertising_info& info) override
            {
                BT_TRACE(ControlFlow, info);

                if ((Application() != nullptr) && (info.bdaddr_type == 0 /* public */)) {
                    const uint8_t CONNECTABLE_UNDIRECTED = 0;
                    const uint8_t CONNECTABLE_DIRECTED = 1;
                    const uint8_t SCAN_RESPONSE = 4;

                    if ((info.evt_type == SCAN_RESPONSE) || (info.evt_type == CONNECTABLE_UNDIRECTED) || (info.evt_type == CONNECTABLE_DIRECTED)) {
                        const Bluetooth::Address address(info.bdaddr);
                        DeviceImpl* device = Application()->Find(address, true);

                        if ((device == nullptr) && ((info.evt_type == SCAN_RESPONSE) || (_continuousBackgroundScan == true))) {
                            device = Application()->Discovered(true, info.bdaddr);
                        }

                        if (device != nullptr) {
                            // Let's see if the adv packet contains extra information we could update our data with.
                            if (info.length > 0) {
                                Bluetooth::EIR eir(info.data, info.length);
                                device->Update(eir);
                            }

#if defined(USE_KERNEL_CONNECTION_CONTROL)
                            // Kernel autoconnects on directed advertisment
                            if (info.evt_type == CONNECTABLE_UNDIRECTED)
#else
                            if ((info.evt_type == CONNECTABLE_DIRECTED) || (info.evt_type == CONNECTABLE_UNDIRECTED))
#endif
                            {
                                if ((device->IsAutoConnectable() == true) && (device->IsPaired() == true) && (device->IsConnected() == false)) {
                                    // The device broadcasts connection requests, let's connect now.
                                    _reconnectJob.Submit([device, this]() {
                                        TRACE(ControlFlow, (_T("Reconnecting to %s..."), device->RemoteId().c_str()));
                                        if (device->Connect() != Core::ERROR_NONE) {
                                            TRACE(Trace::Error, (_T("Failed reconnecting to %s"), device->RemoteId().c_str()));
                                        };
                                    });
                                } else {
                                    TRACE(ControlFlow, (_T("Won't reconnect to %s - not enabled for auto-connection"), device->RemoteId().c_str()));
                                }
                            }
                        }
                    }
                }
            }

        public:
            void Update(const hci_event_hdr& header) override
            {
                const uint8_t* data = &(reinterpret_cast<const uint8_t*>(&header)[sizeof(hci_event_hdr)]);

                switch (header.evt) {
                    case EVT_VENDOR:
                        Update(header.plen, data);
                        break;
                    case EVT_CMD_COMPLETE:
                        Update(*reinterpret_cast<const evt_cmd_complete*>(data));
                        break;
                    case EVT_CMD_STATUS:
                        Update(*reinterpret_cast<const evt_cmd_status*>(data));
                        break;
                    case EVT_CONN_REQUEST:
                        Update(*reinterpret_cast<const evt_conn_request*>(data));
                        break;
                    case EVT_CONN_COMPLETE:
                        Update(*reinterpret_cast<const evt_conn_complete*>(data));
                        break;
                    case EVT_DISCONN_COMPLETE:
                        Update(*reinterpret_cast<const evt_disconn_complete*>(data));
                        break;
                    case EVT_REMOTE_NAME_REQ_COMPLETE:
                        Update(*reinterpret_cast<const evt_remote_name_req_complete*>(data));
                        break;
                    case EVT_IO_CAPABILITY_REQUEST:
                        Update(*reinterpret_cast<const evt_io_capability_request*>(data));
                        break;
                    case EVT_IO_CAPABILITY_RESPONSE:
                        Update(*reinterpret_cast<const evt_io_capability_response*>(data));
                        break;
                    case EVT_USER_CONFIRM_REQUEST:
                        Update(*reinterpret_cast<const evt_user_confirm_request*>(data));
                        break;
                    case EVT_USER_PASSKEY_REQUEST:
                        Update(*reinterpret_cast<const evt_user_passkey_request*>(data));
                        break;
                    case EVT_USER_PASSKEY_NOTIFY:
                        Update(*reinterpret_cast<const evt_user_passkey_notify*>(data));
                        break;
                    case EVT_READ_REMOTE_FEATURES_COMPLETE:
                        Update(*reinterpret_cast<const evt_read_remote_features_complete*>(data));
                        break;
                    case EVT_LINK_KEY_REQ:
                        Update(*reinterpret_cast<const evt_link_key_req*>(data));
                        break;
                    case EVT_LINK_KEY_NOTIFY:
                        Update(*reinterpret_cast<const evt_link_key_notify*>(data));
                        break;
                    case EVT_AUTH_COMPLETE:
                        Update(*reinterpret_cast<const evt_auth_complete*>(data));
                        break;
                    case EVT_SIMPLE_PAIRING_COMPLETE:
                        Update(*reinterpret_cast<const evt_simple_pairing_complete*>(data));
                        break;
                    case EVT_LE_META_EVENT: {
                        const evt_le_meta_event* input = reinterpret_cast<const evt_le_meta_event*>(data);
                        switch (input->subevent) {
                        case EVT_LE_CONN_COMPLETE:
                            Update(*reinterpret_cast<const evt_le_connection_complete*>(data + sizeof(evt_le_meta_event)));
                            break;
                        case EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE:
                            Update(*reinterpret_cast<const evt_le_read_remote_used_features_complete*>(data + sizeof(evt_le_meta_event)));
                            break;
                        case EVT_LE_CONN_UPDATE_COMPLETE:
                            Update(*reinterpret_cast<const evt_le_connection_update_complete*>(data + sizeof(evt_le_meta_event)));
                            break;
                        default:
                            BT_TRACE(ControlFlow, header, *input);
                            break;
                        }
                        break;
                    }
                    default:
                        BT_TRACE(ControlFlow, header, data);
                        break;
                }
            }

        public:
            void Update(const uint8_t length, const uint8_t vendorData[])
            {
                BT_TRACE(ControlFlow, length, vendorData);
            }
            void Update(const evt_cmd_status& info)
            {
                BT_TRACE(ControlFlow, info);
            }
            void Update(const evt_cmd_complete& info)
            {
                BT_TRACE(ControlFlow, info);
            }
            void Update(const evt_conn_request& info)
            {
                BT_TRACE(ControlFlow, info);

                if (Application() != nullptr) {
                    const Bluetooth::Address address(info.bdaddr);
                    DeviceImpl* device = Application()->Find(address, false);

                    if (device == nullptr) {
                        device = Application()->Discovered(false, info.bdaddr);
                    }

                    if (device != nullptr) {
                        device->Class(UnpackDeviceClass(info.dev_class));
                    }
                }
            }
            void Update(const evt_conn_complete& info)
            {
                BT_TRACE(ControlFlow, info);
                if (info.status == 0) {
                    UpdateDevice<DeviceRegular>(info.bdaddr, [&](DeviceRegular* device) {
                        device->Connection(btohs(info.handle));
                    });
                }
            }
            void Update(const evt_disconn_complete& info)
            {
                BT_TRACE(ControlFlow, info);
                if (info.status == 0) {
                   UpdateDevice<DeviceImpl>(btohs(info.handle), [&](DeviceImpl* device) {
                        device->Disconnection(info.reason);
                    });
                }
            }
            void Update(const evt_le_connection_complete& info)
            {
                BT_TRACE(ControlFlow, info);
                if (info.status == 0) {
                    UpdateDevice<DeviceLowEnergy>(info.peer_bdaddr, [&](DeviceLowEnergy* device) {
                        device->Connection(btohs(info.handle));
                    });
                }
            }
            void Update(const evt_le_connection_update_complete& info)
            {
                BT_TRACE(ControlFlow, info);
            }
            void Update(const evt_remote_name_req_complete& info)
            {
                uint8_t length = 0;

                if (info.status == 0) {
                    while ((length < HCI_MAX_NAME_LENGTH) && ::isprint(info.name[length])) {
                        length++;
                    }
                }

                string friendlyName(reinterpret_cast<const char*>(info.name), length);

                BT_TRACE(ControlFlow, info, friendlyName);

                if (friendlyName.empty() == false) {
                    UpdateDevice<DeviceImpl>(info.bdaddr, [&](DeviceImpl* device) {
                        device->Name(friendlyName);
                    });
                }
            }
            void Update(const evt_io_capability_request& info)
            {
                BT_TRACE(ControlFlow, info);
            }
            void Update(const evt_io_capability_response& info)
            {
                BT_TRACE(ControlFlow, info);

                UpdateDevice<DeviceImpl>(info.bdaddr, [&](DeviceImpl* device) {
                    device->Capabilities(info.capability, info.authentication, info.oob_data);
                });
            }
            void Update(const evt_user_confirm_request& info)
            {
                BT_TRACE(ControlFlow, info);
            }
            void Update(const evt_user_passkey_request& info)
            {
                BT_TRACE(ControlFlow, info);
            }
            void Update(const evt_user_passkey_notify& info)
            {
                BT_TRACE(ControlFlow, info);
            }
            void Update(const evt_read_remote_features_complete& info)
            {
                BT_TRACE(ControlFlow, info);
                if (info.status == 0) {
                    UpdateDevice<DeviceImpl>(btohs(info.handle), [&](DeviceImpl* device) {
                        device->Features(sizeof(info.features), info.features);
                    });
                }
            }
            void Update(const evt_le_read_remote_used_features_complete& info)
            {
                BT_TRACE(ControlFlow, info);
                if (info.status == 0) {
                    UpdateDevice<DeviceImpl>(btohs(info.handle), [&](DeviceImpl* device) {
                        device->Features(sizeof(info.features), info.features);
                    });
                }
            }
            void Update(const evt_link_key_req& info)
            {
                BT_TRACE(ControlFlow, info);
            }
            void Update(const evt_link_key_notify& info)
            {
                BT_TRACE(ControlFlow, info);
            }
            void Update(const evt_auth_complete& info)
            {
                BT_TRACE(ControlFlow, info);
            }
            void Update(const evt_simple_pairing_complete& info)
            {
                BT_TRACE(ControlFlow, info);
            }

        private:
            BluetoothControl* _parent;
            ManagementSocket _administrator;
            Core::StateTrigger<state> _state;
            bool _continuousBackgroundScan;
            DecoupledJob _scanJob;
            DecoupledJob _inquiryJob;
            DecoupledJob _reconnectJob;
        }; // class ControlSocket

    public:
        class DeviceImpl : public IBluetooth::IDevice {
        private:
            static constexpr uint16_t ACTION_MASK = 0x00FF;

        public:
            static constexpr uint32_t MAX_ACTION_TIMEOUT = 2000;

            enum state : uint16_t {
                CONNECTING    = 0x0001,
                DISCONNECTING = 0x0002,
                PAIRING       = 0x0004,
                UNPAIRING     = 0x0008,

                LOWENERGY     = 0x1000,
                PUBLIC        = 0x2000
            };

            class Config : public Core::JSON::Container {
            public:
                Config(const Config&) = delete;
                Config& operator=(const Config&) = delete;
                Config()
                    : Core::JSON::Container()
                    , Type()
                    , Name()
                    , Class(0)
                    , LinkKeys()
                    , LongTermKeys()
                    , IdentityKey()
                {
                    Add(_T("type"), &Type);
                    Add(_T("name"), &Name);
                    Add(_T("class"), &Class);
                    Add(_T("linkkeys"), &LinkKeys);
                    Add(_T("longtermkeys"), &LongTermKeys);
                    Add(_T("identitykey"), &IdentityKey);
                }
                ~Config() = default;

            public:
                template<typename KEYLISTTYPE>
                void Get(const Core::JSON::ArrayType<Core::JSON::String>& keys, const Bluetooth::Address& remote,
                         const Bluetooth::Address::type type, KEYLISTTYPE& list) const
                {
                    auto index = keys.Elements();
                    while (index.Next() == true) {
                        typename KEYLISTTYPE::type key(remote, type, index.Current().Value());
                        if (key.IsValid() == true) {
                            list.Add(key);
                        }
                    }
                }
                template<typename KEYLISTTYPE>
                void Set(Core::JSON::ArrayType<Core::JSON::String>& keys, const KEYLISTTYPE& list) const
                {
                    auto index = list.Elements();
                    while(index.Next() == true) {
                        Core::JSON::String key;
                        key = index.Current().ToString();
                        keys.Add(key);
                    }
                }

            public:
                Core::JSON::EnumType<Bluetooth::Address::type> Type;
                Core::JSON::String Name;
                Core::JSON::DecUInt32 Class;
                Core::JSON::ArrayType<Core::JSON::String> LinkKeys;
                Core::JSON::ArrayType<Core::JSON::String> LongTermKeys;
                Core::JSON::String IdentityKey;
            }; // class Config

            class Data : public Core::JSON::Container {
            public:
                Data& operator=(const Data&) = delete;
                Data()
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
                    Add(_T("bonded"), &Paired);
                    Add(_T("paired"), &Paired);
                    Add(_T("reason"), &Reason);
                }
                Data(const Data& copy)
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
                    Add(_T("bonded"), &Paired);
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
                ~Data() = default;

            public:
                Data& Set(const DeviceImpl* source)
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

            public:
                Core::JSON::String LocalId;
                Core::JSON::String RemoteId;
                Core::JSON::String Name;
                Core::JSON::Boolean LowEnergy;
                Core::JSON::Boolean Connected;
                Core::JSON::Boolean Paired;
                Core::JSON::DecUInt16 Reason;
            }; // class Data

        public:
            class IteratorImpl : public IBluetooth::IDevice::IIterator {
            public:
                IteratorImpl() = delete;
                IteratorImpl(const IteratorImpl&) = delete;
                IteratorImpl& operator=(const IteratorImpl&) = delete;

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
                ~IteratorImpl() override
                {
                    while (_list.size() != 0) {
                        _list.front()->Release();
                        _list.pop_front();
                    }
                }

            public:
                void Reset() override
                {
                    _index = 0;
                }
                bool IsValid() const override
                {
                    return ((_index != 0) && (_index <= _list.size()));
                }
                bool Next() override
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
                IBluetooth::IDevice* Current() override
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
            }; // class IteratorImpl

        public:
            DeviceImpl() = delete;
            DeviceImpl(const DeviceImpl&) = delete;
            DeviceImpl& operator=(const DeviceImpl&) = delete;

            DeviceImpl(BluetoothControl* parent, const Bluetooth::Address::type type, const uint16_t deviceId, const Bluetooth::Address& remote)
                : _parent(parent)
                , _state(static_cast<state>((type == Bluetooth::Address::BREDR_ADDRESS      ? 0 :
                                            (type == Bluetooth::Address::LE_RANDOM_ADDRESS  ? LOWENERGY :
                                                                                              LOWENERGY|PUBLIC))))
                , _name()
                , _type(type)
                , _deviceId(deviceId)
                , _class(0)
                , _handle(~0)
                , _remote(remote)
                , _uuids()
                , _capabilities(~0)
                , _authentication(~0)
                , _oobData(~0)
#if defined(USE_KERNEL_CONNECTION_CONTROL)
                , _autoConnectionSubmitted(false)
#endif
                , _autoConnect(false)
                , _callback(nullptr)
                , _securityCallback(nullptr)
                , _deviceUpdateJob()
                , _autoConnectJob()
                , _userRequestJob()
                , _userReplyJob()
            {
                ASSERT(parent != nullptr);
                ::memset(_features, 0xFF, sizeof(_features));
            }
            ~DeviceImpl() override
            {
                TRACE(Trace::Information, (_T("Device [%s] has been forgotten"), RemoteId().c_str()));
            }

        public:
            // SecurityKeys getters
            virtual void SecurityKey(Bluetooth::LinkKeys& key) const = 0;
            virtual void SecurityKey(Bluetooth::LongTermKeys& key) const = 0;
            virtual const Bluetooth::IdentityKey& IdentityKey() const = 0;

        public:
            Bluetooth::Address Address() const
            {
                return (_remote);
            }
            Bluetooth::Address::type AddressType() const
            {
                return (_type);
            }
            inline uint16_t DeviceId() const
            {
                return (_deviceId);
            }
            inline bool LowEnergy() const
            {
                return ((_state & LOWENERGY) != 0);
            }
            const Bluetooth::Address& Locator() const
            {
                return(_remote);
            }
            const std::list<Bluetooth::UUID>& UUIDs() const
            {
                return (_uuids);
            }
            uint16_t ConnectionId() const
            {
                return (_handle);
            }
            bool IsAutoConnectable() const
            {
                return (_autoConnect);
            }

        public:
            // IBluetooth::IDevice overrides
            // ---------------------------------------------------------------------------------------------
            bool IsValid() const override
            {
                return (_state != 0);
            }
            IBluetooth::IDevice::type Type() const override
            {
                return ( (_state & (LOWENERGY|PUBLIC)) == 0         ? IBluetooth::IDevice::ADDRESS_BREDR :
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
                if (_name.empty() == true) {
                    return _T("[Unknown]");
                } else {
                    return (_name);
                }
            }
            uint32_t Class() const override
            {
                return (_class);
            }
            bool IsServiceSupported(const string& serviceUUID) const override
            {
                Bluetooth::UUID uuid(serviceUUID);

                _state.Lock();

                const bool result =  (uuid.IsValid() == true? (std::find(_uuids.cbegin(), _uuids.cend(), uuid) != _uuids.cend()) : false);

                _state.Unlock();

                return (result);
            }
            bool IsConnected() const override
            {
                return (_handle != static_cast<uint16_t>(~0));
            }
            uint32_t Disconnect() override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                AutoConnect(false);

                if (SetState(DISCONNECTING) == Core::ERROR_NONE) {

                    Bluetooth::HCISocket::Command::Disconnect disconnect;
                    disconnect->handle = htobs(ConnectionId());
                    disconnect->reason = HCI_OE_USER_ENDED_CONNECTION;

                    result = _parent->Connector().Exchange(MAX_ACTION_TIMEOUT, disconnect, disconnect);
                    if (result == Core::ERROR_NONE) {
                        if (disconnect.Result() != 0) {
                            switch (disconnect.Result()) {
                            case HCI_NO_CONNECTION:
                                result = Core::ERROR_ALREADY_RELEASED;
                                TRACE(ControlFlow, (_T("Device not connected")));
                                break;
                            default:
                                result = Core::ERROR_ASYNC_FAILED;
                                TRACE(ControlFlow, (_T("Disconnect command failed [%d]"), disconnect.Result()));
                                break;
                            }
                        }
                    }

                    BackgroundScan(true);

                    if (result != Core::ERROR_NONE) {
                        TRACE(ControlFlow, (_T("Unable to disconnect [%d]"), result));
                    }

                    ClearState(DISCONNECTING);
                } else {
                    TRACE(Trace::Information, (_T("Device is currently busy")));
                }

                return (result);
            }
            uint32_t Pair(const IBluetooth::IDevice::pairingcapabilities capabilities, const uint16_t timeout) override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(PAIRING) == Core::ERROR_NONE) {
                    BackgroundScan(false);
                    result = _parent->Connector().Pair(Address(), AddressType(), static_cast<Bluetooth::ManagementSocket::capabilities>(capabilities));
                    if (result == Core::ERROR_INPROGRESS) {
                        UpdateListener();
                        DeviceStateChanged(JBluetoothControl::devicestate::PAIRING);
                        TRACE(Trace::Information, (_T("Pairing of device %s in progress..."), RemoteId().c_str()));
                        _abortPairingJob.Schedule([this](){
                            TRACE(Trace::Information, (_T("Timeout! Aborting pairing!"), RemoteId().c_str()));
                            AbortPairing();
                            BackgroundScan(true);
                        }, (1000L * timeout));
                        result = Core::ERROR_NONE;
                    } else {
                        if (result == Core::ERROR_ALREADY_CONNECTED) {
                            TRACE(Trace::Information, (_T("Device is already paired")));
                        } else if (result != Core::ERROR_NONE) {
                            TRACE(Trace::Error, (_T("Failed to pair device %s [%d]"), RemoteId().c_str(), result));
                        }
                    }

                    if (result != Core::ERROR_REQUEST_SUBMITTED) {
                        BackgroundScan(true);
                        ClearState(PAIRING);
                    }
                } else {
                    TRACE(Trace::Information, (_T("Device is currently busy")));
                }

                return (result);
            }
            uint32_t AbortPairing() override
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                if (SetState(PAIRING) != Core::ERROR_NONE) {
                    result = _parent->Connector().PairAbort(Address(), AddressType());
                    if (result == Core::ERROR_INPROGRESS) {
                        TRACE(Trace::Information, (_T("Aborting pairing of device %s in progress..."), RemoteId().c_str()));
                    } else if (result != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Failed to abort pairing [%d]"), result));
                    }

                    if (result != Core::ERROR_INPROGRESS) {
                        ClearState(PAIRING);
                    }
                } else {
                    TRACE(Trace::Information, (_T("Not currently pairing to this device")));
                }

                return (result);
            }
            uint32_t Unpair() override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(UNPAIRING) == Core::ERROR_NONE) {
                    PurgeSecurityKeys();

                    result = _parent->Connector().Unpair(Address(), AddressType());
                    if (result == Core::ERROR_ALREADY_RELEASED) {
                        TRACE(Trace::Information, (_T("Device is not paired")));
                    } else if (result != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Failed to unpair device %s [%d]"), RemoteId().c_str(), result));
                    } else {
                        TRACE(Trace::Information, (_T("Device unpaired")));
                    }

                    ClearState(UNPAIRING);
                } else {
                    TRACE(Trace::Information, (_T("Device is currently busy")));
                }

                return (result);
            }
            void Passkey(const uint32_t passkey) override
            {
                TRACE(Trace::Information, (_T("Passkey reply: %06d"), passkey));
                _userReplyJob.Submit([this, passkey]() {
                    _parent->Connector().PasskeyReply(Address(), AddressType(), passkey);
                });
            }
            void ConfirmPasskey(const bool confirm) override
            {
                TRACE(Trace::Information, (_T("Passkey confirmation reply: %s"), confirm? _T("YES") : _T("NO")));
                _userReplyJob.Submit([this, confirm]() {
                    _parent->Connector().PasskeyConfirmReply(Address(), AddressType(), confirm);
                });
            }
            uint32_t Callback(IBluetooth::IDevice::ICallback* callback) override
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                _state.Lock();

                if (callback == nullptr) {
                    if (_callback != nullptr) {
                        _callback->Release();
                        _callback = nullptr;
                        result = Core::ERROR_NONE;
                    }
                } else if (_callback == nullptr) {
                    _callback = callback;
                    _callback->AddRef();
                    result = Core::ERROR_NONE;
                }

                _state.Unlock();

                return (result);
            }
            uint32_t Callback(IBluetooth::IDevice::ISecurityCallback* callback) override
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                _state.Lock();

                if (callback == nullptr) {
                    if (_securityCallback != nullptr) {
                        _securityCallback->Release();
                        _securityCallback = nullptr;
                        result = Core::ERROR_NONE;
                    }
                }
                else if (_securityCallback == nullptr) {
                    _securityCallback = callback;
                    _securityCallback->AddRef();
                    result = Core::ERROR_NONE;
                }

                _state.Unlock();

                return (result);
            }

            BEGIN_INTERFACE_MAP(DeviceImpl)
            INTERFACE_ENTRY(Exchange::IBluetooth::IDevice)
            END_INTERFACE_MAP

        public:
            inline bool operator==(const Bluetooth::Address& rhs) const
            {
                return (_remote == rhs);
            }
            inline bool operator!=(const Bluetooth::Address& rhs) const
            {
                return (!operator==(rhs));
            }
            inline bool operator==(const std::pair<const Bluetooth::Address&, const bool>& rhs) const
            {
                return ((_remote == rhs.first) && (LowEnergy() == rhs.second));
            }
            inline bool operator!=(const std::pair<const Bluetooth::Address&, const bool>& rhs) const
            {
                return (!operator==(rhs));
            }
            Bluetooth::HCISocket::FeatureIterator Features() const
            {
                return (Bluetooth::HCISocket::FeatureIterator(static_cast<uint8_t>(sizeof(_features)), _features));
            }
            void Capabilities(const uint8_t capability, const uint8_t authentication, const uint8_t oobData)
            {
                _state.Lock();
                _capabilities = capability;
                _authentication = authentication;
                _oobData = oobData;
                _state.Unlock();
            }

        protected:
            friend class ControlSocket;

            inline ControlSocket& Connector() {
                return (_parent->Connector());
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
            void ClearState()
            {
                _state.Lock();

                if ((IsConnected() == false) && ((_state & ACTION_MASK) == 0)) {
                    _state.SetState(static_cast<state>(0));
                }

                _state.Unlock();
            }
            uint32_t WaitState(const uint32_t state, const uint32_t waitTime)
            {
                return (_state.WaitState(state, waitTime));
            }

        protected:
            void RequestPasskey()
            {
                TRACE(Trace::Information, (_T("Pairing with device %s; requesting passkey..."), RemoteId().c_str()));
                _userRequestJob.Submit([this](){
                    Callback<IBluetooth::IDevice::ISecurityCallback>(SecurityCallback(), [&](IBluetooth::IDevice::ISecurityCallback* cb) {
                        cb->PasskeyRequest();
                    });

                    _parent->JSONRPC().PasskeyRequest(RemoteId(), DeviceType(Type()));
                });
            }
            void RequestPasskeyConfirm(const uint32_t passkey)
            {
                TRACE(Trace::Information, (_T("Pairing with device %s; confirm passkey %06d? (YES/NO)"), RemoteId().c_str(), passkey));

                // Request passkey confirmation from client or, if auto confirm is enabled, reply already.
                if (_parent->Configuration().AutoPasskeyConfirm.Value() == true) {
                    TRACE(Trace::Information, (_T("Auto-confirm enabled, accepting the passkey!")));
                    ConfirmPasskey(true);
                } else {
                    TRACE(Trace::Information, (_T("Waiting for user confirmation of the passkey...")));
                    _userRequestJob.Submit([this, passkey]() {
                        Callback<IBluetooth::IDevice::ISecurityCallback>(SecurityCallback(), [&](IBluetooth::IDevice::ISecurityCallback* cb) {
                            cb->PasskeyConfirmRequest(passkey);
                        });

                       _parent->JSONRPC().PasskeyConfirmRequest(RemoteId(), DeviceType(Type()), passkey);
                    });
                }
            }
            void PairingComplete(const uint8_t status = 0)
            {
                _abortPairingJob.Revoke();

                if (status == 0) {
                    if (IsPaired() == true) {
                        TRACE(Trace::Information, (_T("Device %s pairing successful"), _remote.ToString().c_str()));
                    }
                } else {
                    if (status == MGMT_STATUS_CANCELLED) {
                        TRACE(Trace::Information, (_T("Device %s pairing aborted!"), _remote.ToString().c_str()));
                    } else {
                        TRACE(Trace::Error, (_T("Device %s pairing failed! [0x%02x]"), _remote.ToString().c_str(), status));
                    }

                    PairedChange();
                }

                ClearState(PAIRING);
            }
            void PairedChange()
            {
                _parent->PairedChange(this);
                UpdateListener();
                DeviceStateChanged(IsPaired()? JBluetoothControl::devicestate::PAIRED : JBluetoothControl::devicestate::UNPAIRED);
            }
            void Connection(const uint16_t handle)
            {
                bool updated = false;

                _state.Lock();

                if ( (_handle == static_cast<uint16_t>(~0)) ^ (handle == static_cast<uint16_t>(~0)) ) {

                    TRACE(DeviceFlow, (_T("The connection state changed to: %d"), handle));
                    // This is a proper, Connect or disconnect. A real state change..
                    _handle = handle;
                    ClearState(CONNECTING);
                    updated = true;
                }
                else {
                    TRACE(DeviceFlow, (_T("The connection handle has changed during the runtime, from: %d to: %d"), _handle, handle));
                    _handle = handle;
                }

                _state.Unlock();

                if (updated == true) {
                    UpdateListener();
                    DeviceStateChanged(JBluetoothControl::devicestate::CONNECTED);

                    _autoConnectJob.Submit([this]() {
                        // Each time a device connects evalute whether background scan is needed.
                        BackgroundScan(true);
                    });
                }
            }
            void Disconnection(const uint8_t reason)
            {
                TRACE(DeviceFlow, (_T("Disconnected connection %d, reason: %d"), _handle, reason));

                _state.Lock();
                ClearState(CONNECTING);
                ClearState(DISCONNECTING);
                _handle = ~0;
                _state.Unlock();

                JBluetoothControl::disconnectreason disconnReason;
                if (reason == HCI_CONNECTION_TIMEOUT) {
                    disconnReason = JBluetoothControl::disconnectreason::CONNECTION_TIMEOUT;
                } else if (reason == HCI_AUTHENTICATION_FAILURE) {
                    disconnReason = JBluetoothControl::disconnectreason::AUTHENTICATION_FAILURE;
                } else if (reason == HCI_OE_USER_ENDED_CONNECTION) {
                    disconnReason = JBluetoothControl::disconnectreason::TERMINATED_BY_REMOTE;
                } else if (reason == HCI_OE_LOW_RESOURCES) {
                    disconnReason = JBluetoothControl::disconnectreason::REMOTE_LOW_ON_RESOURCES;
                } else if (reason == HCI_OE_POWER_OFF) {
                    disconnReason = JBluetoothControl::disconnectreason::REMOTE_POWERED_OFF;
                } else {
                    disconnReason = JBluetoothControl::disconnectreason::TERMINATED_BY_HOST;
                }

                UpdateListener();
                DeviceStateChanged(JBluetoothControl::devicestate::DISCONNECTED, disconnReason);

               _autoConnectJob.Submit([this]() {
                    // Each time a device disconnects evalute whether background scan is needed.
                    BackgroundScan(true);
                });
            }
            bool Class(const uint32_t classId, bool notifyListener = true)
            {
                bool updated = false;

                _state.Lock();

                if ((classId != 0) && (classId != _class)) {
                    _class = classId;
                    TRACE(DeviceFlow, (_T("Device class updated to: '%6x'"), _class));
                    updated = true;
                }

                _state.Unlock();

                if ((notifyListener == true) && (updated == true)) {
                    UpdateListener();
                }

                return (updated);
            }
            bool Name(const string& name, bool notifyListener = true)
            {
                bool updated = false;

                _state.Lock();

                if ((name.empty() == false) && (name != _name)) {
                    _name = name;
                    TRACE(DeviceFlow, (_T("Device name updated to: '%s'"), _name.c_str()));
                    updated = true;
                }

                _state.Unlock();

                if ((notifyListener == true) && (updated == true)) {
                    UpdateListener();
                }

                return (updated);
            }
            bool Update(const Bluetooth::EIR& eir, bool notifyListener = true)
            {
                bool updated = false;

                updated |= Class(eir.Class(), false);
                updated |= Name(eir.CompleteName(), false);

                _state.Lock();

                if ((eir.UUIDs() != _uuids) && (eir.UUIDs().empty() == false)) {
                    _uuids.clear();

                    TRACE(DeviceFlow, (_T("Supported services UUIDs:")));
                    for (auto uuid : eir.UUIDs()) {
                        TRACE(DeviceFlow, (_T(" - %s"), uuid.ToString().c_str()));
                        _uuids.emplace_back(uuid);
                    }

                    updated = true;
                }

                _state.Unlock();

                if ((updated == true) && (notifyListener == true)) {
                    UpdateListener();
                }

                return (updated);
            }
            void Features(const uint8_t length, const uint8_t feature[])
            {
                bool updated = false;
                uint8_t copyLength = std::min(length, static_cast<uint8_t>(sizeof(_features)));

                _state.Lock();

                if (::memcmp(_features, feature, copyLength) != 0) {
                    ::memcpy(_features, feature, copyLength);
                    updated = true;
                }

                _state.Unlock();

                if (updated == true) {
                    TRACE(DeviceFlow, (_T("The device features are updated")));
                }
            }
            void AutoConnect(bool enable)
            {
                _state.Lock();

                if (_autoConnect != enable) {
                    _autoConnect = enable;
                    TRACE(ControlFlow, (_T("Device %swhitelisted for autoconnection"), (enable? "" : "un")));
                }

                _state.Unlock();
            }

        protected:
            void BackgroundScan(bool evaluate)
            {
#if defined(USE_KERNEL_CONNECTION_CONTROL)
                // Won't stop discovery explicitly in kernel mode,
                // simply Add/Remove device when requested to evaluate background scan.
                if ((evaluate == true) && (IsConnected() == false)) {
                    SetAutoConnect(IsPaired() && IsAutoConnectable());
                }
#else
                _parent->Connector().BackgroundScan(evaluate);
#endif
            }

            void DeviceStateChanged(const JBluetoothControl::devicestate state,
                const JBluetoothControl::disconnectreason reason = JBluetoothControl::disconnectreason{})
            {
                Core::EnumerateType<JBluetoothControl::devicetype> enumType(Type());
                Core::EnumerateType<JBluetoothControl::devicestate> enumState(state);
                TRACE(Trace::Information, (_T("Device [%s, %s] state changed to '%s'"), RemoteId().c_str(), enumType.Data(), enumState.Data()));

                _parent->JSONRPC().DeviceStateChanged(RemoteId(), DeviceType(Type()), state, reason);
            }

        private:
            // Depending on the device type, different keys are exchanged. Leave it up to
            // the specific implementation to implement the needed keys!
            virtual uint32_t SecurityKey(const Bluetooth::LinkKey& key) = 0;
            virtual uint32_t SecurityKey(const Bluetooth::LongTermKey& key) = 0;
            virtual uint32_t SecurityKey(const Bluetooth::IdentityKey& key) = 0;
            virtual void PurgeSecurityKeys() = 0;

#if defined(USE_KERNEL_CONNECTION_CONTROL)
            virtual Bluetooth::ManagementSocket::autoconnmode AutoConnectMode() const = 0;
#endif

            void UpdateListener()
            {
                _deviceUpdateJob.Submit([this](){
                    Callback<IBluetooth::IDevice::ICallback>(Callback(), [](IBluetooth::IDevice::ICallback* cb) {
                        cb->Updated();
                    });
                });
            }
#if defined(USE_KERNEL_CONNECTION_CONTROL)
            void SetAutoConnect(const bool enable)
            {
                _state.Lock();

                if (enable == true) {
                    if (_autoConnectionSubmitted == false) {
                        uint32_t result = _parent->Connector().Control().AddDevice(Address(), AddressType(), AutoConnectMode());
                        if (result != Core::ERROR_NONE) {
                            TRACE(DeviceFlow, (_T("Could not add device %s [%d]"), RemoteId().c_str(), result));
                        } else {
                            TRACE(DeviceFlow, (_T("Enabled autoconnect of device %s"), RemoteId().c_str()));
                            _autoConnectionSubmitted = true;
                        }
                    }
                } else {
                    uint32_t result = _parent->Connector().Control().RemoveDevice(Address(), AddressType());
                    if (result != Core::ERROR_NONE) {
                        TRACE(DeviceFlow, (_T("Could not remove device %s [%d]"), RemoteId().c_str(), result));
                    } else {
                        TRACE(DeviceFlow, (_T("Disabled autoconnect of device %s"), RemoteId().c_str()));
                    }

                    _autoConnectionSubmitted = false;
                }

                _state.Unlock();
            }
#endif // defined(USE_KERNEL_CONNECTION_CONTROL)

            template<typename ICALLBACK>
            void Callback(ICALLBACK* callbackInstance, const std::function<void(ICALLBACK* cb)>& action)
            {
                ICALLBACK* callback = nullptr;

                _state.Lock();

                if (_callback != nullptr) {
                    callback = callbackInstance;
                    callback->AddRef();
                }

                _state.Unlock();

                if (callback != nullptr) {
                    action(callback);
                    callback->Release();
                }
            }
            IBluetooth::IDevice::ISecurityCallback* SecurityCallback() const
            {
                return (_securityCallback);
            }
            IBluetooth::IDevice::ICallback* Callback() const
            {
                return (_callback);
            }

        protected:
            BluetoothControl* _parent;
            Core::StateTrigger<state> _state;

        private:
            string _name;
            Bluetooth::Address::type _type;
            uint16_t _deviceId;
            uint32_t _class;
            uint16_t _handle;
            Bluetooth::Address _remote;
            std::list<Bluetooth::UUID> _uuids;
            uint8_t _features[8];
            uint8_t _capabilities;
            uint8_t _authentication;
            uint8_t _oobData;
#if defined(USE_KERNEL_CONNECTION_CONTROL)
            bool _autoConnectionSubmitted;
#endif
            bool _autoConnect;
            IBluetooth::IDevice::ICallback* _callback;
            IBluetooth::IDevice::ISecurityCallback* _securityCallback;
            DecoupledJob _deviceUpdateJob;
            DecoupledJob _autoConnectJob;
            DecoupledJob _abortPairingJob;
            DecoupledJob _userRequestJob;
            DecoupledJob _userReplyJob;
        }; // class DeviceImpl

    public:
        class DeviceRegular : public DeviceImpl, public Exchange::IBluetooth::IDevice::IClassic {
        public:
            DeviceRegular() = delete;
            DeviceRegular(const DeviceRegular&) = delete;
            DeviceRegular& operator=(const DeviceRegular&) = delete;

            DeviceRegular(BluetoothControl* parent, const uint16_t deviceId, const Bluetooth::Address& address)
                : DeviceImpl(parent, Bluetooth::Address::BREDR_ADDRESS, deviceId, address)
                , _securityCallback(nullptr)
                , _linkKeys()
                , _userRequestJob()
                , _userReplyJob()
            {
                RemoteName();
            }
            DeviceRegular(BluetoothControl* parent, const uint16_t deviceId, const Bluetooth::Address& address, const DeviceImpl::Config* config)
                : DeviceImpl(parent, Bluetooth::Address::BREDR_ADDRESS, deviceId, address)
                , _securityCallback(nullptr)
                , _linkKeys()
                , _userRequestJob()
                , _userReplyJob()
            {
                ASSERT(config != nullptr);

                Name(config->Name.Value());
                Class(config->Class.Value());
                config->Get(config->LinkKeys, address, Bluetooth::Address::BREDR_ADDRESS, _linkKeys);
            }
            ~DeviceRegular() = default;

        public:
            // IBluetooth::IDevice overrides
            // ---------------------------------------------------------------------------------------------
            bool IsPaired() const override
            {
                return (_linkKeys.Entries() > 0);
            }
            uint32_t Connect() override
            {
                uint32_t result = Core::ERROR_GENERAL;

                if (SetState(CONNECTING) == Core::ERROR_NONE) {
                    if (IsConnected() == false) {
                        if (IsPaired() == true) {
                            BackgroundScan(false);

                            Bluetooth::HCISocket::Command::Connect connect;
                            connect.Clear();
                            connect->bdaddr = *(Locator().Data());
                            connect->pkt_type = htobs(HCI_DM1 | HCI_DM3 | HCI_DM5 | HCI_DH1 | HCI_DH3 | HCI_DH5);
                            connect->pscan_rep_mode = 0x02;
                            connect->clock_offset = 0x0000;
                            connect->role_switch = 0x01;

                            result = _parent->Connector().Exchange(MAX_ACTION_TIMEOUT, connect, connect);
                            if (result == Core::ERROR_NONE) {
                                const uint16_t handle = btohs(connect.Response().handle);
                                if ((connect.Result() == 0) && (handle != 0)){
                                    Connection(handle);
                                } else {
                                    TRACE(ControlFlow, (_T("Connect command failed [%d]"), connect.Result()));
                                    result = Core::ERROR_ASYNC_FAILED;
                                }
                            } else {
                                if (result == Core::ERROR_TIMEDOUT) {
                                    TRACE(Trace::Information, (_T("Waiting for connection... ")));
                                    result = Core::ERROR_REQUEST_SUBMITTED;
                                }
                            }

                            if (result != Core::ERROR_NONE) {
                                BackgroundScan(true);

                                if (result != Core::ERROR_REQUEST_SUBMITTED) {
                                    TRACE(Trace::Error, (_T("Unable to connect [%d]"), result));
                                }
                            }
                        } else {
                            result = Core::ERROR_ILLEGAL_STATE;
                            TRACE(Trace::Error, (_T("Device not paired!")));
                        }
                    } else {
                        result = Core::ERROR_ALREADY_CONNECTED;
                        TRACE(Trace::Error, (_T("Device already connected!")));
                    }

                    ClearState(CONNECTING);
                } else {
                    TRACE(Trace::Information, (_T("Device is currently busy")));
                }

                return (result);
            }

        public:
            // IBluetooth::IDevice::IClassic overrides
            // ---------------------------------------------------------------------------------------------
            void PINCode(const string& pinCode) override
            {
                _userReplyJob.Submit([this, pinCode](){
                    _parent->Connector().PINCodeReply(Address(), AddressType(), pinCode);
                });
            }
            uint32_t Callback(IBluetooth::IDevice::IClassic::ISecurityCallback* callback) override
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                _state.Lock();

                if (callback == nullptr) {
                    if (_securityCallback != nullptr) {
                        _securityCallback->Release();
                        _securityCallback = nullptr;
                        result = Core::ERROR_NONE;
                    }
                }
                else if (_securityCallback == nullptr) {
                    _securityCallback = callback;
                    _securityCallback->AddRef();
                    result = Core::ERROR_NONE;
                }

                _state.Unlock();

                return (result);
            }

        public:
            // DeviceImpl overrides
            // ---------------------------------------------------------------------------------------------
            void SecurityKey(Bluetooth::LinkKeys& keys) const override
            {
                keys.Add(_linkKeys);
            }
            void SecurityKey(Bluetooth::LongTermKeys& /* keys */) const override
            {
            }
            const Bluetooth::IdentityKey& IdentityKey() const override
            {
                static Bluetooth::IdentityKey emptyKey;
                return (emptyKey);
            }

        public:
            void RequestPINCode()
            {
                TRACE(Trace::Information, (_T("Pairing with legacy device %s; requesting PIN code..."), RemoteId().c_str()));
                _userRequestJob.Submit([this]() {
                    Callback([&](IBluetooth::IDevice::IClassic::ISecurityCallback* cb) {
                        cb->PINCodeRequest();
                    });

                    _parent->JSONRPC().PINCodeRequest(RemoteId(), DeviceType(Type()));
                });
            }

        public:
            BEGIN_INTERFACE_MAP(DeviceRegular)
            INTERFACE_ENTRY(Exchange::IBluetooth::IDevice::IClassic)
            NEXT_INTERFACE_MAP(DeviceImpl)

        private:
            void RemoteName()
            {
                Bluetooth::HCISocket::Command::RemoteName cmd;

                cmd.Clear();
                cmd->bdaddr = *(Locator().Data());
                cmd->pscan_mode = 0x00;
                cmd->pscan_rep_mode = 0x02;
                cmd->clock_offset = 0x0000;

                _parent->Connector().Execute<Bluetooth::HCISocket::Command::RemoteName>(MAX_ACTION_TIMEOUT, cmd, [&](Bluetooth::HCISocket::Command::RemoteName& /* cmd */, const uint32_t error) {
                    if (error != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Failed to request remote name [%d]"), error));
                    }
                });
            }
            void Callback(const std::function<void(IBluetooth::IDevice::IClassic::ISecurityCallback* cb)>& action)
            {
                IBluetooth::IDevice::IClassic::ISecurityCallback* callback = nullptr;

                _state.Lock();

                if (_securityCallback != nullptr) {
                    callback = _securityCallback;
                    callback->AddRef();
                }

                _state.Unlock();

                if (callback != nullptr) {
                    action(callback);
                    callback->Release();
                }
            }
            IBluetooth::IDevice::IClassic::ISecurityCallback* SecurityCallback() const
            {
                return (_securityCallback);
            }

        private:
            // DeviceImpl overrides
            // ---------------------------------------------------------------------------------------------
            uint32_t SecurityKey(const Bluetooth::LinkKey& lk) override
            {
                uint32_t result = Core::ERROR_GENERAL;
                if (lk.IsValid() == true) {
                    result = Core::ERROR_NONE;
                    bool isPaired = IsPaired();
                    _linkKeys.Add(lk);
                    if (IsPaired() ^ isPaired) {
                        PairedChange();
                        if (IsPaired() == true) {
                            PairingComplete();
                        }
                    }
                }
                return (result);
            }
            uint32_t SecurityKey(const Bluetooth::LongTermKey& /* key */) override
            {
                TRACE(DeviceFlow, (_T("Not expecting a Long Term Key on a BR/EDR device")));
                return (Core::ERROR_GENERAL);
            }
            uint32_t SecurityKey(const Bluetooth::IdentityKey& /* key */) override
            {
                TRACE(DeviceFlow, (_T("Not expecting an Identity Key on a BR/EDR device")));
                return (Core::ERROR_GENERAL);
            }
            void PurgeSecurityKeys() override
            {
                bool isPaired = IsPaired();
                _linkKeys.Clear();
                if (IsPaired() ^ isPaired) {
                    PairedChange();
                }
            }
#if defined(USE_KERNEL_CONNECTION_CONTROL)
            Bluetooth::ManagementSocket::autoconnmode AutoConnectMode() const override
            {
                return (Bluetooth::ManagementSocket::DIRECT);
            }
#endif

        private:
            IBluetooth::IDevice::IClassic::ISecurityCallback* _securityCallback;
            Bluetooth::LinkKeys _linkKeys;
            DecoupledJob _userRequestJob;
            DecoupledJob _userReplyJob;
        }; // class DeviceRegular

    public:
        class DeviceLowEnergy : public DeviceImpl, public Exchange::IBluetooth::IDevice::ILowEnergy {
        public:
            DeviceLowEnergy() = delete;
            DeviceLowEnergy(const DeviceLowEnergy&) = delete;
            DeviceLowEnergy& operator=(const DeviceLowEnergy&) = delete;

            DeviceLowEnergy(BluetoothControl* parent, const uint16_t deviceId, const Bluetooth::Address& address)
                : DeviceImpl(parent, Bluetooth::Address::LE_PUBLIC_ADDRESS, deviceId, address)
                , _ltks()
                , _irk()
            {
            }
            DeviceLowEnergy(BluetoothControl* parent, const uint16_t deviceId, const Bluetooth::Address& address, const DeviceImpl::Config* config)
                : DeviceImpl(parent, Bluetooth::Address::LE_PUBLIC_ADDRESS, deviceId, address)
                , _ltks()
                , _irk()
            {
                ASSERT(config != nullptr);

                config->Get(config->LongTermKeys, Address(), AddressType(), _ltks);
                Name(config->Name.Value());
                Class(config->Class.Value());
                _irk = Bluetooth::IdentityKey(Address(), AddressType(), config->IdentityKey.Value());
            }
            ~DeviceLowEnergy() override = default;

        public:
            // IBluetooth::IDevice overrides
            bool IsPaired() const override
            {
                return (_ltks.Entries() >= 1);
            }
            uint32_t Connect() override
            {
                uint32_t result = Core::ERROR_GENERAL;

                if (IsPaired() == true) {
                    AutoConnect(true);
                    result = Core::ERROR_REQUEST_SUBMITTED;
                }

                if (SetState(CONNECTING) == Core::ERROR_NONE) {
                    if (IsConnected() == false) {
                        if (IsPaired() == true) {
                            BackgroundScan(false);

                            const BluetoothControl::Config& config = _parent->Configuration();

                            Bluetooth::HCISocket::Command::ConnectLE connect;
                            connect.Clear();
                            connect->interval = htobs(4*0x0010);
                            connect->window = htobs(0x0010); // Have some back off time, so the connection does not hog the link layer completely.
                            connect->initiator_filter = 0;
                            connect->peer_bdaddr_type = LE_PUBLIC_ADDRESS;
                            connect->peer_bdaddr = *(Locator().Data());
                            connect->own_bdaddr_type = LE_PUBLIC_ADDRESS;
                            connect->min_interval = htobs(config.LowEnergy.MinInterval.Value());
                            connect->max_interval = htobs(config.LowEnergy.MaxInterval.Value());
                            connect->latency = htobs(config.LowEnergy.Latency.Value());
                            connect->supervision_timeout = htobs(config.LowEnergy.Timeout.Value() / 10); // in centiseconds
                            connect->min_ce_length = htobs(0);
                            connect->max_ce_length = htobs(0);

                            result = Connector().Exchange(MAX_ACTION_TIMEOUT, connect, connect);
                            if (result == Core::ERROR_NONE) {
                                if (connect.Result() == 0) {
                                    Connection(btohs(connect.Response().handle));
                                } else {
                                    TRACE(ControlFlow, (_T("ConnectLE command failed [%d]"), connect.Result()));
                                    result = Core::ERROR_ASYNC_FAILED;
                                }
                            }
                            else {
                                if (result == Core::ERROR_TIMEDOUT) {
                                    // The device is not nearby or active, but we just whitelisted it, so keep searching...
                                    Bluetooth::HCISocket::Command::ConnectLECancel connectCancel;
                                    connectCancel.Clear();

                                    result = Connector().Exchange(MAX_ACTION_TIMEOUT, connectCancel, connectCancel);
                                    if ((result != Core::ERROR_NONE) || (connectCancel.Result() != 0)) {
                                        TRACE(Trace::Error, (_T("Failed to cancel connection")));
                                    } else if (connectCancel.Response() != 0) {
                                        TRACE(Trace::Error, (_T("Controller failed to cancel connection (%i)"), connectCancel.Response()));
                                    } else {
                                        TRACE(Trace::Information, (_T("Canceled connection attempt")));
                                    }

                                    TRACE(Trace::Information, (_T("Waiting for connection...")));
                                    result = Core::ERROR_REQUEST_SUBMITTED;
                                }
                            }

                            if (result != Core::ERROR_NONE) {
                                BackgroundScan(true); // also when waiting for BLE connection

                                if (result != Core::ERROR_REQUEST_SUBMITTED) {
                                    TRACE(Trace::Error, (_T("Unable to connect [%d]"), result));
                                }
                            }
                        } else {
                            result = Core::ERROR_ILLEGAL_STATE;
                            TRACE(Trace::Error, (_T("Device is not paired")));
                        }
                    } else {
                        result = Core::ERROR_ALREADY_CONNECTED;
                        TRACE(Trace::Error, (_T("Device is already connected!")));
                    }

                    ClearState(CONNECTING);
                } else {
                    TRACE(Trace::Error, (_T("Device is currently busy")));
                }

                return (result);
            }

        public:
            uint16_t Appearance() const
            {
                // TODO
                return (0);
            }

        public:
            // DeviceImpl overrides
            void SecurityKey(Bluetooth::LinkKeys& /* key */ ) const override
            {
            }
            void SecurityKey(Bluetooth::LongTermKeys& key) const override
            {
                key.Add(_ltks);
            }
            const Bluetooth::IdentityKey& IdentityKey() const override
            {
                return (_irk);
            }

            BEGIN_INTERFACE_MAP(DeviceLowEnergy)
            INTERFACE_ENTRY(Exchange::IBluetooth::IDevice::ILowEnergy)
            NEXT_INTERFACE_MAP(DeviceImpl)

        private:
            uint32_t SecurityKey(const Bluetooth::LinkKey& /* lk */) override
            {
                TRACE(DeviceFlow, (_T("Not expecting a Link Key on a BLE device")));
                return (Core::ERROR_GENERAL);
            }
            uint32_t SecurityKey(const Bluetooth::LongTermKey& ltk) override
            {
                uint32_t result = Core::ERROR_GENERAL;
                if (ltk.IsValid() == true) {
                    result = Core::ERROR_NONE;
                    bool isPaired = IsPaired();
                    _ltks.Add(ltk);
                    if (IsPaired() ^ isPaired) {
                        PairedChange();
                        if (IsPaired() == true) {
                            PairingComplete();
                        }
                    }
                }
                return (result);
            }
            uint32_t SecurityKey(const Bluetooth::IdentityKey& irk) override
            {
                uint32_t result = Core::ERROR_GENERAL;
                if (irk.IsValid() == true) {
                    result = Core::ERROR_NONE;
                    _irk = irk;
                }
                return (result);
            }
            void PurgeSecurityKeys() override
            {
                bool isPaired = IsPaired();
                _ltks.Clear();
                _irk = Bluetooth::IdentityKey();
                if (IsPaired() ^ isPaired) {
                    PairedChange();
                }
            }
#if defined(USE_KERNEL_CONNECTION_CONTROL)
            Bluetooth::ManagementSocket::autoconnmode AutoConnectMode() const override
            {
                return (Bluetooth::ManagementSocket::DIRECT);
            }
#endif

        private:
            Bluetooth::LongTermKeys _ltks;
            Bluetooth::IdentityKey _irk;
        }; // class DeviceLowEnergy

    public:
        class Status : public Core::JSON::Container {
        public:
            class Property : public Core::JSON::Container {
            public:
                Property& operator=(const Property&) = delete;
                Property()
                    : Name()
                    , Supported(false)
                    , Enabled(false)
                {
                    Add(_T("name"), &Name);
                    Add(_T("supported"), &Supported);
                    Add(_T("enabled"), &Enabled);
                }
                Property(const string& name, const bool supported, const bool enabled)
                    : Name(name)
                    , Supported(supported)
                    , Enabled(enabled)
                {
                    Add(_T("name"), &Name);
                    Add(_T("supported"), &Supported);
                    Add(_T("enabled"), &Enabled);
                    Name = name;
                    Supported = supported;
                    if (supported == true) {
                        Enabled = enabled;
                    }
                }
                Property(const Property& copy)
                    : Core::JSON::Container()
                    , Name(copy.Name)
                    , Supported(copy.Supported)
                    , Enabled(copy.Enabled)
                {
                    Add(_T("name"), &Name);
                    Add(_T("supported"), &Supported);
                    Add(_T("enabled"), &Enabled);
                }
                ~Property() = default;

            public:
                Core::JSON::String Name;
                Core::JSON::Boolean Supported;
                Core::JSON::Boolean Enabled;
            }; // class Property

        public:
            Status(const Status&) = delete;
            Status& operator=(const Status&) = delete;
            Status()
                : Scanning()
                , Devices()
                , Name()
                , ShortName()
                , Version(0)
                , Address()
                , DeviceClass(0)
                , Properties()
            {
                Add(_T("scanning"), &Scanning);
                Add(_T("devices"), &Devices);
                Add(_T("name"), &Name);
                Add(_T("shortname"), &ShortName);
                Add(_T("version"), &Version);
                Add(_T("address"), &Address);
                Add(_T("deviceclass"), &DeviceClass);
                Add(_T("properties"), &Properties);
            }
            ~Status() = default;

        public:
            void AddProperty(const string& name, const bool supported, const bool enabled)
            {
                Property newEntry (name, supported, enabled);
                Properties.Add(newEntry);
            }

        public:
            Core::JSON::Boolean Scanning;
            Core::JSON::ArrayType<DeviceImpl::Data> Devices;
            Core::JSON::String Name;
            Core::JSON::String ShortName;
            Core::JSON::DecUInt32 Version;
            Core::JSON::String Address;
            Core::JSON::DecUInt32 DeviceClass;
            Core::JSON::ArrayType<Property> Properties;
        }; // class Status

    public:
        class ClassicImpl : public Exchange::IBluetooth::IClassic {
        public:
            ClassicImpl(const ClassicImpl&) = delete;
            ClassicImpl& operator=(const ClassicImpl&) = delete;
            ClassicImpl(BluetoothControl& parent)
                : _parent(parent)
            {
            }
            ~ClassicImpl() = default;

        public:
            uint16_t Interface() const override
            {
                return (_parent.InterfaceID());
            }
            bool IsScanning(bool& limited) const override
            {
                return (_parent.Connector().IsInquiring(limited));
            }
            uint32_t Scan(const bool limited, const uint16_t duration /* sec */) override
            {
                return (_parent.Connector().Inquiry(duration, limited));
            }
            uint32_t StopScanning() override
            {
                return (_parent.Connector().StopInquiry());
            }
            bool IsInquiryScanning(bool& limited) const override
            {
                return (_parent.Connector().IsDiscoverable(limited));
            }
            uint32_t InquiryScan(const bool limited, const uint16_t duration /* sec */) override
            {
                return (_parent.Connector().SetDiscoverable(limited, duration));
            }
            uint32_t StopInquiryScanning() override
            {
                return (_parent.Connector().StopDiscoverable());
            }
            uint32_t Register(IBluetooth::IClassic::INotification* notification)
            {
                ASSERT(notification != nullptr);

                uint32_t result = Core::ERROR_NONE;
                _lock.Lock();
                auto it = std::find(_observers.begin(), _observers.end(), notification);
                ASSERT(it != _observers.end());
                if (it != _observers.end()) {
                    notification->Release();
                    _observers.erase(it);
                } else {
                    result = Core::ERROR_UNKNOWN_KEY;
                }
                _lock.Unlock();
                return (result);
            }
            uint32_t Unregister(IBluetooth::IClassic::INotification* notification)
            {
                ASSERT(notification != nullptr);

                _lock.Lock();
                _observers.push_back(notification);
                notification->AddRef();
                _lock.Unlock();
                return (Core::ERROR_NONE);
            }

        public:
            BEGIN_INTERFACE_MAP(ClasicImpl)
            INTERFACE_ENTRY(Exchange::IBluetooth::IClassic)
            END_INTERFACE_MAP

        private:
            BluetoothControl& _parent;
            Core::CriticalSection _lock;
            std::list<IBluetooth::IClassic::INotification*> _observers;
        }; // class ClassicImpl

        class LowEnergyImpl : public Exchange::IBluetooth::ILowEnergy {
        public:
            LowEnergyImpl(const LowEnergyImpl&) = delete;
            LowEnergyImpl& operator=(const LowEnergyImpl&) = delete;
            LowEnergyImpl(BluetoothControl& parent)
                : _parent(parent)
            {
            }
            ~LowEnergyImpl() = default;

        public:
            uint16_t Interface() const override
            {
                return (_parent.InterfaceID());
            }
            bool IsScanning(bool& limited) const override
            {
                return (_parent.Connector().IsScanning(limited));
            }
            uint32_t Scan(const bool limited, const uint16_t duration /* sec */) override
            {
                return (_parent.Connector().Scan(duration, limited, false));
            }
            uint32_t StopScanning() override
            {
                return (_parent.Connector().StopScanning());
            }
            bool IsAdvertising(bool& limited, bool& connectable) const override
            {
                return (_parent.Connector().IsAdvertising(limited, connectable));
            }
            uint32_t Advertise(const bool limited, const bool connectable, const uint16_t duration /* sec */) override
            {
                return (_parent.Connector().Advertise(limited, connectable, duration));
            }
            uint32_t StopAdvertising() override
            {
                return (_parent.Connector().StopAdvertising());
            }
            uint32_t Register(IBluetooth::ILowEnergy::INotification* notification)
            {
                uint32_t result = Core::ERROR_NONE;

                _lock.Lock();

                auto it = std::find(_observers.begin(), _observers.end(), notification);
                ASSERT(it != _observers.end());

                if (it != _observers.end()) {
                    notification->Release();
                    _observers.erase(it);
                } else {
                    result = Core::ERROR_UNKNOWN_KEY;
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t Unregister(IBluetooth::ILowEnergy::INotification* notification)
            {
                _lock.Lock();
                _observers.push_back(notification);
                notification->AddRef();
                _lock.Unlock();
                return (Core::ERROR_NONE);
            }

        public:
            BEGIN_INTERFACE_MAP(LowEnergyImpl)
            INTERFACE_ENTRY(Exchange::IBluetooth::ILowEnergy)
            END_INTERFACE_MAP

        private:
            BluetoothControl& _parent;
            Core::CriticalSection _lock;
            std::list<IBluetooth::ILowEnergy::INotification*> _observers;
        }; // class LowEnergyImpl

    private:
        class JSONRPCImplementation
                    : public Exchange::JSONRPC::IBluetoothControl
                    , public Exchange::JSONRPC::JBluetoothControl::IHandler {
        public:
            JSONRPCImplementation() = delete;
            JSONRPCImplementation(const JSONRPCImplementation&) = delete;
            JSONRPCImplementation& operator=(JSONRPCImplementation&) = delete;
            JSONRPCImplementation(JSONRPCImplementation&&) = delete;
            JSONRPCImplementation& operator=(JSONRPCImplementation&&) = delete;

            explicit JSONRPCImplementation(BluetoothControl& parent)
                : _parent(parent)
            {
            }
            ~JSONRPCImplementation() = default;

        private:
            using IDevice = Exchange::IBluetooth::IDevice;

            template<typename INTERFACE>
            INTERFACE* QueryInterface()
            {
                return (static_cast<Core::IUnknown&>(_parent).QueryInterface<INTERFACE>());
            }
            template<typename INTERFACE>
            const INTERFACE* QueryInterface() const
            {
                return (static_cast<Core::IUnknown&>(_parent).QueryInterface<INTERFACE>());
            }
            IDevice* Device(const string& address, const devicetype type)
            {
                return (_parent.Device(address, static_cast<IDevice::type>(type)));
            }
            const IDevice* Device(const string& address, const devicetype type) const
            {
                return (_parent.Device(address, static_cast<IDevice::type>(type)));
            }

            Core::hresult AdapterOp(const scantype type,
                const std::function<Core::hresult(Exchange::IBluetooth::IClassic* adapter)>& actionClassic,
                const std::function<Core::hresult(Exchange::IBluetooth::ILowEnergy* adapter)>& actionLowEnergy)
            {
                Core::hresult result = Core::ERROR_UNAVAILABLE;

                if (type == scantype::CLASSIC) {
                    Exchange::IBluetooth::IClassic* adapter = QueryInterface<Exchange::IBluetooth::IClassic>();
                    if (adapter != nullptr) {
                        result = actionClassic(adapter);
                        adapter->Release();
                    }
                }
                else if (type == scantype::LOW_ENERGY) {
                    Exchange::IBluetooth::ILowEnergy* adapter = QueryInterface<Exchange::IBluetooth::ILowEnergy>();
                    if (adapter != nullptr) {
                        result = actionLowEnergy(adapter);
                        adapter->Release();
                    }
                }

                return (result);
            }
            Core::hresult DeviceOp(const string& address, const devicetype type,
                const std::function<Core::hresult(IDevice* device)>& action)
            {
                Core::hresult result = Core::ERROR_UNKNOWN_KEY;

                IDevice* device = Device(address, type);
                if (device != nullptr) {
                    result = action(device);
                    device->Release();
                }

                return (result);
            }
            Core::hresult DeviceOp(const string& address, const devicetype type,
                const std::function<Core::hresult(const IDevice* device)>& action) const
            {
                Core::hresult result = Core::ERROR_UNKNOWN_KEY;

                const IDevice* device = Device(address, type);
                if (device != nullptr) {
                    result = action(device);
                    device->Release();
                }

                return (result);
            }
            template<typename DEVICE>
            Core::hresult DeviceOp(const string& address, const devicetype type,
                const std::function<Core::hresult(DEVICE*)>& action)
            {
                Core::hresult result = Core::ERROR_UNKNOWN_KEY;

                IDevice* device = Device(address, type);
                if (device != nullptr) {
                    DEVICE* classic = device->QueryInterface<DEVICE>();
                    if (classic != nullptr) {
                        result = action(classic);
                        classic->Release();
                    }

                    device->Release();
                }

                return (result);
            }
            template<typename DEVICE>
            Core::hresult DeviceOp(const string& address, const devicetype type,
                const std::function<Core::hresult(const DEVICE*)>& action) const
            {
                Core::hresult result = Core::ERROR_UNKNOWN_KEY;

                const IDevice* device = Device(address, type);
                if (device != nullptr) {
                    const DEVICE* classic = device->QueryInterface<DEVICE>();
                    if (classic != nullptr) {
                        result = action(classic);
                        classic->Release();
                    }

                    device->Release();
                }

                return (result);
            }

        public:
            // Discovery API
            Core::hresult SetDiscoverable(const scantype type, const Core::OptionalType<scanmode>& mode,
                const Core::OptionalType<bool>& connectable, const Core::OptionalType<uint16_t>& duration) override
            {
                return (AdapterOp(type, [&mode, &duration](Exchange::IBluetooth::IClassic* adapter) -> Core::hresult {
                    return (adapter->InquiryScan((mode == scanmode::LIMITED), duration));
                },
                [&mode, &connectable, &duration](Exchange::IBluetooth::ILowEnergy* adapter) -> Core::hresult {
                    return (adapter->Advertise((mode == scanmode::LIMITED), connectable, duration));
                }));
            }
            Core::hresult StopDiscoverable(const scantype type) override
            {
                return (AdapterOp(type, [](Exchange::IBluetooth::IClassic* adapter) -> Core::hresult {
                    return (adapter->StopInquiryScanning());
                },
                [](Exchange::IBluetooth::ILowEnergy* adapter) -> Core::hresult {
                    return (adapter->StopAdvertising());
                }));
            }

            // Scanning API
            Core::hresult Scan(const scantype type, const Core::OptionalType<scanmode>& mode,
                const Core::OptionalType<uint16_t>& duration) override
            {
                return (AdapterOp(type, [&mode, &duration](Exchange::IBluetooth::IClassic* adapter) -> Core::hresult {
                    return (adapter->Scan((mode == scanmode::LIMITED), duration));
                },
                [&mode, &duration](Exchange::IBluetooth::ILowEnergy* adapter) -> Core::hresult {
                    return (adapter->Scan((mode == scanmode::LIMITED), duration));
                }));
            }
            Core::hresult StopScanning(const scantype type) override
            {
                return (AdapterOp(type, [](Exchange::IBluetooth::IClassic* adapter) -> Core::hresult {
                    return (adapter->StopScanning());
                },
                [](Exchange::IBluetooth::ILowEnergy* adapter) -> Core::hresult {
                    return (adapter->StopScanning());
                }));
            }
            Core::hresult Forget(const string& address, const devicetype type) override
            {
                return (_parent.ForgetDevice(address, static_cast<IDevice::type>(type)));
            }

            // Connect & pairing API
            Core::hresult Connect(const string& address, const devicetype type) override
            {
                return (DeviceOp(address, type, [](IDevice* device) {
                    return (device->Connect());
                }));
            }
            Core::hresult Disconnect(const string& address, const devicetype type) override
            {
                return (DeviceOp(address, type, [](IDevice* device) {
                    return (device->Disconnect());
                }));
            }
            Core::hresult Pair(const string& address, const devicetype type,
                const Core::OptionalType<pairingcapabilities>& capabilities,
                const Core::OptionalType<uint16_t>& duration) override
            {
                return (DeviceOp(address, type, [&capabilities, &duration](IDevice* device) {
                    return (device->Pair(capabilities, duration));
                }));
            }
            Core::hresult Unpair(const string& address, const devicetype type) override
            {
                return (DeviceOp(address, type, [](IDevice* device) {
                    return (device->Unpair());
                }));
            }
            Core::hresult AbortPairing(const string& address, const devicetype type) override
            {
                return (DeviceOp(address, type, [](IDevice* device) {
                    return (device->AbortPairing());
                }));
            }
            Core::hresult ProvidePINCode(const string& address, const devicetype type, const string& secret) override
            {
                return (DeviceOp<IDevice::IClassic>(address, type, [&secret](IDevice::IClassic* device) {
                    device->PINCode(secret);
                    return (Core::ERROR_NONE);
                }));
            }
            Core::hresult ProvidePasskey(const string& address, const devicetype type, const uint32_t secret) override
            {
                return (DeviceOp(address, type, [&secret](IDevice* device) {
                    device->Passkey(secret);
                    return (Core::ERROR_NONE);
                }));
            }
            Core::hresult ConfirmPasskey(const string& address, const devicetype type, const bool accept) override
            {
                return (DeviceOp(address, type, [&accept](IDevice* device) {
                    device->ConfirmPasskey(accept);
                    return (Core::ERROR_NONE);
                }));
            }

            // Metadata API
            Core::hresult GetDeviceList(IDeviceIterator*& devices) const override
            {
                std::vector<device> localDevices;

                _parent.IterateDevices([&localDevices](DeviceImpl& device) -> bool {
                    localDevices.push_back({device.RemoteId(), DeviceType(device.Type()), device.Name() });
                    return (false);
                });

                if (localDevices.empty() == false) {
                    devices = Core::ServiceType<RPC::IteratorType<IDeviceIterator>>::Create<IDeviceIterator>(std::move(localDevices));
                }

                return (Core::ERROR_NONE);
            }
            Core::hresult GetDeviceInfo(string& address,
                        devicetype& type,
                        Core::OptionalType<string>& name,
                        Core::OptionalType<uint8_t>& version,
                        Core::OptionalType<uint16_t>& manufacturer,
                        Core::OptionalType<uint32_t>& cod,
                        Core::OptionalType<uint16_t>& appearance /* LE only */,
                        Core::OptionalType<IUUIDIterator*>& services,
                        bool& paired,
                        bool& connected) const override
            {
                return (DeviceOp(address, type, [&](const IDevice* device) -> Core::hresult {

                    connected = device->IsConnected();
                    paired = device->IsPaired();

                    const string localName = device->Name();
                    if (localName.empty() == false) {
                        name = std::move(localName);
                    }

                    if (device->Class() != 0) {
                        cod = device->Class();
                    }

                    // TODO
                    (void) version;
                    (void) manufacturer;

                    auto& uuids = (static_cast<const DeviceImpl*>(device))->UUIDs();
                    if (uuids.empty() == false) {
                        std::vector<string> localUuids;
                        localUuids.reserve(uuids.size());

                        for (const Bluetooth::UUID& item : uuids) {
                            localUuids.push_back(item.ToString());
                        }

                        services = Core::ServiceType<RPC::StringIterator>::Create<IUUIDIterator>(std::move(localUuids));
                    }

                    const IDevice::ILowEnergy* lowEnergy = QueryInterface<IDevice::ILowEnergy>();
                    if (lowEnergy != nullptr) {
                        if (lowEnergy->Appearance() != 0) {
                            appearance = lowEnergy->Appearance();
                        }

                        lowEnergy->Release();
                    }

                    return (Core::ERROR_NONE);
                }));
            }

            // Adapter metadata API
            Core::hresult Adapters(IAdapterIterator*& adapters) const override
            {
                std::vector<uint8_t> localAdapters;

                _parent.IterateAdapters([&localAdapters](uint16_t& adapter) -> bool {
                    localAdapters.push_back(adapter);
                    return (false);
                });

                if (localAdapters.empty() == false) {
                    adapters = Core::ServiceType<RPC::IteratorType<IAdapterIterator>>::Create<IAdapterIterator>(std::move(localAdapters));
                }

                return (Core::ERROR_NONE);
            }
            Core::hresult Adapter(const uint8_t adapter, adapterinfo& details) const override
            {
                uint32_t result = Core::ERROR_UNKNOWN_KEY;

                if (adapter == _parent.InterfaceID()) {

                    Bluetooth::ManagementSocket::Info info(_parent.Connector().Settings());

                    details.id = _parent.InterfaceID();
                    details.address = info.Address().ToString();
                    details.interface = ("hci" + Core::ToString(_parent.InterfaceID()));
                    details.type = _parent.Configuration().Type.Value();
                    details.version = info.Version();

                    if (info.Manufacturer() != 0) {
                        details.manufacturer = info.Manufacturer();
                    }

                    if (info.DeviceClass() != 0) {
                        details.cod = info.DeviceClass();
                    }

                    if (info.Name().empty() == false) {
                        details.name = info.Name();
                    }

                    if (info.ShortName().empty() == false) {
                        details.shortName = info.ShortName();
                    }

                    result = Core::ERROR_NONE;
                }

                return (result);
            }

            // Deprecated API (operating on low-energy devices only)
            Core::hresult Devices(IAddressIterator*& devices) const override
            {
                std::vector<string> localDevices;

                _parent.IterateDevices([&localDevices](DeviceImpl& device) -> bool {

                    if (device.Type() == IBluetooth::IDevice::ADDRESS_LE_PUBLIC) {
                        localDevices.push_back(device.RemoteId());
                    }

                    return (false);
                });

                if (localDevices.empty() == false) {
                    devices = Core::ServiceType<RPC::IteratorType<IAddressIterator>>::Create<IAddressIterator>(std::move(localDevices));
                }

                return (Core::ERROR_NONE);
            }
            uint32_t Device(const string& index, deviceinfo& info) const override
            {
                return (DeviceOp(index, devicetype::LOW_ENERGY, [&](const IDevice* device) -> Core::hresult {

                    info.address = index;
                    info.type = devicetype::LOW_ENERGY;

                    info.connected = device->IsConnected();
                    info.paired = device->IsPaired();

                    const string localName = device->Name();
                    if (localName.empty() == false) {
                        info.name = std::move(localName);
                    }

                    if (device->Class() != 0) {
                        info.cod = device->Class();
                    }

                    auto& uuids = (static_cast<const DeviceImpl*>(device))->UUIDs();
                    if (uuids.empty() == false) {
                        Core::JSON::ArrayType<Core::JSON::String> localUuids;

                        for (const Bluetooth::UUID& item : uuids) {
                            localUuids.Add() = item.ToString();
                        }

                        localUuids.ToString(info.services);
                    }

                    const IDevice::ILowEnergy* lowEnergy = QueryInterface<IDevice::ILowEnergy>();
                    if (lowEnergy != nullptr) {
                        if (lowEnergy->Appearance() != 0) {
                            info.appearance = lowEnergy->Appearance();
                        }

                        lowEnergy->Release();
                    }

                    return (Core::ERROR_NONE);
                }));
            }

        public:
            // IHandler overrides
            void OnDiscoverableStartedEventRegistration(const string& client, const PluginHost::JSONRPCSupportsEventStatus::Status status) override
            {
                // A new discoverable started event listener has registered, let them know if the discoverable state is already enabled
                if (status == PluginHost::JSONRPCSupportsEventStatus::Status::registered) {

                    bool limited = false;
                    bool connectable = true;

                    Exchange::IBluetooth::IClassic* classic = static_cast<Exchange::IBluetooth&>(_parent).QueryInterface<Exchange::IBluetooth::IClassic>();
                    if (classic != nullptr) {
                        if (classic->IsInquiryScanning(limited) == true) {
                            DiscoverableStarted(scantype::CLASSIC, (limited? scanmode::LIMITED : scanmode::GENERAL), connectable, client);
                        }

                        classic->Release();
                    }

                    Exchange::IBluetooth::ILowEnergy* le = static_cast<Exchange::IBluetooth&>(_parent).QueryInterface<Exchange::IBluetooth::ILowEnergy>();
                    if (le != nullptr) {
                        if (le->IsAdvertising(limited, connectable) == true) {
                            DiscoverableStarted(scantype::LOW_ENERGY, (limited? scanmode::LIMITED : scanmode::GENERAL), connectable, client);
                        }

                        le->Release();
                    }
                }
            }
            void OnScanStartedEventRegistration(const string& client, const PluginHost::JSONRPCSupportsEventStatus::Status status) override
            {
                // A new scan started event listener has registered, let them know if the scan is already in progress
                if (status == PluginHost::JSONRPCSupportsEventStatus::Status::registered) {

                    bool limited = false;

                    Exchange::IBluetooth::IClassic* classic = static_cast<Exchange::IBluetooth&>(_parent).QueryInterface<Exchange::IBluetooth::IClassic>();
                    if (classic != nullptr) {
                        if (classic->IsScanning(limited) == true) {
                            ScanStarted(scantype::CLASSIC, (limited? scanmode::LIMITED : scanmode::GENERAL), client);
                        }

                        classic->Release();
                    }

                    Exchange::IBluetooth::ILowEnergy* le = static_cast<Exchange::IBluetooth&>(_parent).QueryInterface<Exchange::IBluetooth::ILowEnergy>();
                    if (le != nullptr) {
                        if (le->IsScanning(limited) == true) {
                            ScanStarted(scantype::LOW_ENERGY, (limited? scanmode::LIMITED : scanmode::GENERAL), client);
                        }

                        le->Release();
                    }
                }
            }
            void OnDeviceStateChangedEventRegistration(const string& client, const PluginHost::JSONRPCSupportsEventStatus::Status status) override
            {
                // A new status change event listener has registered, let them know of current paired/connection status already
                if (status == PluginHost::JSONRPCSupportsEventStatus::Status::registered) {

                    _parent.IterateDevices([this, &client](DeviceImpl& device) -> bool {
                        if (device.IsPaired() == true) {
                            DeviceStateChanged(device.RemoteId(), DeviceType(device.Type()),
                                JBluetoothControl::devicestate::PAIRED, disconnectreason{}, client);
                        }
                        return (false);
                    });

                    _parent.IterateDevices([this, &client](DeviceImpl& device) -> bool {
                        if (device.IsConnected() == true) {
                            DeviceStateChanged(device.RemoteId(), DeviceType(device.Type()),
                                JBluetoothControl::devicestate::CONNECTED, disconnectreason{}, client);
                            // Can't break the loop here, dwo devices (classic and LE) can be connected at the same time.
                        }
                        return (false);
                    });
                }
            }

        public:
            // Event helpers
            void DiscoverableStarted(const scantype type, const scanmode mode, const bool connectable, const string& clientId = string{})
            {
                Core::OptionalType<bool> isconnectable;

                if (type == scantype::LOW_ENERGY) {
                    isconnectable = connectable;
                }

                if (clientId.empty() == true) {
                    Exchange::JSONRPC::JBluetoothControl::Event::DiscoverableStarted(_parent, type, mode, isconnectable);
                } else {
                    // ClientID is set, meaning this is status report during registration
                    Exchange::JSONRPC::JBluetoothControl::Event::DiscoverableStarted(_parent, type, mode, isconnectable,
                        [&clientId](const string& designator) -> bool {
                            return (designator == clientId);
                    });
                }
            }
            void DiscoverableComplete(const scantype type)
            {
                Exchange::JSONRPC::JBluetoothControl::Event::DiscoverableComplete(_parent, type);
            }
            void ScanStarted(const scantype type, const scanmode mode, const string& clientId = string{})
            {
                if (clientId.empty() == true) {
                    Exchange::JSONRPC::JBluetoothControl::Event::ScanStarted(_parent, type, mode);
                } else {
                    // ClientID is set, meaning this is status report during registration
                    Exchange::JSONRPC::JBluetoothControl::Event::ScanStarted(_parent, type, mode,
                        [&clientId](const string& designator) -> bool {
                            return (designator == clientId);
                    });
                }
            }
            void ScanComplete(const scantype type)
            {
                Exchange::JSONRPC::JBluetoothControl::Event::ScanComplete(_parent, type);
            }
            void DeviceStateChanged(const string& address, const devicetype type, const devicestate state,
                const disconnectreason reason, const string& clientId = string{})
            {
                Core::OptionalType<disconnectreason> disconnReason;

                if (state == devicestate::DISCONNECTED) {
                    disconnReason = reason;
                }

                auto AddressMatch = [](const string& designator, const string& address) -> bool {
                    // Also allow subscribing for all device changes when the beginning of the designator does not match
                    // a Bluetooth address pattern (this also ensures backwards compatibility).
                    const size_t delim = designator.find('.');
                    return (((delim != string::npos) && (delim != 17))
                                || (designator.compare(0, delim, address) == 0)
                                || (designator.size() < 17)
                                || (Bluetooth::Address(designator.c_str()).IsValid() == false));
                };

                if (clientId.empty() == true) {
                    // Live change of status, only pick up the events for registered devices
                    Exchange::JSONRPC::JBluetoothControl::Event::DeviceStateChanged(_parent, address, type, state, disconnReason,
                        [&address, &AddressMatch](const string& designator) {
                            return (AddressMatch(designator, address));
                    });
                } else {
                    // ClientID is set, meaning this is status report during registration.
                    Exchange::JSONRPC::JBluetoothControl::Event::DeviceStateChanged(_parent, address, type, state, disconnReason,
                        [&clientId, &address, &AddressMatch](const string& designator) {
                            return ((designator == clientId) && (AddressMatch(designator, address) == true));
                    });
                }
            }
            void PINCodeRequest(const string& address, const devicetype type)
            {
                Exchange::JSONRPC::JBluetoothControl::Event::PINCodeRequest(_parent, address, type);
            }
            void PasskeyRequest(const string& address, const devicetype type)
            {
                Exchange::JSONRPC::JBluetoothControl::Event::PasskeyRequest(_parent, address, type);
            }
            void PasskeyConfirmRequest(const string& address, const devicetype type, const uint32_t secret)
            {
                Core::OptionalType<uint32_t> passkey;

                if (secret != 0) {
                    passkey = secret;
                }

                Exchange::JSONRPC::JBluetoothControl::Event::PasskeyConfirmRequest(_parent, address, type, passkey);
            }

        private:
            BluetoothControl& _parent;
        }; // class JSONRPCImplementation

    public:
        BluetoothControl(const BluetoothControl&) = delete;
        BluetoothControl& operator=(const BluetoothControl&) = delete;
        BluetoothControl()
            : _skipURL(0)
            , _adminLock()
            , _config()
            , _persistentStoragePath()
            , _application()
            , _jsonrpcImplementation(*this)
            , _classicAdapter(nullptr)
            , _lowEnergyAdapter(nullptr)
            , _adapters()
            , _btInterface(0)
            , _btAddress()
            , _devices()
            , _observers()
            , _uuids()
        {
        }
        ~BluetoothControl() override = default;

    public:
        BEGIN_INTERFACE_MAP(BluetoothControl)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_ENTRY(Exchange::IBluetooth)
        INTERFACE_AGGREGATE(Exchange::IBluetooth::IClassic, _classicAdapter)
        INTERFACE_AGGREGATE(Exchange::IBluetooth::ILowEnergy, _lowEnergyAdapter)
        END_INTERFACE_MAP

    public:
        // PluginHost::IPlugin overrides
        // -------------------------------------------------------------------------------------------------------

        // First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
        // information and services for this particular plugin. The Service object contains configuration information that
        // can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
        // If there is an error, return a string describing the issue why the initialisation failed.
        // The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
        // The lifetime of the Service object is guaranteed till the deinitialize method is called.
        const string Initialize(PluginHost::IShell* service) override;

        // The plugin is unloaded from the webbridge. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After this call, the lifetime of the Service object ends.
        void Deinitialize(PluginHost::IShell* service) override;

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetaData plugin to publish this information to the outside world.
        string Information() const override;

        // PluginHost::IPlugin::INotification overrides
        // -------------------------------------------------------------------------------------------------------
        void Activated(const string& callsign, PluginHost::IShell*) override;
        void Deactivated(const string& callsign, PluginHost::IShell*) override;
        void Unavailable(const string&, PluginHost::IShell*) override { }

        // PluginHost::IWeb overrides
        // -------------------------------------------------------------------------------------------------------
        void Inbound(Web::Request& request) override;
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

        // Exchange::IBluetooth overrides
        // -------------------------------------------------------------------------------------------------------
        uint32_t Register(IBluetooth::INotification* notification) override;
        uint32_t Unregister(IBluetooth::INotification* notification) override;
        /* DEPRECATED */ bool IsScanning() const override;
        /* DEPRECATED */ uint32_t Scan(const bool lowEnergy, const uint16_t duration) override;
        /* DEPRECATED */ uint32_t StopScanning() override;
        IBluetooth::IDevice* Device(const string& address, const IBluetooth::IDevice::type type) override;
        IBluetooth::IDevice::IIterator* Devices() override;
        uint32_t ForgetDevice(const string& address, const IBluetooth::IDevice::type type) override;

    public:
        uint16_t InterfaceID() const
        {
            return (_btInterface);
        }
        ControlSocket& Connector()
        {
            return (_application);
        }
        const ControlSocket& Connector() const
        {
            return (_application);
        }
        void Update()
        {
            _adminLock.Lock();
            for (auto observer : _observers) {
PUSH_WARNING(DISABLE_WARNING_DEPRECATED_USE)
                // Internal call to a deprecated callback method
                observer->Update();
POP_WARNING()
            }
            _adminLock.Unlock();
        }
        void Update(IBluetooth::IDevice* device)
        {
            ASSERT(device != nullptr);
            _adminLock.Lock();
            for (auto observer : _observers) {
                observer->Update(device);
            }
            _adminLock.Unlock();
        }

    private:
        void PairedChange(const DeviceImpl* device)
        {
            if (device->IsPaired() == true) {
                SaveDevice(device);
            } else {
                EraseDevice(device);
            }
        }
        void IterateDevices(std::function<bool(DeviceImpl&)> fn)
        {
            _adminLock.Lock();
            for (auto device : _devices) {
                if (fn(*device) == true) {
                    break;
                }
            }
            _adminLock.Unlock();
        }
        void IterateAdapters(std::function<bool(uint16_t&)> fn)
        {
            _adminLock.Lock();
            for (auto adapter : _adapters) {
                if (fn(adapter) == true) {
                    break;
                }
            }
            _adminLock.Unlock();
        }

    private:
        DeviceImpl* Find(const uint16_t handle);
        const DeviceImpl* Find(const uint16_t handle) const;

        DeviceImpl* Find(const Bluetooth::Address& address, const bool lowEnergy);
        const DeviceImpl* Find(const Bluetooth::Address& address, const bool lowEnergy) const;

        template<typename DEVICE, typename std::enable_if<std::is_base_of<DeviceImpl, DEVICE>::value, int>::type = 0>
        DEVICE* Find(const Bluetooth::Address& address)
        {
            return (reinterpret_cast<DEVICE*>(Find(address, std::is_same<DEVICE, DeviceLowEnergy>::value)));
        }

        template<typename DEVICE, typename std::enable_if<std::is_base_of<DeviceImpl, DEVICE>::value, int>::type = 0>
        DEVICE* Find(const uint16_t& handle)
        {
            return (reinterpret_cast<DEVICE*>(Find(handle)));
        }

    private:
        uint16_t RemoveDevices(std::function<bool(DeviceImpl*)> filter);
        DeviceImpl* Discovered(const bool lowEnergy, const Bluetooth::Address& address);
        void Notification(const uint8_t subEvent, const uint16_t length, const uint8_t* dataFrame);
        void Capabilities(const Bluetooth::Address& device, const uint8_t capability, const uint8_t authentication, const uint8_t oobData);
        void LoadController(const string& pathName, Data& data) const;
        void SaveController(const string& pathName, const Data& data);

        uint32_t LoadDevices(const string& devicePath, Bluetooth::ManagementSocket& administrator);
        uint32_t LoadDevice(const string&, Bluetooth::LinkKeys&, Bluetooth::LongTermKeys&, Bluetooth::IdentityKeys&);
        uint32_t EraseDevice(const DeviceImpl* device);
        uint32_t SaveDevice(const DeviceImpl* device) const;

    private:
        const Config& Configuration() const {
            return _config;
        }
        JSONRPCImplementation& JSONRPC()
        {
            return (_jsonrpcImplementation);
        }

    private:
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& request);

    private:
        uint8_t _skipURL;
        Core::CriticalSection _adminLock;
        Config _config;
        string _persistentStoragePath;
        ControlSocket _application;
        JSONRPCImplementation _jsonrpcImplementation;
        ClassicImpl* _classicAdapter;
        LowEnergyImpl* _lowEnergyAdapter;
        std::list<uint16_t> _adapters;
        uint16_t _btInterface;
        Bluetooth::Address _btAddress;
        std::list<DeviceImpl*> _devices;
        std::list<IBluetooth::INotification*> _observers;
        std::list<std::tuple<string /* callsign */, Bluetooth::UUID, uint8_t /* service-bit */>> _uuids;
    }; // class BluetoothControl


} //namespace Plugin

}
