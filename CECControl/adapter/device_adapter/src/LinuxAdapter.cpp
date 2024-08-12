/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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

#include <cec_device_adapter.h>

#include "Message.h"

#include <linux/cec.h>


using namespace Thunder;

namespace Thunder {
namespace CEC {
    namespace Adapter {
        struct conversion_entry {
            int from; // Local value
            int to; // External value
        };

        static constexpr conversion_entry _table_role_to_LogAdressType[] = {
            { .from = CEC_DEVICE_TV, .to = CEC_LOG_ADDR_TYPE_TV },
            { .from = CEC_DEVICE_RECORDER, .to = CEC_LOG_ADDR_TYPE_RECORD },
            { .from = CEC_DEVICE_TUNER, .to = CEC_LOG_ADDR_TYPE_TUNER },
            { .from = CEC_DEVICE_PLAYBACK, .to = CEC_LOG_ADDR_TYPE_PLAYBACK },
            { .from = CEC_DEVICE_AUDIOSYSTEM, .to = CEC_LOG_ADDR_TYPE_AUDIOSYSTEM },
            { .from = CEC_DEVICE_SWITCH, .to = CEC_LOG_ADDR_TYPE_SPECIFIC },
            { .from = CEC_DEVICE_VIDEOPROCESSOR, .to = CEC_LOG_ADDR_TYPE_SPECIFIC },
            { .from = CEC_DEVICE_UNKNOWN, .to = CEC_LOG_ADDR_TYPE_UNREGISTERED }
        };
        static constexpr conversion_entry _table_role_to_PrimaryDeviceType[] = {
            { .from = CEC_DEVICE_TV, .to = CEC_OP_PRIM_DEVTYPE_TV },
            { .from = CEC_DEVICE_RECORDER, .to = CEC_OP_PRIM_DEVTYPE_RECORD },
            { .from = CEC_DEVICE_TUNER, .to = CEC_OP_PRIM_DEVTYPE_TUNER },
            { .from = CEC_DEVICE_PLAYBACK, .to = CEC_OP_PRIM_DEVTYPE_PLAYBACK },
            { .from = CEC_DEVICE_AUDIOSYSTEM, .to = CEC_OP_PRIM_DEVTYPE_AUDIOSYSTEM },
            { .from = CEC_DEVICE_SWITCH, .to = CEC_OP_PRIM_DEVTYPE_SWITCH },
            { .from = CEC_DEVICE_VIDEOPROCESSOR, .to = CEC_OP_PRIM_DEVTYPE_PROCESSOR }
        };
        static constexpr conversion_entry _table_role_to_AllDeviceType[] = {
            { .from = CEC_DEVICE_TV, .to = CEC_OP_ALL_DEVTYPE_TV },
            { .from = CEC_DEVICE_RECORDER, .to = CEC_OP_ALL_DEVTYPE_RECORD },
            { .from = CEC_DEVICE_TUNER, .to = CEC_OP_ALL_DEVTYPE_TUNER },
            { .from = CEC_DEVICE_PLAYBACK, .to = CEC_OP_ALL_DEVTYPE_PLAYBACK },
            { .from = CEC_DEVICE_AUDIOSYSTEM, .to = CEC_OP_ALL_DEVTYPE_AUDIOSYSTEM },
            { .from = CEC_DEVICE_SWITCH, .to = CEC_OP_ALL_DEVTYPE_SWITCH },
            { .from = CEC_DEVICE_VIDEOPROCESSOR, .to = CEC_OP_ALL_DEVTYPE_SWITCH }
        };

        static constexpr uint8_t max_osd_length = 15;
        static constexpr uint8_t oui_length = 3;

        template <size_t N>
        int Convert(const conversion_entry (&table)[N], const int from, const int ifnotfound)
        {
            uint16_t index = 0;
            while ((index < N) && (from != table[index].from)) {
                index++;
            }
            return (index < N ? table[index].to : ifnotfound);
        }

        class CECAdapterImplementation : public Core::IReferenceCounted {
        private:
            friend Thunder::Core::ProxyObject<CECAdapterImplementation>;

            class Link : public Core::IResource {
            public:
                Link(Link&) = delete;
                Link& operator=(const Link&) = delete;
                Link() = delete;

                Link(CECAdapterImplementation& parent, const string& connector)
                    : _parent(parent)
                    , _connector(connector)
                    , _descriptor(open(connector.c_str(), O_RDWR))
                {
                    ASSERT(IsValid());
                    Core::ResourceMonitor::Instance().Register(*this);
                }

