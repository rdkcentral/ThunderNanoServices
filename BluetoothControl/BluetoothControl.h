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

namespace WPEFramework {

namespace Plugin {

class BluetoothControl : public PluginHost::IPlugin
                       , public PluginHost::IWeb
                       , public PluginHost::JSONRPC
                       , public Exchange::IBluetooth {
    private:
        static uint32_t UnpackDeviceClass(const uint8_t buffer[3])
        {
            ASSERT(buffer != nullptr);
            return ((buffer[2] << 16) | (buffer[1] << 8) | (buffer[0]));
        }

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
            ManagementFlow(const uint16_t opcode, const TCHAR* opcodeStr,  const Bluetooth::Address& address, const uint8_t type)
            {
                ASSERT(type < 3);
                static const TCHAR* typeStr[] = { _T("BREDR"), _T("LE_PUBLIC"), _T("LE_RANDOM") };
                Trace::Format(_text, _T("%s(0x%02X) address=%s, type=%s"), opcodeStr, opcode, address.ToString().c_str(), typeStr[type]);
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
        }; // class ManagementFlow

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
            ControlFlow(const uint16_t opcode, const TCHAR* opcodeStr, const uint16_t handle)
            {
                Trace::Format(_text, _T("%s(0x%02X) handle=%i"), opcodeStr, opcode, handle);
            }
            ControlFlow(const uint16_t opcode, const TCHAR* opcodeStr, const Bluetooth::Address& address)
            {
                Trace::Format(_text, _T("%s(0x%02X) address=%s"), opcodeStr, opcode, address.ToString().c_str());
            }
            ControlFlow(const uint16_t opcode, const TCHAR* opcodeStr,  const Bluetooth::Address& address, const uint8_t type)
            {
                ASSERT(type < 3);
                static const TCHAR* typeStr[] = { _T("BREDR"), _T("LE_PUBLIC"), _T("LE_RANDOM") };
                Trace::Format(_text, _T("%s(0x%02X) address=%s, type=%s"), opcodeStr, opcode, address.ToString().c_str(), typeStr[type]);
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
        }; // class ControlFlow

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
        }; // class DeviceFlow

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
                    uint16_t opCode = btohs(header.opcode);
                    uint16_t device = btohs(header.index);
                    uint16_t packageLen = btohs(header.len);

