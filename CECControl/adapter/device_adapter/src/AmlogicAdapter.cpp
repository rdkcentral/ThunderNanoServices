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

#include "LogicAddressClaimer.h"
#include "Message.h"

#include <cec_device_adapter.h>
#include <simpleworker/SimpleWorker.h>
#include <sys/ioctl.h>

using namespace Thunder;

namespace Thunder {
namespace CEC {
    namespace Adapter {

        namespace IoCtl {
            constexpr char Magic = 'C';

            constexpr unsigned long int AddLogicalAddress = _IOW(Magic, 0X0B, uint32_t);
            constexpr unsigned long int ClearLogicalAddress = _IOW(Magic, 0X0C, uint32_t);

            constexpr unsigned long int GetPhysicalAddress = _IOR(Magic, 0X00, uint16_t);
            constexpr unsigned long int GetVersion = _IOR(Magic, 0X01, int);
            constexpr unsigned long int GetVendorId = _IOR(Magic, 0X02, uint32_t);
            constexpr unsigned long int GetPortInfo = _IOR(Magic, 0X03, int);
            constexpr unsigned long int GetPortNum = _IOR(Magic, 0X04, int);
            constexpr unsigned long int GetSendFailReason = _IOR(Magic, 0X05, uint32_t);
            constexpr unsigned long int GetConnectStatus = _IOR(Magic, 0X0A, uint32_t);
            constexpr unsigned long int GetBootAddress = _IOW(Magic, 0X10, uint32_t);
            constexpr unsigned long int GetBootReason = _IOW(Magic, 0X11, uint32_t);
            constexpr unsigned long int GetBootPort = _IOW(Magic, 0X13, uint32_t);

            constexpr unsigned long int SetOptionWakeup = _IOW(Magic, 0X06, uint32_t);
            constexpr unsigned long int SetOptionEnalbeCec = _IOW(Magic, 0X07, uint32_t);
            constexpr unsigned long int SetOptionSysControl = _IOW(Magic, 0X08, uint32_t);
            constexpr unsigned long int SetOptionSetLang = _IOW(Magic, 0X09, uint32_t);
            constexpr unsigned long int SetDevType = _IOW(Magic, 0X0D, uint32_t);
            constexpr unsigned long int SetArcEnable = _IOW(Magic, 0X0E, uint32_t);
            constexpr unsigned long int SetAutoDeviceOff = _IOW(Magic, 0X0F, uint32_t);
            constexpr unsigned long int SetFreezeMode = _IOW(Magic, 0X12, uint32_t);
        }
        class CECAdapterImplementation : public LogicAddressClaimer::IAdapter, public Core::IReferenceCounted {
        private:
            friend Thunder::Core::ProxyObject<CECAdapterImplementation>;

            class Link : public Core::IResource {
            private:
                enum WriteError {
                    CEC_FAIL_NONE = 0,
                    CEC_FAIL_NACK = 1,
                    CEC_FAIL_BUSY = 2,
                    CEC_FAIL_OTHER = 3
                };
                typedef std::list<Message> MessageQueue;

            public:
                Link(Link&) = delete;
                Link& operator=(const Link&) = delete;
                Link() = delete;

                Link(CECAdapterImplementation& parent, const string& connector)
                    : _lock()
                    , _parent(parent)
                    , _connector(connector)
                    , _descriptor(open(connector.c_str(), O_RDWR))
                {
                    ASSERT(IsValid());

                    Core::SafeSyncType<Core::CriticalSection> scopedLock(_lock);

                    ioctl(_descriptor, IoCtl::SetOptionEnalbeCec, 0x1);
                    ioctl(_descriptor, IoCtl::SetOptionSysControl, 0x8);

                    Core::ResourceMonitor::Instance().Register(*this);
                }

                virtual ~Link()
                {
                    if (IsValid()) {
                        Core::ResourceMonitor::Instance().Unregister(*this);

                        Core::SafeSyncType<Core::CriticalSection> scopedLock(_lock);

                        ioctl(_descriptor, IoCtl::SetOptionSysControl, 0x0);
                        ioctl(_descriptor, IoCtl::SetOptionEnalbeCec, 0x0);

                        close(_descriptor);
                        _descriptor = 0;
                    }
                }