                virtual ~Link()
                {
                    if (IsValid()) {
                        Core::ResourceMonitor::Instance().Unregister(*this);
                        close(_descriptor);
                        _descriptor = 0;
                    }
                }

                Core::IResource::handle Descriptor() const override
                {
                    return _descriptor;
                }

                uint16_t Events() override
                {
                    return (POLLIN | POLLPRI);
                }

                void Handle(const uint16_t events) override
                {
                    if (events & POLLIN) {
                        struct cec_msg msg = {};

                        if (Write(CEC_RECEIVE, &msg) == 0) {
                            string sdata;
                            Core::ToHexString(msg.msg, msg.len, sdata);
                            TRACE(Trace::Information, ("CEC_RECEIVE - rx_status=%02x len=%d addr=%02x opcode=%02x raw=%s", msg.rx_status, msg.len, msg.msg[0], cec_msg_opcode(&msg), sdata.c_str()));

                            _parent.Received(Message(msg.len, msg.msg));
                        }
                    }

                    if (events & POLLPRI) {
                        struct cec_event ev = {};

                        if (Write(CEC_DQEVENT, &ev) == 0) {
                            if (ev.event == CEC_EVENT_STATE_CHANGE) {
                                TRACE(Trace::Information, ("CEC_DQEVENT - CEC_EVENT_STATE_CHANGE - log_addr_mask=0x%04X phys_addr=0x%04X", ev.state_change.log_addr_mask, ev.state_change.phys_addr));
                                _parent.StateChange(ev.state_change.log_addr_mask, ev.state_change.phys_addr, (ev.state_change.have_conn_info > 0));
                            }
                        }
                    }
                }

                cec_adapter_error_t Write(unsigned long int request, void* parameters)
                {
                    errno = 0;
                    cec_adapter_error_t result(HDMI_CEC_ADAPTER_ERROR_OK);

                    if (ioctl(_descriptor, request, parameters) != 0) {
                        TRACE(Trace::Error, ("IOCTL[0x%02X] failed: %s", request, strerror(errno)));
                        result = HDMI_CEC_ADAPTER_ERROR_SYSTEM;
                    }

                    return result;
                }

                const std::string& Connector() const
                {
                    return _connector;
                }

                bool IsValid() const
                {
                    return (_descriptor > 0);
                }

            private:
                CECAdapterImplementation& _parent;
                std::string _connector;
                int _descriptor;
            };

        public:
            CECAdapterImplementation(CECAdapterImplementation&) = delete;
            CECAdapterImplementation& operator=(const CECAdapterImplementation&) = delete;
            CECAdapterImplementation() = delete;

            virtual ~CECAdapterImplementation() = default;

        private:
            uint8_t GetPrivateInfo(const uint8_t opcode, const uint8_t response, const int8_t length, uint8_t* data)
            {
                uint8_t data_to_copy(0);

                if ((data != nullptr) && (length > 0)) {
                    _callbacks.received(_callbackData, CEC_DEVICE_INTERNAL, 0xFF, sizeof(opcode), &opcode);

                    if (_internalBuffer[1] == response) {
                        memset(data, 0, length);

                        data_to_copy = ((_internalBuffer[0] - 1) > 13) ? 13 : (_internalBuffer[0] - 1);
                        memcpy(&data[0], &_internalBuffer[2], data_to_copy);
                        memset(_internalBuffer, 0, sizeof(_internalBuffer));
                    }
                }

                return data_to_copy;
            }

            cec_adapter_error_t UpdateSettings(struct cec_log_addrs* settings)
            {
                ASSERT(settings != nullptr);

                settings->vendor_id = _oui[0] | _oui[1] << 8 | _oui[2] << 16;
                memcpy(&settings->osd_name, _osd_name, (strlen(_osd_name) <= sizeof(settings->osd_name)) ? strlen(_osd_name) : sizeof(settings->osd_name));

                return _link.Write(CEC_ADAP_S_LOG_ADDRS, settings);
            }

