/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
#include <interfaces/json/JsonData_BluetoothControl.h>

#include "Tracing.h"

namespace WPEFramework {

namespace Plugin {

using namespace Tracing;

class BluetoothControl : public PluginHost::IPlugin
                       , public PluginHost::IWeb
                       , public PluginHost::JSONRPC
                       , public Exchange::IBluetooth {

    private:
        class DecoupledJob : private Core::ThreadPool::JobType<DecoupledJob&> {
        public:
            using Job = std::function<void()>;

            DecoupledJob(const DecoupledJob&) = delete;
            DecoupledJob& operator=(const DecoupledJob&) = delete;

            DecoupledJob()
                : Core::ThreadPool::JobType<DecoupledJob&>(*this)
                , _lock()
                , _job(nullptr)
            {
            }
            ~DecoupledJob() = default;

        public:
            void Submit(const Job& job, const uint32_t defer = 0)
            {
                Core::ProxyType<Core::IDispatch> handler(Aquire());

               _lock.Lock();
                if (handler.IsValid() == true) {

                    _job = job;

                    if (defer == 0) {
                        Core::WorkerPool::Instance().Submit(handler);
                    } else {
                        Core::WorkerPool::Instance().Schedule(Core::Time::Now().Add(defer), handler);
                    }
                } else {
                    TRACE(Trace::Information, (_T("Job in progress, skipping request")));
                }

               _lock.Unlock();
            }
            void Revoke()
            {
                _lock.Lock();
                Core::WorkerPool::Instance().Revoke(Reset());
                _job = nullptr;
                _lock.Unlock();
            }

        private:
            friend class Core::ThreadPool::JobType<DecoupledJob&>;
            void Dispatch()
            {
                _lock.Lock();
                ASSERT(_job != nullptr);
                Job job = _job;
                _job = nullptr;
                _lock.Unlock();
                job();
            }

        private:
            Core::CriticalSection _lock;
            Job _job;
        }; // class DecoupledJob

        static uint32_t UnpackDeviceClass(const uint8_t buffer[3])
        {
            ASSERT(buffer != nullptr);
            return ((buffer[2] << 16) | (buffer[1] << 8) | (buffer[0]));
        }

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
                    default:
                        BT_TRACE(ManagementFlow, header, data);
                        break;
                    }
                }
                void Update(const mgmt_ev_cmd_complete& info)
                {
                    BT_TRACE(ManagementFlow, info);
                    if (info.opcode == MGMT_OP_PAIR_DEVICE) {
                        const mgmt_rp_pair_device* payload = reinterpret_cast<const mgmt_rp_pair_device*>(info.data);
                        _parent.PairingComplete(payload->addr.bdaddr, static_cast<const Bluetooth::Address::type>(payload->addr.type), info.status);
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
                    _parent.RequestPINCode(info.addr.bdaddr, static_cast<const Bluetooth::Address::type>(info.addr.type));
                }
                void Update(const mgmt_ev_user_passkey_request& info)
                {
                    BT_TRACE(ManagementFlow, info);
                    _parent.RequestPasskey(info.addr.bdaddr, static_cast<const Bluetooth::Address::type>(info.addr.type));
                }
                void Update(const mgmt_ev_user_confirm_request& info)
                {
                    BT_TRACE(ManagementFlow, info);
                    _parent.RequestPasskeyConfirm(info.addr.bdaddr, static_cast<const Bluetooth::Address::type>(info.addr.type), btohl(info.value));
                }
                void Update(const mgmt_ev_new_link_key& info)
                {
                    BT_TRACE(ManagementFlow, info);
                    if (info.store_hint != 0) {
                        if (_parent.SecurityKey(info.key.addr.bdaddr, static_cast<const Bluetooth::Address::type>(info.key.addr.type),
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
                        if (_parent.SecurityKey(info.key.addr.bdaddr, static_cast<const Bluetooth::Address::type>(info.key.addr.type),
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
                        if (_parent.SecurityKey(info.key.addr.bdaddr, static_cast<const Bluetooth::Address::type>(info.key.addr.type),
                                                Bluetooth::LongTermKey(info.key.addr.bdaddr, info.key.addr.type, info.key.type,
                                                                       info.key.master, info.key.enc_size, btohs(info.key.ediv),
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
            ControlSocket(const ControlSocket&) = delete;
            ControlSocket& operator=(const ControlSocket&) = delete;
            ControlSocket()
                : Bluetooth::HCISocket()
                , _parent(nullptr)
                , _administrator(*this)
            {
            }
            ~ControlSocket() = default;

        public:
            Bluetooth::ManagementSocket& Control()
            {
                return(_administrator);
            }
            Bluetooth::ManagementSocket::Info Settings() const {
                return(_administrator.Settings());
            }
            BluetoothControl* Application()
            {
                return (_parent);
            }
            uint32_t Pair(const Bluetooth::Address& remote, const Bluetooth::Address::type type, const Bluetooth::ManagementSocket::capabilities caps)
            {
                return(_administrator.Pair(remote, type, caps));
            }
            uint32_t PairAbort(const Bluetooth::Address& remote, const Bluetooth::Address::type type)
            {
                return(_administrator.PairAbort(remote, type));
            }
            uint32_t Unpair(const Bluetooth::Address& remote, const Bluetooth::Address::type type)
            {
                return(_administrator.Unpair(remote, type));
            }
            void Scan(const uint16_t scanTime, const uint32_t type, const uint8_t flags)
            {
                if (IsOpen() == true) {
                    _scanJob.Submit([this, scanTime, type, flags]() {
                        TRACE(ControlFlow, (_T("Start BT classic scan: %s"), Core::Time::Now().ToRFC1123().c_str()));
                        Bluetooth::HCISocket::Scan(scanTime, type, flags);
                        ScanComplete();
                    });
                }
            }
            void Scan(const uint16_t scanTime, const bool limited, const bool passive)
            {
                if (IsOpen() == true) {
                    _scanJob.Submit([this, scanTime, limited, passive]() {
                        TRACE(ControlFlow, (_T("Start BT LowEnergy scan: %s"), Core::Time::Now().ToRFC1123().c_str()));
                        Bluetooth::HCISocket::Scan(scanTime, limited, passive);
                        ScanComplete();
                    });
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
                _scanJob.Revoke();
                Bluetooth::ManagementSocket::Down(_administrator.DeviceId());
                _administrator.DeviceId(HCI_DEV_NONE);
                _parent = nullptr;
                return (result);
            }
            void ScanComplete()
            {
                TRACE(ControlFlow, (_T("Scan completed: %s"), Core::Time::Now().ToRFC1123().c_str()));
                Application()->event_scancomplete();
            }
            void PairingComplete(const Bluetooth::Address& remote, const Bluetooth::Address::type type, const uint8_t status)
            {
                UpdateDevice<DeviceImpl>(remote, (type != Bluetooth::Address::BREDR_ADDRESS), [&](DeviceImpl* device) {
                    device->PairingComplete(status);
                });
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

        public:
            void Update(const le_advertising_info& info) override
            {
                BT_TRACE(ControlFlow, info);
                if ((Application() != nullptr) && (info.bdaddr_type == 0 /* public */)) {
                    Bluetooth::EIR eir(info.data, info.length);
                    DeviceImpl* device = nullptr;

                    const uint8_t SCAN_RESPONSE = 4;
                    const uint8_t UNDIRECTED_CONNECTABLE_ADVERTISMENT = 0;

                    if ((info.evt_type == SCAN_RESPONSE) || (info.evt_type == UNDIRECTED_CONNECTABLE_ADVERTISMENT)) {
                        device = Application()->Find(Bluetooth::Address(info.bdaddr));

                        if (device == nullptr) {
                            device = Application()->Discovered(true, info.bdaddr);
                        }

                        if (device != nullptr) {
                            if ((device->Update(eir) == false) && (info.evt_type == UNDIRECTED_CONNECTABLE_ADVERTISMENT)) {
                                device->UpdateListener();
                            }
                        }
                    }
                }
            }
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
                    case EVT_EXTENDED_INQUIRY_RESULT: {
                        uint8_t count = data[0];
                        while (count) {
                            Update(*reinterpret_cast<const extended_inquiry_info*>(data + 1 /* count */ + (count * sizeof(extended_inquiry_info))));
                            count--;
                        }
                        break;
                    }
                    case EVT_INQUIRY_RESULT_WITH_RSSI: {
                        uint8_t count = data[0];
                        while (count) {
                            Update(*reinterpret_cast<const inquiry_info_with_rssi*>(data + 1 /* count */ + (count * sizeof(inquiry_info_with_rssi))));
                            count--;
                        }
                        break;
                    }
                    default:
                        BT_TRACE(ControlFlow, header, data);
                        break;
                }
            }
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
            void Update(const extended_inquiry_info& info)
            {
                BT_TRACE(ControlFlow, info);
                UpdateDevice<DeviceImpl>(info.bdaddr, [&](DeviceImpl* device) {
                    device->Class(UnpackDeviceClass(info.dev_class));
                });
            }
            void Update(const inquiry_info_with_rssi& info)
            {
                BT_TRACE(ControlFlow, info);
                UpdateDevice<DeviceImpl>(info.bdaddr, [&](DeviceImpl* device) {
                    device->Class(UnpackDeviceClass(info.dev_class));
                });
            }
            void Update(const evt_conn_request& info)
            {
                BT_TRACE(ControlFlow, info);
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
            DecoupledJob _scanJob;
            ManagementSocket _administrator;
        }; // class ControlSocket

        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config()
                : Core::JSON::Container()
                , Interface(0)
                , Name(_T("Thunder BT Control"))
                , Class(0)
                , AutoPasskeyConfirm(false)
                , PersistMAC(false)
            {
                Add(_T("interface"), &Interface);
                Add(_T("name"), &Name);
                Add(_T("class"), &Class);
                Add(_T("autopasskeyconfirm"), &AutoPasskeyConfirm);
                Add(_T("persistmac"), &PersistMAC);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt16 Interface;
            Core::JSON::String Name;
            Core::JSON::HexUInt32 Class;
            Core::JSON::Boolean AutoPasskeyConfirm;
            Core::JSON::Boolean PersistMAC;
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
            ~Data()
            {
            }

        public:
            Core::JSON::String MAC;
        }; // class Data

    public:
        class DeviceImpl : public Exchange::IBluetooth::IDevice {
        private:
            static constexpr uint16_t ACTION_MASK = 0x00FF;

        public:
            static constexpr uint32_t MAX_ACTION_TIMEOUT = 2000; /* 2S to setup a connection ? */

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
                ~Config()
                {
                }

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
                    , Bonded(false)
                    , Reason(0)
                {
                    Add(_T("local"), &LocalId);
                    Add(_T("remote"), &RemoteId);
                    Add(_T("name"), &Name);
                    Add(_T("le"), &LowEnergy);
                    Add(_T("connected"), &Connected);
                    Add(_T("bonded"), &Bonded);
                    Add(_T("reason"), &Reason);
                }
                Data(const Data& copy)
                    : Core::JSON::Container()
                    , LocalId()
                    , RemoteId()
                    , Name()
                    , LowEnergy(false)
                    , Connected(false)
                    , Bonded(false)
                    , Reason(0)
                {
                    Add(_T("local"), &LocalId);
                    Add(_T("remote"), &RemoteId);
                    Add(_T("name"), &Name);
                    Add(_T("le"), &LowEnergy);
                    Add(_T("connected"), &Connected);
                    Add(_T("bonded"), &Bonded);
                    Add(_T("reason"), &Reason);
                    LocalId = copy.LocalId;
                    RemoteId = copy.RemoteId;
                    Name = copy.Name;
                    LowEnergy = copy.LowEnergy;
                    Connected = copy.Connected;
                    Bonded = copy.Bonded;
                    Reason = copy.Reason;
                }
                ~Data()
                {
                }

            public:
                Data& Set(const DeviceImpl* source)
                {
                    if (source != nullptr) {
                        LocalId = source->LocalId();
                        RemoteId = source->RemoteId();
                        Name = source->Name();
                        LowEnergy = source->LowEnergy();
                        Connected = source->IsConnected();
                        Bonded = source->IsBonded();
                    } else {
                        LocalId.Clear();
                        RemoteId.Clear();
                        Name.Clear();
                        LowEnergy.Clear();
                        Bonded.Clear();
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
                Core::JSON::Boolean Bonded;
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
                , _oob_data(~0)
                , _interval(0)
                , _latency(0)
                , _timeout(0)
                , _autoConnectionSubmitted(false)
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
            }

        public:
            // SecurityKeys getters
            virtual void SecurityKey(Bluetooth::LinkKeys& key) const = 0;
            virtual void SecurityKey(Bluetooth::LongTermKeys& key) const = 0;
            virtual const Bluetooth::IdentityKey& IdentityKey() const = 0;

        public:
            // IDevice overrides
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
            Bluetooth::Address Address() const
            {
                return (_remote);
            }
            Bluetooth::Address::type AddressType() const
            {
                return (_type);
            }
            string Name() const override
            {
                if (_name.empty() == true) {
                    return "[Unknown]";
                } else {
                    return (_name);
                }
            }
            uint32_t Class() const override
            {
                return (_class);
            }
            const std::list<Bluetooth::UUID>& UUIDs() const
            {
                return (_uuids);
            }
            bool IsConnected() const override
            {
                return (_handle != static_cast<uint16_t>(~0));
            }
            uint32_t Disconnect() override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(DISCONNECTING) == Core::ERROR_NONE) {
                    Bluetooth::HCISocket::Command::Disconnect disconnect;

                    disconnect->handle = htobs(ConnectionId());
                    disconnect->reason = HCI_CONNECTION_TERMINATED;

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
                            ClearState(DISCONNECTING);
                        }
                    } else {
                        TRACE(ControlFlow, (_T("Failed to disconnect [%d]"), result));
                        ClearState(DISCONNECTING);
                    }
                } else {
                    TRACE(Trace::Information, (_T("Device is currently busy")));
                }

                return (result);
            }
            uint32_t Pair(const IBluetooth::pairingcapabilities capabilities, const uint16_t timeout) override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(PAIRING) == Core::ERROR_NONE) {
                    result = _parent->Connector().Pair(Address(), AddressType(), static_cast<Bluetooth::ManagementSocket::capabilities>(capabilities));
                    if (result == Core::ERROR_INPROGRESS) {
                        _parent->event_devicestatechange(Address().ToString(), JsonData::BluetoothControl::DevicestatechangeParamsData::DevicestateType::PAIRING);
                        TRACE(Trace::Information, (_T("Pairing of device %s in progress..."), Address().ToString().c_str()));
                        _abortPairingJob.Submit([this](){
                            TRACE(Trace::Information, (_T("Timeout! Aborting pairing!"), Address().ToString().c_str()));
                            AbortPairing();
                        }, 1000L * timeout);
                        result = Core::ERROR_NONE;
                    } else {
                        if (result == Core::ERROR_ALREADY_CONNECTED) {
                            TRACE(Trace::Information, (_T("Device is already paired")));
                        } else if (result != Core::ERROR_NONE) {
                            TRACE(Trace::Error, (_T("Failed to pair device %s [%d]"), Address().ToString().c_str(), result));
                        }

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
                        TRACE(Trace::Information, (_T("Aborting pairing of device %s in progress..."), Address().ToString().c_str()));
                    } else if (result != Core::ERROR_NONE) {
                        ClearState(PAIRING); // Hope for the best anyway...
                        TRACE(Trace::Error, (_T("Failed to abort pairing [%d]"), result));
                    }
                } else {
                    ClearState(PAIRING);
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
                        TRACE(Trace::Error, (_T("Failed to unpair device %s [%d]"), Address().ToString().c_str(), result));
                    } else {
                        TRACE(Trace::Information, (_T("Device unpaired")));
                    }

                    ClearState(UNPAIRING);
                    _autoConnectionSubmitted = false;
                    UpdateListener();
                } else {
                    TRACE(Trace::Information, (_T("Device is currently busy")));
                }

                return (result);
            }
            void Passkey(const uint32_t passkey) override
            {
                TRACE(Trace::Information, (_T("Passkey reply: %06d"), passkey));
                _userReplyJob.Submit([this, passkey]() {
                    _parent->Connector().Control().UserPasskeyReply(Address(), AddressType(), passkey);
                });
            }
            void ConfirmPasskey(const bool confirm) override
            {
                TRACE(Trace::Information, (_T("Passkey confirmation reply: %s"), confirm? "YES" : "NO"));
                _userReplyJob.Submit([this, confirm]() {
                    _parent->Connector().Control().UserPasskeyConfirmReply(Address(), AddressType(), confirm);
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
                }
                else if (_callback == nullptr) {
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
                if ((IsConnected() == false) && ((_state & ACTION_MASK) == 0)) {
                    _state.SetState(static_cast<state>(0));
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
            inline bool operator==(const std::pair<const Bluetooth::Address&, const bool>& rhs) const
            {
                return ((_remote == rhs.first) && (LowEnergy() == rhs.second));
            }
            inline bool operator!=(const std::pair<const Bluetooth::Address&, const bool>& rhs) const
            {
                return (!operator==(rhs));
            }
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
            void Capabilities(const uint8_t capability, const uint8_t authentication, const uint8_t oob_data)
            {
                _capabilities = capability;
                _authentication = authentication;
                _oob_data = oob_data;
            }
            uint16_t ConnectionId() const
            {
                return (_handle);
            }

        protected:
            friend class ControlSocket;

            inline ControlSocket& Connector() {
                return (_parent->Connector());
            }
            void RequestPasskey()
            {
                TRACE(Trace::Information, (_T("Pairing with device %s; requesting passkey..."), Address().ToString().c_str()));
                _userRequestJob.Submit([this](){
                    Callback<IBluetooth::IDevice::ISecurityCallback>(SecurityCallback(), [&](IBluetooth::IDevice::ISecurityCallback* cb) {
                        cb->PasskeyRequest();
                    });
                });
            }
            void RequestPasskeyConfirm(const uint32_t passkey)
            {
                TRACE(Trace::Information, (_T("Pairing with device %s; confirm passkey %06d? (YES/NO)"), Address().ToString().c_str(), passkey));

                // Request passkey confirmation from client or, if auto confirm is enabled, reply already.
                if (_parent->AutoConfirmPasskey() == true) {
                    TRACE(Trace::Information, (_T("Auto-confirm enabled, accepting the passkey!")));
                    ConfirmPasskey(true);
                } else {
                    TRACE(Trace::Information, (_T("Waiting for user confirmation of the passkey...")));
                    _userRequestJob.Submit([this, passkey]() {
                        Callback<IBluetooth::IDevice::ISecurityCallback>(SecurityCallback(), [&](IBluetooth::IDevice::ISecurityCallback* cb) {
                            cb->PasskeyConfirmRequest(passkey);
                        });
                    });
                }
            }
            void PairingComplete(const uint8_t status = 0)
            {
                _abortPairingJob.Revoke();
                if (status == 0) {
                    // Pairing is considered successful only if the appropriate keys are exchanged.
                    if (!IsBonded()) {
                        TRACE(Trace::Information, (_T("Device %s pairing successful"), _remote.ToString().c_str()));
                    }
                } else {
                    if (status == MGMT_STATUS_CANCELLED) {
                        TRACE(Trace::Information, (_T("Device %s pairing aborted!"), _remote.ToString().c_str()));
                    } else {
                        TRACE(Trace::Error, (_T("Device %s pairing failed! [0x%02x]"), _remote.ToString().c_str(), status));
                    }

                    BondedChange();
                }
                ClearState(PAIRING);
            }
            void BondedChange()
            {
                _parent->BondedChange(this);
                UpdateListener();
                _parent->event_devicestatechange(Address().ToString(), IsBonded()? JsonData::BluetoothControl::DevicestatechangeParamsData::DevicestateType::PAIRED
                                                                                 : JsonData::BluetoothControl::DevicestatechangeParamsData::DevicestateType::UNPAIRED);
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
                    TRACE(DeviceFlow, (_T("The connection handle is changed during the runtime, from: %d to: %d"), _handle, handle));
                    _handle = handle;
                }

                _state.Unlock();

                if (updated == true) {
                    UpdateListener();
                    _parent->event_devicestatechange(Address().ToString(), JsonData::BluetoothControl::DevicestatechangeParamsData::DevicestateType::CONNECTED);
                }
            }
            void Disconnection(const uint8_t reason)
            {
                TRACE(DeviceFlow, (_T("Disconnected connection %d, reason: %d"), _handle, reason));

                _state.Lock();

                ClearState(CONNECTING);
                ClearState(DISCONNECTING);
                _handle = ~0;

                if (IsBonded() && (_autoConnectionSubmitted == false)) {
                    // Setting autoconnection can only be done when the device is disconnected.
                    // The whitelist is not cleared on subsequent connections/disconnections, so this only needs to be done once.
                    // Kernel will disable autoconnection on it's own when unbinding, no need to call RemoveDevice then.
                    _autoConnectJob.Submit([this](){
                        AutoConnect(true);
                    });
                }

                _state.Unlock();

                JsonData::BluetoothControl::DevicestatechangeParamsData::DisconnectreasonType disconnReason;
                if (reason == HCI_CONNECTION_TIMEOUT) {
                    disconnReason = JsonData::BluetoothControl::DevicestatechangeParamsData::DisconnectreasonType::CONNECTIONTIMEOUT;
                } else if (reason == HCI_AUTHENTICATION_FAILURE) {
                    disconnReason = JsonData::BluetoothControl::DevicestatechangeParamsData::DisconnectreasonType::AUTHENTICATIONFAILURE;
                } else if (reason == HCI_OE_USER_ENDED_CONNECTION) {
                    disconnReason = JsonData::BluetoothControl::DevicestatechangeParamsData::DisconnectreasonType::TERMINATEDBYREMOTE;
                } else if (reason == HCI_OE_LOW_RESOURCES) {
                    disconnReason = JsonData::BluetoothControl::DevicestatechangeParamsData::DisconnectreasonType::REMOTELOWONRESOURCES;
                } else if (reason == HCI_OE_POWER_OFF) {
                    disconnReason = JsonData::BluetoothControl::DevicestatechangeParamsData::DisconnectreasonType::REMOTEPOWEREDOFF;
                } else {
                    disconnReason = JsonData::BluetoothControl::DevicestatechangeParamsData::DisconnectreasonType::TERMINATEDBYHOST;
                }

                UpdateListener();
                _parent->event_devicestatechange(Address().ToString(), JsonData::BluetoothControl::DevicestatechangeParamsData::DevicestateType::DISCONNECTED, disconnReason);
            }
            bool Class(const uint32_t classId, bool notifyListener = true)
            {
                bool updated = false;

                _state.Lock();

                if ((classId != 0) && (classId != _class)) {
                    _class = classId;
                    TRACE(DeviceFlow, (_T("Device class updated to: '%d'"), _class));
                    updated = true;
                    if (notifyListener == true) {
                        UpdateListener();
                    }
                }

                _state.Unlock();

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
                    if (notifyListener == true) {
                        UpdateListener();
                    }
                }

                _state.Unlock();

                return (updated);
            }
            bool Update(const Bluetooth::EIR& eir)
            {
                bool updated = false;

                updated |= Class(eir.Class(), false);
                updated |= Name(eir.CompleteName(), false);

                _state.Lock();

                if (eir.UUIDs() != _uuids) {
                    _uuids.clear();

                    TRACE(DeviceFlow, (_T("Supported UUIDs:")));
                    for (auto uuid : eir.UUIDs()) {
                        TRACE(DeviceFlow, (_T(" - %s"), uuid.ToString().c_str()));
                        _uuids.emplace_back(uuid);
                    }

                    updated = true;
                }

                _state.Unlock();

                if (updated == true) {
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
                if (enable == true) {
                    uint32_t result = _parent->Connector().Control().AddDevice(AddressType(), Address(), AutoConnectMode());
                    if (result != Core::ERROR_NONE) {
                        TRACE(DeviceFlow, (_T("Could not add device %s [%d]"), Address().ToString().c_str(), result));
                    } else {
                        TRACE(DeviceFlow, (_T("Enabled autoconnect of device %s"), Address().ToString().c_str()));
                        _autoConnectionSubmitted = true;
                    }
                } else {
                    uint32_t result = _parent->Connector().Control().RemoveDevice(AddressType(), Address());
                    if (result != Core::ERROR_NONE) {
                        TRACE(DeviceFlow, (_T("Could not remove device [%d]"), Address().ToString().c_str(), result));
                    }
                    else {
                        TRACE(DeviceFlow, (_T("Disabled autoconnect of device %s"), Address().ToString().c_str()));
                    }

                   _autoConnectionSubmitted = false;
                }
            }

        private:
            // Depending on the device type, different keys are exchanged. Leave it up to
            // the specific implementation to implement the needed keys!
            virtual uint32_t SecurityKey(const Bluetooth::LinkKey& key) = 0;
            virtual uint32_t SecurityKey(const Bluetooth::LongTermKey& key) = 0;
            virtual uint32_t SecurityKey(const Bluetooth::IdentityKey& key) = 0;
            virtual void PurgeSecurityKeys() = 0;

            virtual Bluetooth::ManagementSocket::autoconnmode AutoConnectMode() const = 0;

            void UpdateListener()
            {
                _deviceUpdateJob.Submit([this](){
                    Callback<IBluetooth::IDevice::ICallback>(Callback(), [](IBluetooth::IDevice::ICallback* cb) {
                        cb->Updated();
                    });
                });
            }

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
            uint8_t _oob_data;
            uint16_t _interval;
            uint16_t _latency;
            uint16_t _timeout;
            bool _autoConnectionSubmitted;
            IBluetooth::IDevice::ICallback* _callback;
            IBluetooth::IDevice::ISecurityCallback* _securityCallback;
            DecoupledJob _deviceUpdateJob;
            DecoupledJob _autoConnectJob;
            DecoupledJob _abortPairingJob;
            DecoupledJob _userRequestJob;
            DecoupledJob _userReplyJob;
        }; // class DeviceImpl

    public:
        class DeviceRegular : public DeviceImpl, public Exchange::IBluetooth::IClassic {
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

                if (IsBonded() == true) {
                    AutoConnect(true);
                }
            }
            ~DeviceRegular() = default;

        public:
            // IDevice overrides
            bool IsBonded() const override
            {
                return (_linkKeys.Entries() > 0);
            }
            uint32_t Connect() override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(CONNECTING) == Core::ERROR_NONE) {
                    if (IsConnected() == false) {
                        if (IsBonded() == true) {
                            Bluetooth::HCISocket::Command::Connect connect;

                            connect.Clear();
                            connect->bdaddr = *(Locator().Data());
                            connect->pkt_type = htobs(HCI_DM1 | HCI_DM3 | HCI_DM5 | HCI_DH1 | HCI_DH3 | HCI_DH5);
                            connect->pscan_rep_mode = 0x02;
                            connect->clock_offset = 0x0000;
                            connect->role_switch = 0x01;

                            result = _parent->Connector().Exchange(MAX_ACTION_TIMEOUT, connect, connect);
                            if (result == Core::ERROR_NONE) {
                                if (connect.Result() == 0) {
                                    Connection(btohs(connect.Response().handle));
                                } else {
                                    result = Core::ERROR_ASYNC_FAILED;
                                    TRACE(ControlFlow, (_T("Connect command failed [%d]"), connect.Result()));
                                }
                            } else {
                                TRACE(Trace::Error, (_T("Failed to connect [%d]"), result));
                            }
                        } else {
                            result = Core::ERROR_ILLEGAL_STATE;
                            TRACE(Trace::Information, (_T("Device not paired!")));
                        }
                    } else {
                        result = Core::ERROR_ALREADY_CONNECTED;
                        TRACE(Trace::Information, (_T("Device already connected!")));
                    }
                    ClearState(CONNECTING);
                } else {
                    result = Core::ERROR_INPROGRESS;
                    TRACE(Trace::Information, (_T("Device is currently busy")));
                }

                return (result);
            }

        public:
            // IClassic overrides
            void PINCode(const string& pinCode) override
            {
                _userReplyJob.Submit([this, pinCode](){
                    _parent->Connector().Control().UserPINCodeReply(Address(), AddressType(), pinCode);
                });
            }
            uint32_t Callback(IBluetooth::IClassic::ISecurityCallback* callback) override
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
                TRACE(Trace::Information, (_T("Pairing with legacy device %s; requesting PIN code..."), Address().ToString().c_str()));
                _userRequestJob.Submit([this]() {
                    Callback([&](IBluetooth::IClassic::ISecurityCallback* cb) {
                        cb->PINCodeRequest();
                    });
                });
            }

        public:
            BEGIN_INTERFACE_MAP(DeviceRegular)
            INTERFACE_ENTRY(Exchange::IBluetooth::IClassic)
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

                _parent->Connector().Execute<Bluetooth::HCISocket::Command::RemoteName>(MAX_ACTION_TIMEOUT, cmd, [&](Bluetooth::HCISocket::Command::RemoteName& cmd, const uint32_t error) {
                    if (error != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Failed to request remote name [%d]"), error));
                    }
                });
            }
            void Callback(const std::function<void(IBluetooth::IClassic::ISecurityCallback* cb)>& action)
            {
                IBluetooth::IClassic::ISecurityCallback* callback = nullptr;

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
            uint32_t SecurityKey(const Bluetooth::LinkKey& lk) override
            {
                uint32_t result = Core::ERROR_GENERAL;
                if (lk.IsValid() == true) {
                    result = Core::ERROR_NONE;
                    bool isBonded = IsBonded();
                    _linkKeys.Add(lk);
                    if (IsBonded() ^ isBonded) {
                        BondedChange();
                    }
                    PairingComplete();
                }
                return (result);
            }
            uint32_t SecurityKey(const Bluetooth::LongTermKey& /* key */) override
            {
                TRACE(DeviceFlow, (_T("Not expecting a Long Term Key on a classic device")));
                return (Core::ERROR_GENERAL);
            }
            uint32_t SecurityKey(const Bluetooth::IdentityKey& /* key */) override
            {
                TRACE(DeviceFlow, (_T("Not expecting an Identity Key on a classic device")));
                return (Core::ERROR_GENERAL);
            }
            void PurgeSecurityKeys() override
            {
                bool isBonded = IsBonded();
                _linkKeys.Clear();
                if (IsBonded() ^ isBonded) {
                    BondedChange();
                }
            }
            Bluetooth::ManagementSocket::autoconnmode AutoConnectMode() const override
            {
                return (Bluetooth::ManagementSocket::DIRECT);
            }
            IBluetooth::IClassic::ISecurityCallback* SecurityCallback() const
            {
                return (_securityCallback);
            }

        private:
            IBluetooth::IClassic::ISecurityCallback* _securityCallback;
            Bluetooth::LinkKeys _linkKeys;
            DecoupledJob _userRequestJob;
            DecoupledJob _userReplyJob;
        }; // class DeviceRegular

    public:
        class DeviceLowEnergy : public DeviceImpl, public Exchange::IBluetooth::ILowEnergy {
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

                if (IsBonded() == true) {
                    AutoConnect(true);
                }
            }
            ~DeviceLowEnergy() override
            {
            }

        public:
            // IDevice overrides
            bool IsBonded() const override
            {
                return ( (_ltks.Entries() >= 1) && (_irk.IsValid()) );
            }
            virtual uint32_t Connect() override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(CONNECTING) == Core::ERROR_NONE) {
                    if (IsConnected() == false) {
                        if (IsBonded() == true) {
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
                            connect->supervision_timeout = htobs(0x0C80)/4;
                            connect->min_ce_length = htobs(0x0001);
                            connect->max_ce_length = htobs(0x0001);

                            result = Connector().Exchange(MAX_ACTION_TIMEOUT, connect, connect);
                            if (result == Core::ERROR_NONE) {
                                if (connect.Result() == 0) {
                                    Connection(btohs(connect.Response().handle));
                                } else {
                                    result = Core::ERROR_ASYNC_FAILED;
                                    TRACE(ControlFlow, (_T("ConnectLE command failed [%d]"), connect.Result()));
                                }
                            } else {
                                TRACE(Trace::Error, (_T("Failed to connect [%d]"), result));
                            }
                        } else {
                            result = Core::ERROR_ILLEGAL_STATE;
                            TRACE(Trace::Information, (_T("Device is not paired")));
                        }
                    } else {
                        result = Core::ERROR_ALREADY_CONNECTED;
                        TRACE(Trace::Information, (_T("Device is already connected!")));
                    }
                    ClearState(CONNECTING);
                } else {
                    TRACE(Trace::Information, (_T("Device already connected!")));
                }

                return (result);
            }

        public:
            // ILowEnergy overrides
            bool IsUUIDSupported(const string& uuid) const override
            {
                return false;
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
            INTERFACE_ENTRY(Exchange::IBluetooth::ILowEnergy)
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
                    bool isBonded = IsBonded();
                    _ltks.Add(ltk);
                    if (IsBonded() ^ isBonded) {
                        BondedChange();
                    }
                    PairingComplete();
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
                bool isBonded = IsBonded();
                _ltks.Clear();
                _irk = Bluetooth::IdentityKey();
                if (IsBonded() ^ isBonded) {
                    BondedChange();
                }
            }
            Bluetooth::ManagementSocket::autoconnmode AutoConnectMode() const override
            {
                return (Bluetooth::ManagementSocket::DIRECT);
            }

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
                    : Name(copy.Name)
                    , Supported(copy.Supported)
                    , Enabled(copy.Enabled)
                {
                    Add(_T("name"), &Name);
                    Add(_T("supported"), &Supported);
                    Add(_T("enabled"), &Enabled);
                }
                ~Property()
                {
                }

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
                , Version(0)
                , Address()
                , DeviceClass(0)
            {
                Add(_T("scanning"), &Scanning);
                Add(_T("devices"), &Devices);
                Add(_T("properties"), &Properties);
            }
            ~Status()
            {
            }

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
            Core::JSON::DecUInt32 Version;
            Core::JSON::String Address;
            Core::JSON::DecUInt32 DeviceClass;
            Core::JSON::ArrayType<Property> Properties;
        }; // class Status