                    switch (opCode) {
                    case MGMT_EV_CMD_COMPLETE: {
                        const mgmt_ev_cmd_complete* info = reinterpret_cast<const mgmt_ev_cmd_complete*>(data);
                        TRACE(ManagementFlow, (_T("MGMT_EV_CMD_COMPLETE(%02X)"), MGMT_EV_CMD_COMPLETE));
                        TRACE(ManagementFlow, (_T("  opcode=%02x"), info->opcode));
                        if (info->opcode == MGMT_OP_PAIR_DEVICE) {
                            const mgmt_rp_pair_device* payload = reinterpret_cast<const mgmt_rp_pair_device*>(info->data);
                            Bluetooth::Address address(payload->addr.bdaddr);
                            Bluetooth::Address::type type(static_cast<const Bluetooth::Address::type>(payload->addr.type));
                            _parent.PairingComplete(address, type, info->status);
                        }
                        break;
                    }
                    case MGMT_EV_CONTROLLER_ERROR:
                        Update(reinterpret_cast<const mgmt_ev_controller_error*>(data));
                        break;
                    case MGMT_EV_NEW_CONN_PARAM:
                        Update(reinterpret_cast<const mgmt_ev_new_conn_param*>(data));
                        break;
                    case MGMT_EV_DEVICE_CONNECTED:
                        Update(reinterpret_cast<const mgmt_ev_device_connected*>(data));
                        break;
                    case MGMT_EV_DEVICE_DISCONNECTED:
                        Update(reinterpret_cast<const mgmt_ev_device_disconnected*>(data));
                        break;
                    case MGMT_EV_PIN_CODE_REQUEST:
                        Update(reinterpret_cast<const mgmt_ev_pin_code_request*>(data));
                        break;
                    case MGMT_EV_USER_PASSKEY_REQUEST:
                        Update(reinterpret_cast<const mgmt_ev_user_passkey_request*>(data));
                        break;
                    case MGMT_EV_USER_CONFIRM_REQUEST:
                        Update(reinterpret_cast<const mgmt_ev_user_confirm_request*>(data));
                        break;
                    case MGMT_EV_NEW_LINK_KEY:
                        Update(reinterpret_cast<const mgmt_ev_new_link_key*>(data));
                        break;
                    case MGMT_EV_NEW_IRK:
                        Update(reinterpret_cast<const mgmt_ev_new_irk*>(data));
                        break;
                    case MGMT_EV_NEW_CSRK:
                        Update(reinterpret_cast<const mgmt_ev_new_csrk*>(data));
                        break;
                    case MGMT_EV_NEW_LONG_TERM_KEY:
                        Update(reinterpret_cast<const mgmt_ev_new_long_term_key*>(data));
                        break;
                    default:
                        TRACE(ManagementFlow, (_T("UNKNOWN MGMT EVENT: device=%d, opcode=0x%04X, length=%d"),
                                               device, opCode, packageLen));
                        if (packageLen > 0) {
                            string dataText;
                            Core::ToHexString(data, packageLen, dataText);
                            TRACE(ManagementFlow, (_T("  data=%s"), dataText.c_str()));
                        }
                        break;
                    }
                }
                void Update(const mgmt_ev_controller_error* info)
                {
                    TRACE(ManagementFlow, (_T("MGMT_EV_CONTROLLER_ERROR(%02X)"), MGMT_EV_CONTROLLER_ERROR));
                    TRACE(ManagementFlow, (_T("  error_code=0x%02X"), info->error_code));
                }
                void Update(const mgmt_ev_new_conn_param* info)
                {
                    TRACE(ManagementFlow, (MGMT_EV_NEW_CONN_PARAM, _T("MGMT_EV_NEW_CONN_PARAM"),
                                           Bluetooth::Address(info->addr.bdaddr), info->addr.type));
                    TRACE(ManagementFlow, (_T("  store_hint=%d, min_interval=%d, max_interval=%d, latency=%d, timeout=%d"),
                                           info->store_hint, btohs(info->min_interval), btohs(info->max_interval),
                                           btohs(info->latency), btohs(info->timeout)));
                }
                void Update(const mgmt_ev_device_connected* info)
                {
                    string eir; Core::ToHexString(info->eir, btohs(info->eir_len), eir);

                    TRACE(ManagementFlow, (MGMT_EV_DEVICE_CONNECTED, _T("MGMT_EV_DEVICE_CONNECTED"),
                                           Bluetooth::Address(info->addr.bdaddr), info->addr.type));
                    TRACE(ManagementFlow, (_T("  flags=0x%08x, eir_len=%i, eir=%s"), btohl(info->flags), btohs(info->eir_len), eir.c_str()));
                }
                void Update(const mgmt_ev_device_disconnected* info)
                {
                    TRACE(ManagementFlow, (MGMT_EV_DEVICE_DISCONNECTED, _T("MGMT_EV_DEVICE_DISCONNECTED"),
                                           Bluetooth::Address(info->addr.bdaddr), info->addr.type));
                    TRACE(ManagementFlow, (_T("  reason=%d"), info->reason));
                }
                void Update(const mgmt_ev_pin_code_request* info)
                {
                    Bluetooth::Address address(info->addr.bdaddr);
                    Bluetooth::Address::type type(static_cast<const Bluetooth::Address::type>(info->addr.type));

                    TRACE(ManagementFlow, (MGMT_EV_PIN_CODE_REQUEST, _T("MGMT_EV_PIN_CODE_REQUEST"), address, type));
                    TRACE(ManagementFlow, (_T("  secure=%d"), info->secure));

                    _parent.RequestPINCode(address, type);
                }
                void Update(const mgmt_ev_user_passkey_request* info)
                {
                    Bluetooth::Address address(info->addr.bdaddr);
                    Bluetooth::Address::type type(static_cast<const Bluetooth::Address::type>(info->addr.type));

                    TRACE(ManagementFlow, (MGMT_EV_USER_PASSKEY_REQUEST, _T("MGMT_EV_USER_PASSKEY_REQUEST"), address, type));

                    _parent.RequestPasskey(address, type);
                }
                void Update(const mgmt_ev_user_confirm_request* info)
                {
                    Bluetooth::Address address(info->addr.bdaddr);
                    Bluetooth::Address::type type(static_cast<const Bluetooth::Address::type>(info->addr.type));
                    uint32_t passkey = btohl(info->value);

                    TRACE(ManagementFlow, (MGMT_EV_USER_CONFIRM_REQUEST, _T("MGMT_EV_USER_CONFIRM_REQUEST"), address, type));
                    TRACE(ManagementFlow, (_T("  hint=%d, value=%06d"), info->confirm_hint, passkey));

                    _parent.RequestPasskeyConfirm(address, type, passkey);
                }
                void Update(const mgmt_ev_new_link_key* info)
                {
                    Bluetooth::Address address(info->key.addr.bdaddr);
                    Bluetooth::Address::type type(static_cast<const Bluetooth::Address::type>(info->key.addr.type));
                    string key; Core::ToHexString(info->key.val, sizeof(info->key.val), key);

                    TRACE(ManagementFlow, (MGMT_EV_NEW_LINK_KEY, _T("MGMT_EV_NEW_LINK_KEY"), address, info->key.addr.type));
                    TRACE(ManagementFlow, (_T("  store_hint=%d, key.type=%d, key.val=%s"),
                                           info->store_hint, info->key.type, key.c_str()));

                    if (info->store_hint != 0) {
                        if (_parent.SecurityKey(address, type,
                                                Bluetooth::LinkKey(info->key.addr.bdaddr, info->key.val,
                                                                   info->key.pin_len, info->key.addr.type)) != Core::ERROR_NONE) {
                            TRACE(Trace::Error, (_T("Failed to store the link key")));
                        }
                    }
                }
                void Update(const mgmt_ev_new_irk* info)
                {
                    Bluetooth::Address address(info->key.addr.bdaddr);
                    Bluetooth::Address::type type(static_cast<const Bluetooth::Address::type>(info->key.addr.type));
                    string key; Core::ToHexString(info->key.val, sizeof(info->key.val), key);

                    TRACE(ManagementFlow, (MGMT_EV_NEW_IRK, _T("MGMT_EV_NEW_IRK"),
                                           address, info->key.addr.type));
                    TRACE(ManagementFlow, (_T("  store_hint=%d, rpa=%s, key.val=%s"),
                                           info->store_hint, Bluetooth::Address(info->rpa).ToString().c_str(), key.c_str()));

                    if (info->store_hint != 0) {
                        if (_parent.SecurityKey(address, type,
                                                Bluetooth::IdentityKey(info->key.addr.bdaddr, info->key.addr.type, info->key.val)) != Core::ERROR_NONE) {
                            TRACE(Trace::Error, (_T("Failed to store IRK")));
                        }
                    }
                }
                void Update(const mgmt_ev_new_csrk* info)
                {
                    string key; Core::ToHexString(info->key.val, sizeof(info->key.val), key);

                    TRACE(ManagementFlow, (MGMT_EV_NEW_CSRK, _T("MGMT_EV_NEW_CSRK"),
                                           Bluetooth::Address(info->key.addr.bdaddr), info->key.addr.type));
                    TRACE(ManagementFlow, (_T("  store_hint=%d, key.type=%d, key.val=%s"),
                                           info->store_hint, info->key.type, key.c_str()));

                    /* For signed GATT writes, not required yet. */
                }
                void Update(const mgmt_ev_new_long_term_key* info)
                {
                    Bluetooth::Address address(info->key.addr.bdaddr);
                    Bluetooth::Address::type type(static_cast<const Bluetooth::Address::type>(info->key.addr.type));
                    string key; Core::ToHexString(info->key.val, sizeof(info->key.val), key);

                    TRACE(ManagementFlow, (MGMT_EV_NEW_LONG_TERM_KEY, _T("MGMT_EV_NEW_LONG_TERM_KEY"), address, info->key.addr.type));
                    TRACE(ManagementFlow, (_T("  store_hint=%d, key.type=%d, key.master=%d"),
                                           info->store_hint, info->key.type, info->key.master));
                    TRACE(ManagementFlow, (_T("  key.enc_size=%u, key.ediv=%u, key.rand=%llu, key.val=%s"),
                                           info->key.enc_size, btohs(info->key.ediv), btohll(info->key.rand), key.c_str()));

                    if (info->store_hint != 0) {
                        if (_parent.SecurityKey(address, type,
                                                Bluetooth::LongTermKey(info->key.addr.bdaddr, info->key.addr.type, info->key.type,
                                                                       info->key.master, info->key.enc_size, btohs(info->key.ediv),
                                                                       btohll(info->key.rand), info->key.val)) != Core::ERROR_NONE) {
                            TRACE(Trace::Error, (_T("Failed to store LTK")));
                        }
                    }
                }