        public:
            uint32_t ClaimRole(const cec_adapter_role_t role)
            {
                TRACE(Trace::Information, ("%s adapter[%p], role: 0x%02x [0x%02x]", __FUNCTION__, this, Convert(_table_role_to_LogAdressType, role, uint8_t(~0)), role));
                struct cec_caps caps;
                struct cec_log_addrs settings = {};
                int result(HDMI_CEC_ADAPTER_ERROR_GENERAL);

                // Core::SafeSyncType<Core::CriticalSection> scopedLock(_lock);

                _link.Write(CEC_ADAP_G_CAPS, &caps);
                _link.Write(CEC_ADAP_G_LOG_ADDRS, &settings);

                if (((settings.num_log_addrs < CEC_MAX_LOG_ADDRS) && (settings.num_log_addrs < caps.available_log_addrs))) {

                    settings.primary_device_type[settings.num_log_addrs] = Convert(_table_role_to_PrimaryDeviceType, role, uint8_t(~0));
                    settings.log_addr_type[settings.num_log_addrs] = Convert(_table_role_to_LogAdressType, role, uint8_t(~0));
                    settings.all_device_types[settings.num_log_addrs] = Convert(_table_role_to_AllDeviceType, role, uint8_t(~0));

                    settings.num_log_addrs++;

                    result = UpdateSettings(&settings);
                } else {
                    result = HDMI_CEC_ADAPTER_ERROR_EXHAUSTED;
                    TRACE(Trace::Error, ("%s adapter[%p], No more logical adresses available [%d/%d]", __FUNCTION__, this, settings.num_log_addrs, caps.available_log_addrs));
                }

                TRACE(Trace::Information, ("%s adapter[%p], result: %d", __FUNCTION__, this, result));

                return result;
            }

            uint32_t ReleaseRole(cec_adapter_role_t role)
            {
                cec_adapter_error_t result = HDMI_CEC_ADAPTER_ERROR_NOT_FOUND;

                struct cec_log_addrs settings = {};
                uint8_t i(0);

                _link.Write(CEC_ADAP_G_LOG_ADDRS, &settings);

                while ((result == HDMI_CEC_ADAPTER_ERROR_NOT_FOUND) && (i <= CEC_MAX_LOG_ADDRS)) {
                    if ((settings.primary_device_type[i] == Convert(_table_role_to_PrimaryDeviceType, role, uint8_t(~0))) || (settings.log_addr_type[i] == Convert(_table_role_to_LogAdressType, role, uint8_t(~0))) || (settings.all_device_types[i] == Convert(_table_role_to_AllDeviceType, role, uint8_t(~0)))) {

                        ASSERT(settings.primary_device_type[i] == Convert(_table_role_to_PrimaryDeviceType, role, uint8_t(~0)));
                        ASSERT(settings.log_addr_type[i] == Convert(_table_role_to_LogAdressType, role, uint8_t(~0)));
                        ASSERT(settings.all_device_types[i] == Convert(_table_role_to_AllDeviceType, role, uint8_t(~0)));

                        settings.primary_device_type[i] = 0;
                        settings.log_addr_type[i] = 0;
                        settings.all_device_types[i] = 0;
                        settings.log_addr[i] = ~0;
                        settings.num_log_addrs--;

                        result = UpdateSettings(&settings);
                    } else {
                        ++i;
                    }
                }

                return result;
            }

            uint32_t Transmit(const cec_adapter_role_t initiator, const uint8_t follower, const uint8_t length, const uint8_t data[])
            {
                cec_adapter_error_t result = HDMI_CEC_ADAPTER_ERROR_INVALID_ARGUMENT;
                ASSERT(length < CEC_MAX_MSG_SIZE);

                uint8_t data_size = (length > CEC_MAX_MSG_SIZE) ? CEC_MAX_MSG_SIZE : length;

                if (initiator == CEC_DEVICE_INTERNAL) {

                    memset(_internalBuffer, 0, sizeof(_internalBuffer));

                    _internalBuffer[0] = data_size;

                    memcpy(&_internalBuffer[1], data, data_size);

                    result = HDMI_CEC_ADAPTER_ERROR_OK;
                } else {
                    std::list<uint8_t> initators;
                    GetAdressList(initiator, initators);

                    struct cec_msg msg;
                    int txres(~0);

                    for (auto initator : initators) {
                        cec_msg_init(&msg, initator, follower);

                        memcpy(&msg.msg[1], data, data_size);
                        msg.len += data_size;

                        string sdata;
                        Core::ToHexString(msg.msg, msg.len, sdata);
                        TRACE(Trace::Information, ("raw cec message[0x%02X->0x%02X][operation=0x%02X]: %s", cec_msg_initiator(&msg), cec_msg_destination(&msg), cec_msg_opcode(&msg), sdata.c_str()));

                        if (_link.IsValid()) {
                            txres = _link.Write(CEC_TRANSMIT, &msg);
                        }
                    }

                    if (txres != 0) {
                        result = HDMI_CEC_ADAPTER_ERROR_SYSTEM;
                    } else if (msg.tx_status & CEC_TX_STATUS_OK) {
                        result = HDMI_CEC_ADAPTER_ERROR_OK;
                    } else if (msg.tx_status & CEC_TX_STATUS_NACK) {
                        result = HDMI_CEC_ADAPTER_ERROR_UNAVAILABLE;
                    } else if (msg.tx_status & CEC_TX_STATUS_TIMEOUT) {
                        result = HDMI_CEC_ADAPTER_ERROR_TIMEOUT;
                    } else {
                        result = HDMI_CEC_ADAPTER_ERROR_GENERAL;
                    }
                }

                return result;
            }

