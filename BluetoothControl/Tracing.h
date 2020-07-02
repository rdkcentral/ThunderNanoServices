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

#include "Module.h"

#ifdef BLUETOOTH_DEVELOPMENT
#define BT_TRACE(flow, ...) TRACE(Tracing::flow, (__VA_ARGS__))
#else
#define BT_TRACE(flow, ...) ((void)0)
#endif


namespace WPEFramework {

namespace Plugin {

    namespace Tracing {

        #define FORMAT_EVENT(evt, ...) FormatEvt(#evt, evt, ##__VA_ARGS__)

        class BluetoothFlow {
        protected:
            BluetoothFlow(const BluetoothFlow&) = delete;
            BluetoothFlow& operator=(const BluetoothFlow&) = delete;
            ~BluetoothFlow() = default;
            BluetoothFlow()
                : _text()
            {
            }

        protected:
            void Format(const TCHAR* format, ...)
            {
                string temp;
                va_list ap;
                va_start(ap, format);
                Trace::Format(temp, format, ap);
                va_end(ap);
                _text += (_text.empty()? "" : ", ") + temp;
            }
            void FormatEvt(const TCHAR* name, const uint16_t opcode)
            {
                Format(_T("Event: %s(0x%03X)"), name, opcode);
            }
            void FormatEvt(const TCHAR* name, const uint16_t opcode, const uint16_t handle)
            {
                FormatEvt(name, opcode);
                Format(_T("handle=%d"), handle);
            }
            void FormatEvt(const TCHAR* name, const uint16_t opcode, const Bluetooth::Address& address)
            {
                FormatEvt(name, opcode);
                Format(_T("address=%s"), address.ToString().c_str());
            }
            void FormatEvt(const TCHAR* name, const uint16_t opcode, const Bluetooth::Address& address, const uint8_t type)
            {
                ASSERT(type < 3);
                Core::EnumerateType<Bluetooth::Address::type> value(static_cast<Bluetooth::Address::type>(type));
                string typeStr = (value.IsSet() == true ? string(value.Data()) : _T("<invalid>"));
                FormatEvt(name, opcode);
                Format(_T("address=%s, type=%s"), address.ToString().c_str(), typeStr.c_str());
            }

        public:
            const TCHAR* Data() const
            {
                return (_text.c_str());
            }
            uint16_t Length() const
            {
                return (static_cast<uint16_t>(_text.length()));
            }

        protected:
            std::string _text;
        };

        class ManagementFlow : public BluetoothFlow {
        public:
            ManagementFlow(const ManagementFlow&) = delete;
            ManagementFlow& operator=(const ManagementFlow&) = delete;
            ~ManagementFlow() = default;

        public:
            ManagementFlow(const TCHAR* format, ...)
            {
                va_list ap;
                va_start(ap, format);
                Trace::Format(_text, format, ap);
                va_end(ap);
            }