            private:
                ControlSocket& _parent;
            }; // class ManagementSocket

            // The bluetooth library has some unexpected behaviour. For example, the scan of NON-BLE devices
            // is a blocking call for the duration of the passed in time. Which is, I think, very intrusive
            // fo any responsive design. If a RESTFull call would start a scan, the call would last the duration
            // of the scan, which is typicall >= 10Secods which is unacceptable, so it needs to be decoupled.
            // This decoupling is done on this internal Worker thread.
            class ScanJob : public Core::IDispatch {
            private:
                enum scanMode {
                    LOW_ENERGY = 0x01,
                    REGULAR = 0x02,
                    PASSIVE = 0x04,
                    LIMITED = 0x08
                };

            public:
                ScanJob() = delete;
                ScanJob(const ScanJob&) = delete;
                ScanJob& operator=(const ScanJob&) = delete;
                ScanJob(ControlSocket* parent)
                    : _parent(*parent)
                    , _mode(0)
                {
                }
                ~ScanJob() = default;

            public:
                void Load(const uint16_t scanTime, const uint32_t type, const uint8_t flags)
                {
                    if (_mode == 0) {
                        _mode = REGULAR;
                        _scanTime = scanTime;
                        _type = type;
                        _flags = flags;
                        Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                    }
                }
                void Load(const uint16_t scanTime, const bool limited, const bool passive)
                {
                    if (_mode == 0) {
                        _mode = LOW_ENERGY | (passive ? PASSIVE : 0) | (limited ? LIMITED : 0);
                        _scanTime = scanTime;
                        Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                    }
                }

            private:
                void Dispatch() override
                {
                    if ((_mode & REGULAR) != 0) {
                        TRACE(ControlFlow, (_T("Start regular scan: %s"), Core::Time::Now().ToRFC1123().c_str()));
                        _parent.Run(_scanTime, _type, _flags);
                    } else {
                        TRACE(ControlFlow, (_T("Start Low Energy scan: %s"), Core::Time::Now().ToRFC1123().c_str()));
                        _parent.Run(_scanTime, ((_mode & LIMITED) != 0), ((_mode & PASSIVE) != 0));
                    }
                    TRACE(ControlFlow, (_T("Scan completed: %s"), Core::Time::Now().ToRFC1123().c_str()));
                    _parent.Application()->event_scancomplete();
                    _mode = 0;
                }

            private:
                ControlSocket& _parent;
                uint16_t _scanTime;
                uint32_t _type;
                uint8_t _flags;
                uint8_t _mode;
            }; // class ScanJob

        public:
            ControlSocket(const ControlSocket&) = delete;
            ControlSocket& operator=(const ControlSocket&) = delete;
            ControlSocket()
                : Bluetooth::HCISocket()
                , _parent(nullptr)
                , _activity(Core::ProxyType<ScanJob>::Create(this))
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
                return(_administrator.Pair(remote, type));
            }
            uint32_t Unpair(const Bluetooth::Address& remote, const Bluetooth::Address::type type)
            {
                return(_administrator.Unpair(remote, type));
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
                Core::IWorkerPool::Instance().Revoke(Core::ProxyType<Core::IDispatch>(_activity));
                Bluetooth::ManagementSocket::Down(_administrator.DeviceId());
                _administrator.DeviceId(HCI_DEV_NONE);
                _parent = nullptr;
                return (result);
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
            void Run(const uint16_t scanTime, const uint32_t type, const uint8_t flags)
            {
                Bluetooth::HCISocket::Scan(scanTime, type, flags);
            }
            void Run(const uint16_t scanTime, const bool limited, const bool passive)
            {
                Bluetooth::HCISocket::Scan(scanTime, limited, passive);
            }
            void Discovered(const bool lowEnergy, const Bluetooth::Address& address, const Bluetooth::EIR& info) override
            {
                if (Application() != nullptr) {
                    Application()->Discovered(lowEnergy, address, info);
                }
            }
            void Update(const le_advertising_info& eventData) override
            {
                string data; Core::ToHexString(eventData.data, eventData.length, data);
                Bluetooth::Address address(eventData.bdaddr);

                TRACE(ControlFlow, (EVT_LE_ADVERTISING_REPORT, _T("EVT_LE_ADVERTISING_REPORT"), address, (eventData.bdaddr_type + 1)));
                TRACE(ControlFlow, (_T("  evt_type=%d, length=%d, data=%s"), eventData.evt_type, eventData.length, data.c_str()));

                if ((Application() != nullptr) && (eventData.bdaddr_type == 0 /* public */)
                        && ((eventData.evt_type == 0 /* undirected connectable advertisement */) || (eventData.evt_type == 4 /* scan response */))) {
                    Bluetooth::EIR eir(eventData.data, eventData.length);

                    DeviceImpl* device = Application()->Discovered(true, address, eir);
                    if (device != nullptr) {
                        if (eir.Class() != 0) {
                            device->Class(eir.Class());
                        }
                        if (eir.CompleteName().empty() == false) {
                            device->Name(eir.CompleteName());
                        }
                    }
                }
            }
            void Update(const hci_event_hdr& eventData) override
            {
                const uint8_t* data = &(reinterpret_cast<const uint8_t*>(&eventData)[sizeof(hci_event_hdr)]);

                switch (eventData.evt) {
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
                    case EVT_EXTENDED_INQUIRY_RESULT: {
                        uint8_t count = data[0];
                        while (count) {
                            Update(reinterpret_cast<const extended_inquiry_info*>(data + 1 /* count */ + (count * sizeof(extended_inquiry_info))));
                            count--;
                        }
                        break;
                    }
                    case EVT_INQUIRY_RESULT_WITH_RSSI:
                        Update(reinterpret_cast<const inquiry_info_with_rssi*>(data + 1));
                        break;
                    case EVT_CONN_REQUEST:
                        Update(reinterpret_cast<const evt_conn_request*>(data));
                        break;
                    case EVT_CONN_COMPLETE:
                        Update(reinterpret_cast<const evt_conn_complete*>(data));
                        break;
                    case EVT_DISCONN_COMPLETE:
                        Update(reinterpret_cast<const evt_disconn_complete*>(data));
                        break;
                    case EVT_REMOTE_NAME_REQ_COMPLETE:
                        Update(reinterpret_cast<const evt_remote_name_req_complete*>(data));
                        break;
                    case EVT_IO_CAPABILITY_REQUEST:
                        Update(reinterpret_cast<const evt_io_capability_request*>(data));
                        break;
                    case EVT_IO_CAPABILITY_RESPONSE:
                        Update(reinterpret_cast<const evt_io_capability_response*>(data));
                        break;
                    case EVT_USER_CONFIRM_REQUEST:
                        Update(reinterpret_cast<const evt_user_confirm_request*>(data));
                        break;
                    case EVT_USER_PASSKEY_REQUEST:
                        Update(reinterpret_cast<const evt_user_passkey_request*>(data));
                        break;
                    case EVT_USER_PASSKEY_NOTIFY:
                        Update(reinterpret_cast<const evt_user_passkey_notify*>(data));
                        break;
                    case EVT_READ_REMOTE_FEATURES_COMPLETE:
                        Update(reinterpret_cast<const evt_read_remote_features_complete*>(data));
                        break;
                    case EVT_LINK_KEY_REQ:
                        Update(reinterpret_cast<const evt_link_key_req*>(data));
                        break;
                    case EVT_LINK_KEY_NOTIFY:
                        Update(reinterpret_cast<const evt_link_key_notify*>(data));
                        break;
                    case EVT_AUTH_COMPLETE:
                        Update(reinterpret_cast<const evt_auth_complete*>(data));
                        break;
                    case EVT_SIMPLE_PAIRING_COMPLETE:
                        Update(reinterpret_cast<const evt_simple_pairing_complete*>(data));
                        break;
                    case EVT_LE_META_EVENT: {
                        const evt_le_meta_event* input = reinterpret_cast<const evt_le_meta_event*>(data);
                        switch (input->subevent) {
                        case EVT_LE_CONN_COMPLETE:
                            Update(reinterpret_cast<const evt_le_connection_complete*>(input->data));
                            break;
                        case EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE:
                            Update(reinterpret_cast<const evt_le_read_remote_used_features_complete*>(input->data));
                            break;
                        case EVT_LE_CONN_UPDATE_COMPLETE:
                            Update(reinterpret_cast<const evt_le_connection_update_complete*>(input->data));
                            break;
                        default:
                            TRACE(ControlFlow, (_T("EVT_LE_META_EVENT(%02X): unexpected subevent: %d"),
                                                EVT_LE_META_EVENT, input->subevent));
                            break;
                        }
                        break;
                    }
                    default:
                        TRACE(ControlFlow, (_T("UNKNOWN EVENT: %02X"), eventData.evt));
                        break;
                }
            }
            void Update(const extended_inquiry_info* data)
            {
                Bluetooth::Address address(data->bdaddr);
                uint32_t deviceClass = UnpackDeviceClass(data->dev_class);

                TRACE(ControlFlow, (EVT_EXTENDED_INQUIRY_RESULT, _T("EVT_EXTENDED_INQUIRY_RESULT"), address));
                TRACE(ControlFlow, (_T("  dev_class=0x%06X, pscan_rep_mode=%d, pscan_period_mode=%d, clock_offset=%d, rssi=%d"),
                                    deviceClass, data->pscan_rep_mode, data->pscan_period_mode, btohs(data->clock_offset), data->rssi));

                UpdateDevice<DeviceImpl>(address, [&](DeviceImpl* device) {
                    device->Class(deviceClass);
                });
            }
            void Update(const inquiry_info_with_rssi* data)
            {
                Bluetooth::Address address(data->bdaddr);
                uint32_t deviceClass = UnpackDeviceClass(data->dev_class);

                TRACE(ControlFlow, (EVT_INQUIRY_RESULT_WITH_RSSI, _T("EVT_INQUIRY_RESULT_WITH_RSSI"), address));
                TRACE(ControlFlow, (_T("  dev_class=0x%06X, pscan_rep_mode=%d, pscan_period_mode=%d, clock_offset=%d, rssi=%d"),
                                    deviceClass, data->pscan_rep_mode, data->pscan_period_mode, btohs(data->clock_offset), data->rssi));;

                UpdateDevice<DeviceImpl>(address, [&](DeviceImpl* device) {
                    device->Class(deviceClass);
                });
            }
            void Update(const evt_conn_request* data)
            {
                Bluetooth::Address address(data->bdaddr);
                uint32_t deviceClass = UnpackDeviceClass(data->dev_class);

                TRACE(ControlFlow, (EVT_CONN_REQUEST, _T("EVT_CONN_REQUEST"), address));
                TRACE(ControlFlow, (_T("  dev_class=0x%06X, link_type=%d"), address.ToString().c_str(), deviceClass, data->link_type));
            }
            void Update(const evt_conn_complete* data)
            {
                Bluetooth::Address address(data->bdaddr);
                uint16_t handle(btohs(data->handle));

                TRACE(ControlFlow, (EVT_CONN_COMPLETE, _T("EVT_CONN_COMPLETE"), address));
                TRACE(ControlFlow, (_T("  status=%d, handle=%d, link_type=%d, encr_mode=%d"),
                                    data->status, handle, data->link_type, data->encr_mode));

                if (data->status == 0) {
                    UpdateDevice<DeviceRegular>(address, [&](DeviceRegular* device) {
                        device->Connection(handle);
                    });
                }
            }
            void Update(const evt_disconn_complete* data)
            {
                uint16_t handle(btohs(data->handle));

                TRACE(ControlFlow, (EVT_DISCONN_COMPLETE, _T("EVT_DISCONN_COMPLETE"), handle));
                TRACE(ControlFlow, (_T("  status=%d, reason=%d"), data->status, data->reason));

                if (data->status == 0) {
                   UpdateDevice<DeviceImpl>(handle, [&](DeviceImpl* device) {
                        device->Disconnection(data->reason);
                    });
                }
            }
            void Update(const evt_le_connection_complete* data)
            {
                uint16_t handle(btohs(data->handle));
                Bluetooth::Address address(data->peer_bdaddr);

                TRACE(ControlFlow, (EVT_LE_CONN_COMPLETE, _T("EVT_LE_CONN_COMPLETE"), address));
                TRACE(ControlFlow, (_T("  status=%d, handle=%d"), data->status, handle));

                if (data->status == 0) {
                    UpdateDevice<DeviceLowEnergy>(address, [&](DeviceLowEnergy* device) {
                        device->Connection(handle);
                    });
                }
            }
            void Update(const evt_le_connection_update_complete* data)
            {
                TRACE(ControlFlow, (EVT_LE_CONN_UPDATE_COMPLETE, _T("EVT_LE_CONN_UPDATE_COMPLETE"), btohs(data->handle)));
                TRACE(ControlFlow, (_T("  status=%d, interval=%d, latency=%d, supervision_timeout=%d"),
                                    data->status, btohs(data->interval), btohs(data->latency), btohs(data->supervision_timeout)));
            }
            void Update(const evt_remote_name_req_complete* data)
            {
                Bluetooth::Address address(data->bdaddr);

                uint8_t length = 0;

                if (data->status == 0) {
                    while ((length < HCI_MAX_NAME_LENGTH) && ::isprint(data->name[length])) {
                        length++;
                    }
                }

                string friendlyName(reinterpret_cast<const char*>(data->name), length);

                TRACE(ControlFlow, (EVT_REMOTE_NAME_REQ_COMPLETE, _T("EVT_REMOTE_NAME_REQ_COMPLETE"), address));
                TRACE(ControlFlow, (_T(" status=%d, name='%s'"), data->status, friendlyName.c_str()));

                if (friendlyName.empty() == false) {
                    UpdateDevice<DeviceImpl>(address, [&](DeviceImpl* device) {
                        device->Name(friendlyName);
                    });
                }
            }
            void Update(const evt_io_capability_request* data)
            {
                Bluetooth::Address address(data->bdaddr);

                TRACE(ControlFlow, (EVT_IO_CAPABILITY_REQUEST, _T("EVT_IO_CAPABILITY_REQUEST"), address));
            }
            void Update(const evt_io_capability_response* data)
            {
                Bluetooth::Address address(data->bdaddr);

                TRACE(ControlFlow, (EVT_IO_CAPABILITY_RESPONSE, _T("EVT_IO_CAPABILITY_RESPONSE"), address));
                TRACE(ControlFlow, (_T("  capability=%i, authentication=%i, oob_data=%i"),
                                    data->capability, data->authentication, data->oob_data));

                UpdateDevice<DeviceImpl>(address, [&](DeviceImpl* device) {
                    device->Capabilities(data->capability, data->authentication, data->oob_data);
                });
            }
            void Update(const evt_user_confirm_request* data)
            {
                Bluetooth::Address address(data->bdaddr);

                TRACE(ControlFlow, (EVT_USER_CONFIRM_REQUEST, _T("EVT_USER_CONFIRM_REQUEST"), address));
                TRACE(ControlFlow, (_T("  passkey=%06i"), btohl(data->passkey)));
            }
            void Update(const evt_user_passkey_request* data)
            {
                Bluetooth::Address address(data->bdaddr);

                TRACE(ControlFlow, (EVT_USER_PASSKEY_REQUEST, _T("EVT_USER_PASSKEY_REQUEST"), address));
            }
            void Update(const evt_user_passkey_notify* data)
            {
                Bluetooth::Address address(data->bdaddr);

                TRACE(ControlFlow, (EVT_USER_PASSKEY_NOTIFY, _T("EVT_USER_PASSKEY_NOTIFY"), address));
                TRACE(ControlFlow, (_T("  passkey=%06i"), btohl(data->passkey)));
            }
            void Update(const evt_read_remote_features_complete* data)
            {
                uint16_t handle(btohs(data->handle));
                string features; Core::ToHexString(data->features, sizeof(data->features), features);

                TRACE(ControlFlow, (EVT_READ_REMOTE_FEATURES_COMPLETE, _T("EVT_READ_REMOTE_FEATURES_COMPLETE"), handle));
                TRACE(ControlFlow, (_T("  status=%d, features=%s"), data->status, features.c_str()));

                if (data->status == 0) {
                    UpdateDevice<DeviceImpl>(handle, [&](DeviceImpl* device) {
                        device->Features(sizeof(data->features), data->features);
                    });
                }
            }
            void Update(const evt_le_read_remote_used_features_complete* data)
            {
                uint16_t handle(btohs(data->handle));
                string features; Core::ToHexString(data->features, sizeof(data->features), features);

                TRACE(ControlFlow, (EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE, _T("EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE"), handle));
                TRACE(ControlFlow, (_T("  status=%d, features=%s"), data->status, features.c_str()));

                if (data->status == 0) {
                    UpdateDevice<DeviceImpl>(handle, [&](DeviceImpl* device) {
                        device->Features(sizeof(data->features), data->features);
                    });
                }
            }
            void Update(const evt_link_key_req* data)
            {
                Bluetooth::Address address(data->bdaddr);

                TRACE(ControlFlow, (EVT_LINK_KEY_NOTIFY, _T("EVT_LINK_KEY_REQ"), address));
            }
            void Update(const evt_link_key_notify* data)
            {
                Bluetooth::Address address(data->bdaddr);
                string key; Core::ToHexString(data->link_key, sizeof(data->link_key), key);

                TRACE(ControlFlow, (EVT_LINK_KEY_NOTIFY, _T("EVT_LINK_KEY_NOTIFY"), address));
                TRACE(ControlFlow, (_T("  key_type=%d, key=%s"), data->key_type, key.c_str()));
            }
            void Update(const evt_auth_complete* data)
            {
                TRACE(ControlFlow, (EVT_AUTH_COMPLETE, _T("EVT_AUTH_COMPLETE"), btohs(data->handle)));
                TRACE(ControlFlow, (_T("  status=%d"), data->status));
            }
            void Update(const evt_simple_pairing_complete* data)
            {
                Bluetooth::Address address(data->bdaddr);

                TRACE(ControlFlow, (EVT_SIMPLE_PAIRING_COMPLETE, _T("EVT_SIMPLE_PAIRING_COMPLETE"), address));
                TRACE(ControlFlow, (_T("  status=%d"), data->status));
            }

        private:
            BluetoothControl* _parent;
            Core::ProxyType<ScanJob> _activity;
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
        class EXTERNAL DeviceImpl : public Exchange::IBluetooth::IDevice {
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
            class UpdateJob : public Core::IDispatch {
            public:
                UpdateJob() = delete;
                UpdateJob(const UpdateJob&) = delete;
                UpdateJob& operator=(const UpdateJob&) = delete;

                UpdateJob(DeviceImpl* parent)
                    : _parent(parent)
                {
                    ASSERT(parent != nullptr);
                }
                ~UpdateJob() = default;

            public:
                void Submit()
                {
                    Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                }
                void Dispatch() override
                {
                    _parent->Callback<IBluetooth::IDevice::ICallback>(_parent->Callback(), [](IBluetooth::IDevice::ICallback* cb) {
                        cb->Updated();
                    });
                }

            private:
                DeviceImpl* _parent;
            }; // class UpdateJob

        private:
            class AutoConnectJob : public Core::IDispatch {
            public:
                AutoConnectJob() = delete;
                AutoConnectJob(const AutoConnectJob&) = delete;
                AutoConnectJob& operator=(const AutoConnectJob&) = delete;

                AutoConnectJob(DeviceImpl* parent)
                    : _parent(parent)
                    , _enable(false)
                {
                    ASSERT(parent != nullptr);
                }
                ~AutoConnectJob() = default;

            public:
                void Submit(bool enable)
                {
                    _enable = enable;
                    Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                }

            private:
                void Dispatch() override
                {
                    _parent->AutoConnect(_enable);
                }

            private:
                DeviceImpl* _parent;
                bool _enable;
            };

            class UserRequestJob : public Core::IDispatch {
            public:
                UserRequestJob() = delete;
                UserRequestJob(const UserRequestJob&) = delete;
                UserRequestJob& operator=(const UserRequestJob&) = delete;

                UserRequestJob(DeviceImpl* parent)
                    : _parent(parent)
                    , _passkey(~0)
                {
                    ASSERT(parent != nullptr);
                }
                ~UserRequestJob() = default;

            public:
                void Submit(uint32_t passkey)
                {
                    TRACE(Trace::Information, (_T("Requesting passkey %06d confirmation..."), passkey));
                    _passkey = passkey;
                    Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                }
                void Submit()
                {
                    TRACE(Trace::Information, (_T("Requesting passkey...")));
                    Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                }
                void Dispatch() override
                {
                    if (_passkey != static_cast<uint32_t>(~0)) {
                        _parent->Callback<IBluetooth::IDevice::ISecurityCallback>(_parent->SecurityCallback(), [&](IBluetooth::IDevice::ISecurityCallback* cb) {
                            cb->PasskeyConfirmRequest(_passkey);
                        });
                        _passkey = ~0;
                    } else {
                        _parent->Callback<IBluetooth::IDevice::ISecurityCallback>(_parent->SecurityCallback(), [&](IBluetooth::IDevice::ISecurityCallback* cb) {
                            cb->PasskeyRequest();
                        });
                    }
                }

            private:
                DeviceImpl* _parent;
                uint32_t _passkey;
            }; // class UserRequestJob

        private:
            class UserReplyJob : public Core::IDispatch {
            public:
                UserReplyJob() = delete;
                UserReplyJob(const UserReplyJob&) = delete;
                UserReplyJob& operator=(const UserReplyJob&) = delete;

                UserReplyJob(DeviceImpl* parent)
                    : _parent(parent)
                    , _confirm(false)
                    , _passkey(~0)
                {
                    ASSERT(parent != nullptr);
                }
                ~UserReplyJob() = default;

            public:
                void Submit(const bool confirm)
                {
                    TRACE(Trace::Information, (_T("Passkey confirmation reply: %s"), confirm? "YES" : "NO"));
                    _confirm = confirm;
                    Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                }
                void Submit(const uint32_t passkey)
                {
                    TRACE(Trace::Information, (_T("Passkey reply: %06d"), passkey));
                    _passkey = passkey;
                    Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                }

            private:
                void Dispatch() override
                {
                    if (_passkey != static_cast<uint32_t>(~0)) {
                        _parent->ReplyPasskey(_passkey);
                        _passkey = ~0;
                    } else {
                        _parent->ReplyPasskeyConfirm(_confirm);
                    }
                }

            private:
                DeviceImpl* _parent;
                bool _confirm;
                uint32_t _passkey;
            }; // class UserReplyJob

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
                , _autoConnection(false)
                , _callback(nullptr)
                , _securityCallback(nullptr)
                , _updateJob(Core::ProxyType<UpdateJob>::Create(this))
                , _autoConnectJob(Core::ProxyType<AutoConnectJob>::Create(this))
                , _userRequestJob(Core::ProxyType<UserRequestJob>::Create(this))
                , _userReplyJob(Core::ProxyType<UserReplyJob>::Create(this))
            {
                ASSERT(parent != nullptr);
                ::memset(_features, 0xFF, sizeof(_features));
            }
            DeviceImpl(BluetoothControl* parent, const Bluetooth::Address::type type, const uint16_t deviceId,
                       const Bluetooth::Address& remote, const Bluetooth::EIR& info)
                : DeviceImpl(parent, type, deviceId, remote)
            {
                _name = info.CompleteName();
                _class = info.Class();
                _uuids = info.UUIDs();
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
                    disconnect->reason = 2;

                    result = _parent->Connector().Exchange(MAX_ACTION_TIMEOUT, disconnect, disconnect);
                    if (result != Core::ERROR_NONE) {
                        TRACE(ControlFlow, (_T("Failed to disconnect. Error [%d]"), result));
                    }
                }

                return (result);
            }
            uint32_t Pair(const IBluetooth::pairingcapabilities capabilities) override
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (SetState(PAIRING) == Core::ERROR_NONE) {
                    result = _parent->Connector().Pair(Address(), AddressType(), static_cast<Bluetooth::ManagementSocket::capabilities>(capabilities));
                    if (result == Core::ERROR_INPROGRESS) {
                        TRACE(Trace::Information, (_T("Pairing of device %s in progress..."), Address().ToString().c_str()));
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
                    if (result != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Failed to abort pairing [%d]"), result));
                    }

                    ClearState(PAIRING);
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
                        TRACE(Trace::Error, (_T("Failed to unpair device %s [%d]"), Address().ToString().c_str(), result));
                    }

                    ClearState(UNPAIRING);
                    _updateJob->Submit();
                } else {
                    TRACE(Trace::Information, (_T("Device is currently busy")));
                }

                return (result);
            }
            void Passkey(const uint32_t passkey) override
            {
                _userReplyJob->Submit(passkey);
            }
            void ConfirmPasskey(const bool confirm) override
            {
                _userReplyJob->Submit(confirm);
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
                _userRequestJob->Submit();
            }
            void RequestPasskeyConfirm(const uint32_t passkey)
            {
                TRACE(Trace::Information, (_T("Pairing with device %s; confirm passkey %06d? (YES/NO)"), Address().ToString().c_str(), passkey));

                // Request passkey confirmation from client or, if auto confirm is enabled, reply already.
                if (_parent->AutoConfirmPasskey() == true) {
                    TRACE(Trace::Information, (_T("Auto-confirm enabled, accepting the passkey!")));
                    _userReplyJob->Submit(true);
                } else {
                    TRACE(Trace::Information, (_T("Waiting for user confirmation of the passkey...")));
                    _userRequestJob->Submit(passkey);
                }
            }
            void PairingComplete(uint8_t status)
            {
                // We consider ourselves paired if the link keys are exchanged, this event is merely to clear our state of pairing in case of timeout.
                TRACE(Trace::Information, (_T("Device %s pairing complete: [%d]"), _remote.ToString().c_str(), status));
                ClearState(PAIRING);
            }
            void BondedChange()
            {
                _parent->BondedChange(this);
                _updateJob->Submit();
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
                    _updateJob->Submit();
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

                if (IsBonded() && (_autoConnection == false)) {
                    // Setting autoconnection can only be done when the device is disconnected.
                    // The whitelist is not cleared on subsequent connections/disconnections, so this only needs to be done once.
                    // Bonded state has already been set accordingly at this time.
                    _autoConnectJob->Submit(IsBonded());
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

                _updateJob->Submit();
                _parent->event_devicestatechange(Address().ToString(), JsonData::BluetoothControl::DevicestatechangeParamsData::DevicestateType::DISCONNECTED, disconnReason);
            }
            void Name(const string& name)
            {
                bool updated = false;

                _state.Lock();

                if (name != _name) {
                    _name = name;
                    updated = true;
                }

                _state.Unlock();

                if (updated == true) {
                    TRACE(DeviceFlow, (_T("Device name updated to: '%s'"), name.c_str()));
                    _updateJob->Submit();
                }
            }
            void Class(const uint32_t classOfDevice)
            {
                bool updated = false;

                _state.Lock();

                if (classOfDevice != _class) {
                    _class = classOfDevice;
                    updated = true;
                }

                _state.Unlock();

                if (updated == true) {
                    TRACE(DeviceFlow, (_T("Device class )updated to: 0x%06X"), classOfDevice));
                    _updateJob->Submit();
                }
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
                        _autoConnection = true;
                    }
                } else {
                    uint32_t result = _parent->Connector().Control().RemoveDevice(AddressType(), Address());
                    if (result != Core::ERROR_NONE) {
                        TRACE(DeviceFlow, (_T("Could not remove device [%d]"), Address().ToString().c_str(), result));
                    }
                    else {
                        TRACE(DeviceFlow, (_T("Disabled autoconnect of device %s"), Address().ToString().c_str()));
                    }

                   _autoConnection = false;
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

        private:
            void ReplyPasskeyConfirm(const bool confirm)
            {
                _parent->Connector().Control().UserPasskeyConfirmReply(Address(), AddressType(), confirm);
            }
            void ReplyPasskey(const uint32_t passkey)
            {
                _parent->Connector().Control().UserPasskeyReply(Address(), AddressType(), passkey);
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
            bool _autoConnection;
            IBluetooth::IDevice::ICallback* _callback;
            IBluetooth::IDevice::ISecurityCallback* _securityCallback;
            Core::ProxyType<UpdateJob> _updateJob;
            Core::ProxyType<AutoConnectJob> _autoConnectJob;
            Core::ProxyType<UserRequestJob> _userRequestJob;
            Core::ProxyType<UserReplyJob> _userReplyJob;
        }; // class DeviceImpl

    public:
        class EXTERNAL DeviceRegular : public DeviceImpl, public Exchange::IBluetooth::IClassic {
        private:
            class UserRequestJob : public Core::IDispatch {
            public:
                UserRequestJob() = delete;
                UserRequestJob(const UserRequestJob&) = delete;
                UserRequestJob& operator=(const UserRequestJob&) = delete;

                UserRequestJob(DeviceRegular* parent)
                    : _parent(parent)
                {
                    ASSERT(parent != nullptr);
                }
                ~UserRequestJob() = default;

            public:
                void Submit()
                {
                    TRACE(Trace::Information, (_T("Requesting PIN code...")));
                    Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                }
                void Dispatch() override
                {
                    _parent->Callback([&](IBluetooth::IClassic::ISecurityCallback* cb) {
                        cb->PINCodeRequest();
                    });
                }

            private:
                DeviceRegular* _parent;
                uint32_t _passkey;
            }; // class UserRequestJob

        private:
            class UserReplyJob : public Core::IDispatch {
            public:
                UserReplyJob() = delete;
                UserReplyJob(const UserReplyJob&) = delete;
                UserReplyJob& operator=(const UserReplyJob&) = delete;

                UserReplyJob(DeviceRegular* parent)
                    : _parent(parent)
                    , _pin()
                {
                    ASSERT(parent != nullptr);
                }
                ~UserReplyJob() = default;

            public:
                void Submit(const string& pin)
                {
                    TRACE(Trace::Information, (_T("PIN code reply: %s"), pin)); // TODO: pitfall; potentially can be binary
                    _pin = pin;
                    Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                }

            private:
                void Dispatch() override
                {
                    _parent->ReplyPINCode(_pin);
                    _pin.clear();
                }

            private:
                DeviceRegular* _parent;
                string _pin;
            }; // class UserReplyJob

        public:
            DeviceRegular() = delete;
            DeviceRegular(const DeviceRegular&) = delete;
            DeviceRegular& operator=(const DeviceRegular&) = delete;

            DeviceRegular(BluetoothControl* parent, const uint16_t deviceId, const Bluetooth::Address& address, const Bluetooth::EIR& info)
                : DeviceImpl(parent, Bluetooth::Address::BREDR_ADDRESS, deviceId, address, info)
                , _securityCallback(nullptr)
                , _linkKeys()
                , _userRequestJob(Core::ProxyType<UserRequestJob>::Create(this))
                , _userReplyJob(Core::ProxyType<UserReplyJob>::Create(this))
            {
                RemoteName();
            }
            DeviceRegular(BluetoothControl* parent, const uint16_t deviceId, const Bluetooth::Address& address, const DeviceImpl::Config* config)
                : DeviceImpl(parent, Bluetooth::Address::BREDR_ADDRESS, deviceId, address)
                , _securityCallback(nullptr)
                , _linkKeys()
                , _userRequestJob(Core::ProxyType<UserRequestJob>::Create(this))
                , _userReplyJob(Core::ProxyType<UserReplyJob>::Create(this))
            {
                ASSERT(config != nullptr);

                Name(config->Name.Value());
                Class(config->Class.Value());
                config->Get(config->LinkKeys, address, Bluetooth::Address::BREDR_ADDRESS, _linkKeys);

                if (IsBonded() == true) {
                    AutoConnect(true);
                }
            }
            ~DeviceRegular() override
            {
            }

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
                    Bluetooth::HCISocket::Command::Connect connect;

                    connect.Clear();
                    connect->bdaddr = *(Locator().Data());
                    connect->pkt_type = htobs(HCI_DM1 | HCI_DM3 | HCI_DM5 | HCI_DH1 | HCI_DH3 | HCI_DH5);
                    connect->pscan_rep_mode = 0x02;
                    connect->clock_offset = 0x0000;
                    connect->role_switch = 0x01;

                    result = _parent->Connector().Exchange(MAX_ACTION_TIMEOUT, connect, connect);
                    if (result != Core::ERROR_NONE) {
                        TRACE(ControlFlow, (_T("Failed to connect. Error [%d]"), result));
                    }
                }

                return (result);
            }

        public:
            // IClassic overrides
            void PINCode(const string& pinCode) override
            {
                _userReplyJob->Submit(pinCode);
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
                _userRequestJob->Submit();
            }

        private:
            void ReplyPINCode(const string& pinCode)
            {
                _parent->Connector().Control().UserPINCodeReply(Address(), AddressType(), pinCode);
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
                        TRACE(ControlFlow, (_T("Failed to request remote name [%d]"), error));
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
                    ClearState(PAIRING);
                    _linkKeys.Add(lk);
                    if (IsBonded() ^ isBonded) {
                        BondedChange();
                    }
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
            Core::ProxyType<UserRequestJob> _userRequestJob;
            Core::ProxyType<UserReplyJob> _userReplyJob;
        }; // class DeviceRegular

    public:
        class EXTERNAL DeviceLowEnergy : public DeviceImpl, public Exchange::IBluetooth::ILowEnergy {
        public:
            DeviceLowEnergy() = delete;
            DeviceLowEnergy(const DeviceLowEnergy&) = delete;
            DeviceLowEnergy& operator=(const DeviceLowEnergy&) = delete;

            DeviceLowEnergy(BluetoothControl* parent, const uint16_t deviceId, const Bluetooth::Address& address, const Bluetooth::EIR& info)
                : DeviceImpl(parent, Bluetooth::Address::LE_PUBLIC_ADDRESS, deviceId, address, info)
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
                return ( (_ltks.Entries() >= 2) && (_irk.IsValid()) );
            }
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

                    result = Connector().Exchange(MAX_ACTION_TIMEOUT, connect, connect);
                    if (result != Core::ERROR_NONE) {
                        TRACE(ControlFlow, (_T("Failed to connect. Error [%d]"), result));
                    } else {
                        Connection(connect.Response().handle);
                    }
                }

                TRACE(ControlFlow, (_T("DeviceLowEnergy Connect() status <%i>"), result));

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
                    ClearState(PAIRING);
                    _ltks.Add(ltk);
                    if (IsBonded() ^ isBonded) {
                        BondedChange();
                    }
                }
                return (result);
            }
            uint32_t SecurityKey(const Bluetooth::IdentityKey& irk) override
            {
                uint32_t result = Core::ERROR_GENERAL;
                if (irk.IsValid() == true) {
                    result = Core::ERROR_NONE;
                    bool isBonded = IsBonded();
                    ClearState(PAIRING);
                    _irk = irk;
                    if (IsBonded() ^ isBonded) {
                        BondedChange();
                    }
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
        class EXTERNAL Status : public Core::JSON::Container {
        public:
            class EXTERNAL Property : public Core::JSON::Container {
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
        DeviceImpl* Discovered(const bool lowEnergy, const Bluetooth::Address& address, const Bluetooth::EIR& info);
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
        uint32_t endpoint_pair(const JsonData::BluetoothControl::ConnectParamsInfo& params);
        uint32_t endpoint_unpair(const JsonData::BluetoothControl::ConnectParamsInfo& params);
        uint32_t get_adapters(Core::JSON::ArrayType<Core::JSON::DecUInt16>& response) const;
        uint32_t get_adapter(const string& index, JsonData::BluetoothControl::AdapterData& response) const;
        uint32_t get_devices(Core::JSON::ArrayType<Core::JSON::String>& response) const;
        uint32_t get_device(const string& index, JsonData::BluetoothControl::DeviceData& response) const;
        void event_scancomplete();
        void event_devicestatechange(const string& address, const JsonData::BluetoothControl::DevicestatechangeParamsData::DevicestateType& state,
                                     const JsonData::BluetoothControl::DevicestatechangeParamsData::DisconnectreasonType& disconnectreason = JsonData::BluetoothControl::DevicestatechangeParamsData::DisconnectreasonType::TERMINATEDBYHOST);

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