            uint32_t Receive(cec_adapter_callbacks_t* callbacks, void* cb_data)
            {
                Core::SafeSyncType<Core::CriticalSection> scopedLock(_lock);

                _callbacks.received = (callbacks != NULL) ? callbacks->received : NULL;
                _callbacks.claimed_roles = (callbacks != NULL) ? callbacks->claimed_roles : NULL;

                _callbackData = cb_data;

                if (_callbacks.received) {
                    GetPrivateInfo(
                        CEC_MSG_GIVE_DEVICE_VENDOR_ID, CEC_MSG_DEVICE_VENDOR_ID,
                        sizeof(_oui), _oui);

                    int osd_length = GetPrivateInfo(
                        CEC_MSG_GIVE_OSD_NAME, CEC_MSG_SET_OSD_NAME,
                        sizeof(_osd_name), reinterpret_cast<uint8_t*>(_osd_name));

                    _osd_name[(osd_length <= 15) ? osd_length : 15] = '\0';
                }

                if (_callbacks.claimed_roles) {
                    struct cec_log_addrs settings = {};
                    _link.Write(CEC_ADAP_G_LOG_ADDRS, &settings);
                    _callbacks.claimed_roles(_callbackData, GetRoles(settings.log_addr_mask));
                }

                return HDMI_CEC_ADAPTER_ERROR_OK;
            }

            static CECAdapterImplementation* Instance(const std::string& connector)
            {
                static std::map<std::string, Core::ProxyType<CECAdapterImplementation>> _adapters;

                auto index = _adapters.find(connector);

                if (index == _adapters.end()) {
                    index = _adapters.emplace(std::piecewise_construct,
                                         std::forward_as_tuple(connector),
                                         std::forward_as_tuple(Core::ProxyType<CECAdapterImplementation>(Core::ProxyType<CECAdapterImplementation>::Create(connector))))
                                .first;
                }

                index->second.AddRef();

                TRACE_GLOBAL(Trace::Information, ("%s adapter[%p]", __FUNCTION__, index->second.operator->()));

                return index->second.operator->();
            }

        private:
            CECAdapterImplementation(const std::string& connector)
                : _lock()
                , _link(*this, connector)
                , _internalBuffer()
                , _callbacks()
                , _callbackData(nullptr)
            {
                if (_link.IsValid()) {
                    // reset adapter
                    struct cec_log_addrs reset;
                    memset(&reset, 0, sizeof(reset));
                    _link.Write(CEC_ADAP_S_LOG_ADDRS, &reset);

                    // set the modes we need to operate on.
                    uint32_t mode;
                    _link.Write(CEC_G_MODE, &mode);
                    mode |= (CEC_MODE_INITIATOR | CEC_MODE_FOLLOWER);
                    _link.Write(CEC_S_MODE, &mode);
                } else {
                    TRACE(Trace::Error, ("Failed to open %s: %s", connector.c_str(), strerror(errno)));
                }
            }

            void Received(const Message& message)
            {
                Core::SafeSyncType<Core::CriticalSection> scopedLock(_lock);
                if (_callbacks.received != nullptr) {
                    cec_adapter_role_t role = Convert(message.Follower());
                    _callbacks.received(_callbackData, role, static_cast<logical_address_t>(message.Initiator()), message.PayloadLength(), message.Payload());
                }
            }

            void StateChange(const uint16_t addressMask, const uint16_t /*physicalAddress*/, const bool /*infoAvalable*/)
            {
                Core::SafeSyncType<Core::CriticalSection> scopedLock(_lock);

                if (_callbacks.claimed_roles != nullptr) {
                    _callbacks.claimed_roles(_callbackData, GetRoles(addressMask));
                }
            }