        public:
            ManagementFlow(const mgmt_hdr& info, const uint8_t data[])
            {
                FormatEvt(_T("<UNKNOWN MANAGEMENT EVENT>"), btohs(info.opcode));
                uint16_t len = btohs(info.len);
                Format(_T("len=%d"), len);
                if (len > 0) {
                    string dataText;
                    Core::ToHexString(data, len, dataText);
                    Format(_T("data=%s"), dataText.c_str());
                }
            }
            ManagementFlow(const mgmt_ev_cmd_complete& info)
            {
                FORMAT_EVENT(MGMT_EV_CMD_COMPLETE);
                Format(_T("opcode=0x%03X"), btohs(info.opcode));
            }
            ManagementFlow(const mgmt_ev_controller_error& info)
            {
                FORMAT_EVENT(MGMT_EV_CONTROLLER_ERROR);
                Format(_T("error_code=0x%02X"), info.error_code);
            }
            ManagementFlow(const mgmt_ev_auth_failed& info)
            {
                FORMAT_EVENT(MGMT_EV_AUTH_FAILED, info.addr.bdaddr, info.addr.type);
                Format(_T("status=0x%02X"), info.status);
            }
            ManagementFlow(const mgmt_ev_new_conn_param& info)
            {
                FORMAT_EVENT(MGMT_EV_NEW_CONN_PARAM, info.addr.bdaddr, info.addr.type);
                Format(_T("store_hint=%d, min_interval=%d, max_interval=%d, latency=%d, timeout=%d"),
                       info.store_hint, btohs(info.min_interval), btohs(info.max_interval),
                       btohs(info.latency), btohs(info.timeout));
            }
            ManagementFlow(const mgmt_ev_device_connected& info)
            {
                FORMAT_EVENT(MGMT_EV_DEVICE_CONNECTED, info.addr.bdaddr, info.addr.type);
                Format(_T("flags=0x%08x, eir_len=%d"), btohl(info.flags), btohs(info.eir_len));
                if (btohs(info.eir_len) > 0) {
                    string eir;
                    Core::ToHexString(info.eir, btohs(info.eir_len), eir);
                    Format(_T("eir="), eir.c_str());
                }
            }
            ManagementFlow(const mgmt_ev_device_disconnected& info)
            {
                FORMAT_EVENT(MGMT_EV_DEVICE_DISCONNECTED, info.addr.bdaddr, info.addr.type);
                const char* reasonStr;
                switch (info.reason) {
                    case MGMT_DEV_DISCONN_UNKNOWN: reasonStr = "DEV_DISCONN_UNKNOWN"; break;
                    case MGMT_DEV_DISCONN_TIMEOUT: reasonStr = "DEV_DISCONN_TIMEOUT"; break;
                    case MGMT_DEV_DISCONN_LOCAL_HOST: reasonStr = "DEV_DISCONN_LOCAL_HOST"; break;
                    case MGMT_DEV_DISCONN_REMOTE: reasonStr = "DEV_DISCONN_REMOTE"; break;
                    default: reasonStr = "<unknown>"; break;
                }
                Format(_T("reason=0x%02X=%s"), info.reason, reasonStr);
            }
            ManagementFlow(const mgmt_ev_pin_code_request& info)
            {
                FORMAT_EVENT(MGMT_EV_PIN_CODE_REQUEST, info.addr.bdaddr, info.addr.type);
                Format(_T("secure=%d"), info.secure);
            }
            ManagementFlow(const mgmt_ev_user_passkey_request& info)
            {
                FORMAT_EVENT(MGMT_EV_USER_PASSKEY_REQUEST, info.addr.bdaddr, info.addr.type);
            }
            ManagementFlow(const mgmt_ev_user_confirm_request& info)
            {
                FORMAT_EVENT(MGMT_EV_USER_CONFIRM_REQUEST, info.addr.bdaddr, info.addr.type);
                Format(_T("hint=%d, value=%06d"), info.confirm_hint, btohl(info.value));
            }
            ManagementFlow(const mgmt_ev_new_link_key& info)
            {
                FORMAT_EVENT(MGMT_EV_NEW_LINK_KEY, info.key.addr.bdaddr, info.key.addr.type);
                Format(_T("store_hint=%d, key.type=%d, key.pin_len=%d"), info.store_hint, info.key.type, info.key.pin_len);
                if (info.key.pin_len > 0) {
                    string key;
                    Core::ToHexString(info.key.val, std::min(info.key.pin_len, static_cast<uint8_t>(sizeof(info.key.val))), key);
                    Format(_T("key.val=%s"), key.c_str());
                }
            }
            ManagementFlow(const mgmt_ev_new_irk& info)
            {
                FORMAT_EVENT(MGMT_EV_NEW_IRK, info.key.addr.bdaddr, info.key.addr.type);
                string key;
                Core::ToHexString(info.key.val, sizeof(info.key.val), key);
                Format(_T("store_hint=%d, rpa=%s, key.val=%s"), info.store_hint, Bluetooth::Address(info.rpa).ToString().c_str(), key.c_str());
            }
            ManagementFlow(const mgmt_ev_new_csrk& info)
            {
                FORMAT_EVENT(MGMT_EV_NEW_CSRK, info.key.addr.bdaddr, info.key.addr.type);
                string key;
                Core::ToHexString(info.key.val, sizeof(info.key.val), key);
                Format(_T("store_hint=%d, key.type=%d, key.val=%s"), info.store_hint, info.key.type, key.c_str());
            }
            ManagementFlow(const mgmt_ev_new_long_term_key& info)
            {
                FORMAT_EVENT(MGMT_EV_NEW_LONG_TERM_KEY, info.key.addr.bdaddr, info.key.addr.type);
                string key;
                Core::ToHexString(info.key.val, sizeof(info.key.val), key);
                Format(_T("store_hint=%d, key.type=%d, key.master=%d"), info.store_hint, info.key.type, info.key.master);
                Format(_T("key.enc_size=%u, key.ediv=%u, key.rand=%llu, key.val=%s"), info.key.enc_size, btohs(info.key.ediv), btohll(info.key.rand), key.c_str());
            }
        }; // class ManagementFlow