                bool IsValid() const
                {
                    return (_descriptor > 0);
                }

                cec_adapter_error_t Send(const uint8_t initiator, const uint8_t follower, const uint8_t length, const uint8_t data[])
                {
                    cec_adapter_error_t result(HDMI_CEC_ADAPTER_ERROR_INVALID_ARGUMENT);

                    if (initiator != follower) {
                        Message message(initiator, follower);

                        uint8_t* payloadPtr = message.LockPayload();

                        if (payloadPtr && (length < Message::MaxPayloadLength)) {
                            memcpy(payloadPtr, data, length);
                        }

                        message.UnlockPayload(length);

                        string sdata;
                        Core::ToHexString(message.Data(), message.Size(), sdata);
                        TRACE(Trace::Information, ("Send Data(%d) [%s]", message.Size(), sdata.c_str()));

                        result = Write(message);
                    }

                    return result;
                }

                cec_adapter_error_t ClaimAddress(uint8_t logical_address) const
                {
                    Core::SafeSyncType<Core::CriticalSection> scopedLock(_lock);
                    return (ioctl(_descriptor, IoCtl::AddLogicalAddress, logical_address) >= 0) ? HDMI_CEC_ADAPTER_ERROR_OK : HDMI_CEC_ADAPTER_ERROR_SYSTEM;
                }

                cec_adapter_error_t ReleaseAddress(uint8_t logical_address) const
                {
                    Core::SafeSyncType<Core::CriticalSection> scopedLock(_lock);
                    return (ioctl(_descriptor, IoCtl::ClearLogicalAddress, logical_address) >= 0) ? HDMI_CEC_ADAPTER_ERROR_OK : HDMI_CEC_ADAPTER_ERROR_SYSTEM;
                }

                cec_adapter_error_t PhysicalAddress(uint16_t& pa)
                {
                    Core::SafeSyncType<Core::CriticalSection> scopedLock(_lock);
                    return (ioctl(_descriptor, IoCtl::GetPhysicalAddress, &pa) >= 0) ? HDMI_CEC_ADAPTER_ERROR_OK : HDMI_CEC_ADAPTER_ERROR_SYSTEM;
                }

            private:
                cec_adapter_error_t Write(const Message& msg) const
                {
                    errno = 0;
                    int err = ~0;
                    cec_adapter_error_t result(HDMI_CEC_ADAPTER_ERROR_OK);

                    Core::SafeSyncType<Core::CriticalSection> scopedLock(_lock);

                    if (IsValid() == true) {
                        err = write(_descriptor, msg.Data(), msg.Size());

                        if (err != CEC_FAIL_NONE && err != CEC_FAIL_NACK) {
                            result = HDMI_CEC_ADAPTER_ERROR_GENERAL;
                        } else if (err == CEC_FAIL_NACK) {
                            result = HDMI_CEC_ADAPTER_ERROR_UNAVAILABLE;
                        }
                    } else {
                        result = HDMI_CEC_ADAPTER_ERROR_INVALID_STATE;
                    }

                    return result;
                }

                uint16_t Events() override
                {
                    return (POLLIN | POLLRDNORM);
                }

                void Handle(const uint16_t events) override
                {
                    if ((events & POLLIN || events & POLLRDNORM)) {

                        Message msg;
                        uint8_t length(0);

                        _lock.Lock();

                        if (IsValid() == true) {
                            length = read(_descriptor, msg.LockRaw(), 1);
                        }

                        _lock.Unlock();

                        msg.UnlockRaw(length);

                        if (length > 0) {
                            _parent.Received(msg);
                        }
                    }
                }

                Core::IResource::handle Descriptor() const override
                {
                    return _descriptor;
                }

            private:
                mutable Core::CriticalSection _lock;
                CECAdapterImplementation& _parent;
                std::string _connector;
                int _descriptor;
            };

            class Sink : public LogicAddressClaimer::ICallback {
            public:
                Sink() = delete;
                Sink(const Sink&) = delete;
                Sink& operator=(const Sink&) = delete;

                Sink(CECAdapterImplementation& parent)
                    : _parent(parent)
                {
                }