    public:
        BluetoothControl(const BluetoothControl&) = delete;
        BluetoothControl& operator=(const BluetoothControl&) = delete;
        BluetoothControl()
            : _skipURL(0)
            , _adminLock()
            , _service(nullptr)
            , _adapters()
            , _btInterface(0)
            , _btAddress()
            , _devices()
            , _observers()
        {
            RegisterAll();
        }
        virtual ~BluetoothControl()
        {
            UnregisterAll();
        }

    public:
        BEGIN_INTERFACE_MAP(BluetoothControl)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
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

    public:
        inline ControlSocket& Connector() {
            return (_application);
        }

    private:
        void BondedChange(const DeviceImpl* device) {
            if (device->IsBonded() == true) {
                SaveDevice(device);
            }
            else {
                ForgetDevice(device);
            }
        }

    private:
        template<typename DEVICE=DeviceImpl>
        DEVICE* Find(const uint16_t handle) const;
        DeviceImpl* Find(const Bluetooth::Address& address) const;
        DeviceImpl* Find(const Bluetooth::Address& address, bool lowEnergy) const;
        template<typename DEVICE>
        DEVICE* Find(const Bluetooth::Address& address) const;
        void RemoveDevices(std::function<bool(DeviceImpl*)> filter);
        DeviceImpl* Discovered(const bool lowEnergy, const Bluetooth::Address& address);
        void Notification(const uint8_t subEvent, const uint16_t length, const uint8_t* dataFrame);
        void Capabilities(const Bluetooth::Address& device, const uint8_t capability, const uint8_t authentication, const uint8_t oob_data);
        void LoadController(const string& pathName, Data& data) const;
        void SaveController(const string& pathName, const Data& data);