            void GetAdressList(const cec_adapter_role_t role, std::list<uint8_t>& list)
            {
                bool found(false);
                uint8_t i(0);

                list.clear();

                struct cec_log_addrs currentSettings = {};

                if (_link.Write(CEC_ADAP_G_LOG_ADDRS, &currentSettings) == HDMI_CEC_ADAPTER_ERROR_OK) {
                    while (!found && (i < CEC_MAX_LOG_ADDRS)) {
                        if ((currentSettings.log_addr[i] <= 0xf) && ((role == static_cast<cec_adapter_role_t>(Convert(static_cast<logical_address_t>(currentSettings.log_addr[i])))) || (role == CEC_DEVICE_ALL))) {
                            list.push_back(currentSettings.log_addr[i]);
                            found = (role != CEC_DEVICE_ALL);
                        }
                        if (!found) {
                            ++i;
                        }
                    }
                }
            }

            uint8_t GetRoles(uint16_t addressMask)
            {
                uint8_t roles(0);

                // maybe this can be optimized... :'D
                if (cec_has_tv(addressMask) > 0) {
                    roles |= CEC_DEVICE_TV;
                }
                if (cec_has_record(addressMask) > 0) {
                    roles |= CEC_DEVICE_RECORDER;
                }
                if (cec_has_tuner(addressMask) > 0) {
                    roles |= CEC_DEVICE_TUNER;
                }
                if (cec_has_playback(addressMask) > 0) {
                    roles |= CEC_DEVICE_PLAYBACK;
                }
                if (cec_has_audiosystem(addressMask) > 0) {
                    roles |= CEC_DEVICE_AUDIOSYSTEM;
                }
                if (cec_has_backup(addressMask) > 0) {
                    roles |= CEC_DEVICE_SWITCH;
                }
                if (cec_has_specific(addressMask) > 0) {
                    roles |= CEC_DEVICE_VIDEOPROCESSOR;
                }

                return roles;
            }

        private:
            Core::CriticalSection _lock;
            Link _link;
            uint8_t _internalBuffer[CEC_MAX_MSG_SIZE + 1];
            cec_adapter_callbacks_t _callbacks;
            void* _callbackData;
            uint8_t _oui[oui_length];
            char _osd_name[max_osd_length];
        }; // class CECAdapterImplementation
    } // namespace Adapter
} // namespace CEC
} // namespace Thunder

extern "C" {
cec_adapter_handle_t cec_adapter_create(const char connector[])
{
    return Thunder::CEC::Adapter::CECAdapterImplementation::Instance(connector);
}

cec_adapter_error_t cec_adapter_destroy(cec_adapter_handle_t handle)
{
    ASSERT(handle != nullptr);
    return static_cast<cec_adapter_error_t>(reinterpret_cast<Thunder::CEC::Adapter::CECAdapterImplementation*>(handle)->Release());
}

cec_adapter_error_t cec_adapter_claim_role(cec_adapter_handle_t handle, cec_adapter_role_t role)
{
    ASSERT(handle != nullptr);
    return static_cast<cec_adapter_error_t>(reinterpret_cast<Thunder::CEC::Adapter::CECAdapterImplementation*>(handle)->ClaimRole(role));
}

cec_adapter_error_t cec_adapter_release_role(cec_adapter_handle_t handle, cec_adapter_role_t role)
{
    ASSERT(handle != nullptr);
    return static_cast<cec_adapter_error_t>(reinterpret_cast<Thunder::CEC::Adapter::CECAdapterImplementation*>(handle)->ReleaseRole(role));
}

cec_adapter_error_t cec_adapter_receive(cec_adapter_handle_t handle, cec_adapter_callbacks_t* callbacks, void* cb_data)
{
    ASSERT(handle != nullptr);
    return static_cast<cec_adapter_error_t>(reinterpret_cast<Thunder::CEC::Adapter::CECAdapterImplementation*>(handle)->Receive(callbacks, cb_data));
}

cec_adapter_error_t cec_adapter_transmit(cec_adapter_handle_t handle, const cec_adapter_role_t initiator, const uint8_t follower, const uint8_t length, const uint8_t data[])
{
    ASSERT(handle != nullptr);
    return static_cast<cec_adapter_error_t>(reinterpret_cast<Thunder::CEC::Adapter::CECAdapterImplementation*>(handle)->Transmit(initiator, follower, length, data));
}
} // extern "C"