                virtual ~Sink() = default;

            public:
                void Finished(const LogicAddressClaimer* claimer)
                {
                    _parent.Finished(claimer);
                }

            private:
                CECAdapterImplementation& _parent;
            };

            struct hdmi_port_info {
                int type;
                /* Port ID should start from 1 which corresponds to HDMI "port 1". */
                int port_id;
                int cec_supported;
                int arc_supported;
                uint16_t physical_address;
            };

        public:
            CECAdapterImplementation(CECAdapterImplementation&) = delete;
            CECAdapterImplementation& operator=(const CECAdapterImplementation&) = delete;
            CECAdapterImplementation() = delete;

            virtual ~CECAdapterImplementation()
            {
                TRACE(Trace::Information, ("%s Amlogic exits..", __FUNCTION__, __TIMESTAMP__));
            };

        public:
            uint32_t ClaimRole(const cec_adapter_role_t role)
            {
                uint32_t result = HDMI_CEC_ADAPTER_ERROR_GENERAL;

                Core::SafeSyncType<Core::CriticalSection> scopedLock(_lock);

                auto index = _devices.find(role);

                if (index == _devices.end()) {
                    TRACE(Trace::Information, ("%s adapter[%p], role: 0x%02x", __FUNCTION__, this, role));

                    _devices.emplace(std::piecewise_construct,
                        std::forward_as_tuple(role),
                        std::forward_as_tuple(CEC_LOGICAL_ADDRESS_UNREGISTERED));

                    _claims.emplace_back(role, this, _sink);

                    result = HDMI_CEC_ADAPTER_ERROR_OK;
                } else {
                    TRACE(Trace::Error, ("%s adapter[%p], skipping role: 0x%02x, already claimed or claim in progress.", __FUNCTION__, this, role));
                }

                return result;
            }

            uint32_t ReleaseRole(cec_adapter_role_t role)
            {
                uint32_t result = HDMI_CEC_ADAPTER_ERROR_NOT_FOUND;

                Core::SafeSyncType<Core::CriticalSection> scopedLock(_lock);

                auto index = _devices.find(role);

                if (index != _devices.end()) {
                    uint32_t r = _link.ReleaseAddress(index->second);
                    _devices.erase(index);

                    TRACE(Trace::Information, ("%s adapter[%p], role: 0x%02x released[%d]", __FUNCTION__, this, role, r));

                    UpdateRoles();

                    result = HDMI_CEC_ADAPTER_ERROR_OK;
                } else {
                    TRACE(Trace::Error, ("%s adapter[%p], skipping role: 0x%02x, not claimed.", __FUNCTION__, this, role));
                }

                return result;
            }

            uint32_t Transmit(const cec_adapter_role_t role, const uint8_t follower, const uint8_t length, const uint8_t data[])
            {
                ASSERT(length < Message::MaxPayloadLength);

                uint32_t result(HDMI_CEC_ADAPTER_ERROR_OK);

                if (role == CEC_DEVICE_ALL) {
                    role_map_t::const_iterator index = _devices.cbegin();

                    while ((index != _devices.cend()) && (result == HDMI_CEC_ADAPTER_ERROR_OK)) {
                        result = _link.Send(index->second, follower, length, data);
                        index++;
                    }
                } else {
                    result = _link.Send(LogicalAddress(role), follower, length, data);
                }

                return result;
            }

            uint32_t Receive(cec_adapter_callbacks_t* callbacks, void* cb_data)
            {
                Core::SafeSyncType<Core::CriticalSection> scopedLock(_lock);

                _callbacks.received = (callbacks != NULL) ? callbacks->received : NULL;
                _callbacks.claimed_roles = (callbacks != NULL) ? callbacks->claimed_roles : NULL;

                _callbackData = cb_data;

                UpdateRoles();

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

                TRACE(Trace::Information, ("%s adapter[%p]", __FUNCTION__, index->second.operator->()));

                return index->second.operator->();
            }