        uint32_t LoadDevices(const string& devicePath, Bluetooth::ManagementSocket& administrator);
        uint32_t LoadDevice(const string&, Bluetooth::LinkKeys&, Bluetooth::LongTermKeys&, Bluetooth::IdentityKeys&);
        uint32_t ForgetDevice(const DeviceImpl* device);
        uint32_t SaveDevice(const DeviceImpl* device) const;

        bool AutoConfirmPasskey() const
        {
            return (_config.AutoPasskeyConfirm.Value());
        }

    private:
        // JSON-RPC
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_scan(const JsonData::BluetoothControl::ScanParamsData& params);
        uint32_t endpoint_connect(const JsonData::BluetoothControl::ConnectParamsInfo& params);
        uint32_t endpoint_disconnect(const JsonData::BluetoothControl::ConnectParamsInfo& params);
        uint32_t endpoint_pair(const JsonData::BluetoothControl::PairParamsData& params);
        uint32_t endpoint_unpair(const JsonData::BluetoothControl::ConnectParamsInfo& params);
        uint32_t endpoint_abortpairing(const JsonData::BluetoothControl::ConnectParamsInfo& params);
        uint32_t endpoint_pincode(const JsonData::BluetoothControl::PincodeParamsData& params);
        uint32_t endpoint_passkey(const JsonData::BluetoothControl::PasskeyParamsInfo& params);
        uint32_t endpoint_confirmpasskey(const JsonData::BluetoothControl::ConfirmpasskeyParamsData& params);
        uint32_t get_adapters(Core::JSON::ArrayType<Core::JSON::DecUInt16>& response) const;
        uint32_t get_adapter(const string& index, JsonData::BluetoothControl::AdapterData& response) const;
        uint32_t get_devices(Core::JSON::ArrayType<Core::JSON::String>& response) const;
        uint32_t get_device(const string& index, JsonData::BluetoothControl::DeviceData& response) const;
        void event_scancomplete();
        void event_devicestatechange(const string& address, const JsonData::BluetoothControl::DevicestatechangeParamsData::DevicestateType& state,
                                     const JsonData::BluetoothControl::DevicestatechangeParamsData::DisconnectreasonType& disconnectreason = JsonData::BluetoothControl::DevicestatechangeParamsData::DisconnectreasonType::TERMINATEDBYHOST);
        void event_pincoderequest(const string& address);
        void event_passkeyrequest(const string& address);
        void event_passkeyconfirmrequest(const string& address, const uint32_t& secret);

    private:
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& request);

    private:
        uint8_t _skipURL;
        Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        std::list<uint16_t> _adapters;
        uint16_t _btInterface;
        Bluetooth::Address _btAddress;
        std::list<DeviceImpl*> _devices;
        std::list<IBluetooth::INotification*> _observers;
        Config _config;
        ControlSocket _application;
        string _persistentStoragePath;
    }; // class BluetoothControl

    template<>
    inline BluetoothControl::DeviceImpl* BluetoothControl::Find<BluetoothControl::DeviceImpl>(const Bluetooth::Address& search) const
    {
        return (Find(search));
    }
    template<>
    inline BluetoothControl::DeviceRegular* BluetoothControl::Find<BluetoothControl::DeviceRegular>(const Bluetooth::Address& search) const
    {
        return (reinterpret_cast<DeviceRegular*>(Find(search, false)));
    }
    template<>
    inline BluetoothControl::DeviceLowEnergy* BluetoothControl::Find< BluetoothControl::DeviceLowEnergy>(const Bluetooth::Address& search) const
    {
        return (reinterpret_cast<DeviceLowEnergy*>(Find(search, true)));
    }

} //namespace Plugin

}