        class ControlFlow : public BluetoothFlow {
        public:
            ControlFlow() = delete;
            ControlFlow(const ControlFlow&) = delete;
            ControlFlow& operator=(const ControlFlow&) = delete;
            ~ControlFlow() = default;

        public:
            ControlFlow(const TCHAR formatter[], ...)
            {
                va_list ap;
                va_start(ap, formatter);
                Trace::Format(_text, formatter, ap);
                va_end(ap);
            }

        public:
            ControlFlow(const hci_event_hdr& info, const uint8_t data[])
            {
                FormatEvt(_T("<UNKNOWN CONTROL EVENT>"), info.evt);
                uint16_t len = btohs(info.plen);
                Format(_T("plen=%d"), len);
                if (len > 0) {
                    string dataText;
                    Core::ToHexString(data, len, dataText);
                    Format(_T("data=%s"), dataText.c_str());
                }
            }
            ControlFlow(const hci_event_hdr& info, const evt_le_meta_event& leinfo)
            {
                Format(_T("<UNKNOWN CONTROL LE EVENT>"), leinfo.subevent);
                uint16_t len = (btohs(info.plen) - sizeof(evt_le_meta_event));
                Format(_T("len=%d"), len);
                if (len > 0) {
                    string dataText;
                    Core::ToHexString(leinfo.data, len, dataText);
                    Format(_T("data=%s"), dataText.c_str());
                }
            }
            ControlFlow(const uint8_t length, const uint8_t vendorData[])
            {
                FORMAT_EVENT(EVT_VENDOR);
                string data;
                Core::ToHexString(vendorData, length, data);
                Format(_T("data=%s"), data.c_str());
            }
            ControlFlow(const evt_cmd_complete& info)
            {
                FORMAT_EVENT(EVT_CMD_COMPLETE);
                const char* group;
                const uint16_t opcode = UnpackOpcode(btohs(info.opcode), &group);
                Format(_T("group=%s, opcode=0x%03X"), group, opcode);
            }
            ControlFlow(const evt_cmd_status& info)
            {
                FORMAT_EVENT(EVT_CMD_COMPLETE);
                const char* group;
                const uint16_t opcode = UnpackOpcode(btohs(info.opcode), &group);
                Format(_T("group=%s, opcode=0x%03X, status=%d"), group, opcode, info.status);
            }
            ControlFlow(const le_advertising_info& info)
            {
                FORMAT_EVENT(EVT_LE_ADVERTISING_REPORT, info.bdaddr, (info.bdaddr_type + 1 /* not for classic devices */));
                const char* typeStr;
                switch (info.evt_type) {
                    case 0: typeStr = "Connectable-undirected"; break;
                    case 1: typeStr = "Connectable-directed"; break;
                    case 2: typeStr = "Scannable-undirected"; break;
                    case 3: typeStr = "Non-connectable-undirected"; break;
                    case 4: typeStr = "Scan-response"; break;
                    default: typeStr = "<unknown>"; break;
                }
                Format(_T("evt_type=%d=%s, length=%d"), info.evt_type, typeStr, info.length);
                if (info.length > 0) {
                    string data;
                    Core::ToHexString(info.data, info.length, data);
                    Format(_T("data=%s"), data.c_str());
                }
            }
            ControlFlow(const extended_inquiry_info& info)
            {
                FORMAT_EVENT(EVT_EXTENDED_INQUIRY_RESULT, info.bdaddr);
                Format(_T("dev_class=0x%06X, pscan_rep_mode=%d, pscan_period_mode=%d, clock_offset=%d, rssi=%d"),
                       UnpackDeviceClass(info.dev_class), info.pscan_rep_mode, info.pscan_period_mode, btohs(info.clock_offset), info.rssi);
            }
            ControlFlow(const inquiry_info_with_rssi& info)
            {
                FORMAT_EVENT(EVT_INQUIRY_RESULT_WITH_RSSI, info.bdaddr);
                Format(_T("dev_class=0x%06X, pscan_rep_mode=%d, pscan_period_mode=%d, clock_offset=%d, rssi=%d"),
                       UnpackDeviceClass(info.dev_class), info.pscan_rep_mode, info.pscan_period_mode, btohs(info.clock_offset), info.rssi);
            }
            ControlFlow(const evt_conn_request& info)
            {
                FORMAT_EVENT(EVT_CONN_REQUEST, info.bdaddr);
                Format(_T("dev_class=0x%06X, link_type=%d"), UnpackDeviceClass(info.dev_class), info.link_type);
            }
            ControlFlow(const evt_conn_complete& info)
            {
                FORMAT_EVENT(EVT_CONN_COMPLETE, info.bdaddr);
                Format(_T("status=%d, handle=%d, link_type=%d, encr_mode=%d"), info.status, btohs(info.handle), info.link_type, info.encr_mode);
            }
            ControlFlow(const evt_disconn_complete& info)
            {
                FORMAT_EVENT(EVT_DISCONN_COMPLETE, btohs(info.handle));
                const char* reasonStr;
                switch (info.reason) {
                    case HCI_AUTHENTICATION_FAILURE: reasonStr = "AUTHENTICATION_FAILURE"; break;
                    case HCI_PIN_OR_KEY_MISSING: reasonStr = "PIN_OR_KEY_MISSING"; break;
                    case HCI_CONNECTION_TIMEOUT: reasonStr = "CONNECTION_TIMEOUT"; break;
                    case HCI_MAX_NUMBER_OF_CONNECTIONS: reasonStr = "MAX_NUMBER_OF_CONNECTIONS"; break;
                    case HCI_OE_USER_ENDED_CONNECTION: reasonStr = "OE_USER_ENDED_CONNECTION"; break;
                    case HCI_OE_LOW_RESOURCES: reasonStr = "OE_LOW_RESOURCES"; break;
                    case HCI_OE_POWER_OFF: reasonStr = "OE_POWER_OFF"; break;
                    case HCI_CONNECTION_TERMINATED: reasonStr = "CONNECTION_TERMINATED"; break;
                    case HCI_INSUFFICIENT_SECURITY: reasonStr = "INSUFFICIENT_SECURITY"; break;
                    default: reasonStr = "<unknown>"; break;
                }
                Format(_T("status=%d, reason=0x%02X=%s"), info.status, info.reason, reasonStr);
            }
            ControlFlow(const evt_le_connection_complete& info)
            {
                FORMAT_EVENT(EVT_LE_CONN_COMPLETE, info.peer_bdaddr, (info.peer_bdaddr_type + 1 /* not for classic devices */));
                Format(_T("status=%d, handle=%d, role=%d, interval=%d, latency=%d, supervision_timeout=%d"),
                       info.status, btohs(info.handle), info.role, btohs(info.interval), btohs(info.latency), btohs(info.supervision_timeout));
            }
            ControlFlow(const evt_le_connection_update_complete& info)
            {
                FORMAT_EVENT(EVT_LE_CONN_UPDATE_COMPLETE, btohs(info.handle));
                Format(_T("status=%d, interval=%d, latency=%d, supervision_timeout=%d"),
                       info.status, btohs(info.interval), btohs(info.latency), btohs(info.supervision_timeout));
            }
            ControlFlow(const evt_remote_name_req_complete& info, const string& name)
            {
                FORMAT_EVENT(EVT_REMOTE_NAME_REQ_COMPLETE, info.bdaddr);
                Format(_T("status=%d, name='%s'"), info.status, name.c_str());
            }
            ControlFlow(const evt_io_capability_request& info)
            {
                FORMAT_EVENT(EVT_IO_CAPABILITY_REQUEST, info.bdaddr);
            }
            ControlFlow(const evt_io_capability_response& info)
            {
                FORMAT_EVENT(EVT_IO_CAPABILITY_RESPONSE, info.bdaddr);
                Format(_T("capability=%d, authentication=%d, oob_data=%d"), info.capability, info.authentication, info.oob_data);
            }
            ControlFlow(const evt_user_confirm_request& info)
            {
                FORMAT_EVENT(EVT_USER_CONFIRM_REQUEST, info.bdaddr);
                Format(_T("passkey=%06d"), btohl(info.passkey));
            }
            ControlFlow(const evt_user_passkey_request& info)
            {
                FORMAT_EVENT(EVT_USER_PASSKEY_REQUEST, info.bdaddr);
            }
            ControlFlow(const evt_user_passkey_notify& info)
            {
                FORMAT_EVENT(EVT_USER_PASSKEY_NOTIFY, info.bdaddr);
                Format(_T("passkey=%06d"), btohl(info.passkey));
            }
            ControlFlow(const evt_read_remote_features_complete& info)
            {
                FORMAT_EVENT(EVT_READ_REMOTE_FEATURES_COMPLETE, btohs(info.handle));
                string features;
                Core::ToHexString(info.features, sizeof(info.features), features);
                Format(_T("status=%d, features=%s"), info.status, features.c_str());
            }
            ControlFlow(const evt_le_read_remote_used_features_complete& info)
            {
                FORMAT_EVENT(EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE, btohs(info.handle));
                string features;
                Core::ToHexString(info.features, sizeof(info.features), features);
                Format(_T("status=%d, features=%s"), info.status, features.c_str());
            }
            ControlFlow(const evt_link_key_req& info)
            {
                FORMAT_EVENT(EVT_LINK_KEY_REQ, info.bdaddr);
            }
            ControlFlow(const evt_link_key_notify& info)
            {
                FORMAT_EVENT(EVT_LINK_KEY_NOTIFY, info.bdaddr);
                string key;
                Core::ToHexString(info.link_key, sizeof(info.link_key), key);
                Format(_T("key_type=%d, key=%s"), info.key_type, key.c_str());
            }
            ControlFlow(const evt_auth_complete& info)
            {
                FORMAT_EVENT(EVT_AUTH_COMPLETE, btohs(info.handle));
                Format(_T("status=%d"), info.status);
            }
            ControlFlow(const evt_simple_pairing_complete& info)
            {
                FORMAT_EVENT(EVT_SIMPLE_PAIRING_COMPLETE, info.bdaddr);
                Format(_T("status=%d"), info.status);
            }
        private:
            static uint32_t UnpackDeviceClass(const uint8_t buffer[3])
            {
                ASSERT(buffer != nullptr);
                return ((buffer[2] << 16) | (buffer[1] << 8) | (buffer[0]));
            }
            static uint16_t UnpackOpcode(uint16_t value, const char** category)
            {
                if (category != nullptr) {
                    switch (cmd_opcode_ogf(value)) {
                        case OGF_LINK_CTL: (*category) = "LINK_CTL"; break;
                        case OGF_LINK_POLICY: (*category) = "LINK_POLICY"; break;
                        case OGF_HOST_CTL: (*category) = "HOST_CTL"; break;
                        case OGF_INFO_PARAM: (*category) = "INFO_PARAM"; break;
                        case OGF_STATUS_PARAM: (*category) = "STATUS_PARAM"; break;
                        case OGF_LE_CTL: (*category) = "LE_CTL"; break;
                        case OGF_TESTING_CMD: (*category) = "TESTING_CMD"; break;
                        case OGF_VENDOR_CMD: (*category) = "VENDOR_CMD"; break;
                        default: (*category) = "<unknown>"; break;
                    }
                }
                return (cmd_opcode_ocf(value));
            }
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
            ~DeviceFlow() = default;

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

        #undef FORMAT_EVENT

    } // namespace Tracing

} // namespace Plugin

}