        private:
            CECAdapterImplementation(const std::string& connector)
                : _lock()
                , _link(*this, connector)
                , _sink(*this)
                , _callbacks()
                , _callbackData(nullptr)
                , _claims()
                , _devices()
            {
                TRACE(Trace::Information, ("Amlogic %s build: %s", __FUNCTION__, __TIMESTAMP__));

                if (_link.IsValid() == false) {
                    TRACE(Trace::Error, ("Failed to open %s: %s", connector.c_str(), strerror(errno)));
                }
            }

            void Received(const Message& message)
            {
                Core::SafeSyncType<Core::CriticalSection> scopedLock(_lock);

                string sdata;
                Core::ToHexString(message.Data(), message.Size(), sdata);
                TRACE(Trace::Information, ("Received Data(%d) [%s]", message.Size(), sdata.c_str()));

                cec_adapter_role_t role = static_cast<cec_adapter_role_t>(Convert(message.Follower()));

                if ((message.Size() == 2) && (message.Data()[1] == 0x83)) {
                    ReportPhysicalAddress(LogicalAddress(role));
                } else {
                    if (_callbacks.received != nullptr) {
                        _callbacks.received(_callbackData, role, static_cast<logical_address_t>(message.Initiator()), message.PayloadLength(), message.Payload());
                    }
                }
            }

            void
            Finished(const LogicAddressClaimer* claimer)
            {
                ASSERT(claimer != nullptr);

                if (claimer != nullptr) {
                    TRACE(Trace::Information, ("Got address 0x%02X for role 0x%02X", claimer->LogicalAddress(), claimer->Device()));

                    Core::SafeSyncType<Core::CriticalSection> scopedLock(_lock);

                    auto device = _devices.find(claimer->Device());

                    if ((device != _devices.end()) && _link.ClaimAddress(claimer->LogicalAddress()) == HDMI_CEC_ADAPTER_ERROR_OK) {
                        device->second = claimer->LogicalAddress();
                        TRACE(Trace::Information, ("Finished set address 0x%02X", device->second));
                        UpdateRoles();
                    }

                    if (device == _devices.end()) {
                        TRACE(Trace::Error, ("no device found... :-( [device: 0x%02X  la: 0x%02X]", claimer->Device(), claimer->LogicalAddress()));
                    }
                }

                auto claim = _claims.cbegin();
                while (claim != _claims.cend()) {
                    auto current = claim++;
                    if (claimer == current.operator->()) {
                        _claims.erase(current);
                    }
                }
            }

            logical_address_t LogicalAddress(const cec_adapter_role_t role) const
            {
                Core::SafeSyncType<Core::CriticalSection> scopedLock(_lock);

                role_map_t::const_iterator index = _devices.find(role);

                TRACE(Trace::Information, ("LogicalAddress for 0x%02X: 0x%02X", (index != _devices.cend()) ? index->first : 0xFF, (index != _devices.cend()) ? index->second : 0xFF));

                return (index != _devices.cend()) ? index->second : CEC_LOGICAL_ADDRESS_INVALID;
            }

            void UpdateRoles() const
            {
                auto index = _devices.cbegin();

                uint8_t rolesMask(0);

                while (index != _devices.cend()) {
                    rolesMask |= (index->second < CEC_LOGICAL_ADDRESS_UNREGISTERED) ? index->first : 0;
                    index++;
                }

                TRACE(Trace::Information, ("UpdateRoles mask=0x%02X", rolesMask));

                if (_callbacks.claimed_roles) {
                    _callbacks.claimed_roles(_callbackData, rolesMask);
                }
            }

            uint32_t ReportPhysicalAddress(const logical_address_t initiator)
            {
                uint8_t payload[5];

                payload[0] = 0x84;

                uint16_t pa(~0);
                cec_adapter_error_t paResult = _link.PhysicalAddress(pa);

                memcpy(&payload[1], &pa, 2);

                return (paResult == HDMI_CEC_ADAPTER_ERROR_OK) ? _link.Send(initiator, 0x0f, sizeof(payload), payload) : HDMI_CEC_ADAPTER_ERROR_SYSTEM;
            }

        private:
            mutable Core::CriticalSection _lock;
            Link _link;
            Sink _sink;
            cec_adapter_callbacks_t _callbacks;
            void* _callbackData;
            std::list<LogicAddressClaimer> _claims;
            role_map_t _devices;
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
