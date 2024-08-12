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

#include <nexus_config.h>

#include <berr.h>
#include <bkni.h>
#include <bstd.h>
#include <nexus_cec.h>
#include <nxclient.h>

namespace {
std::string ClientName()
{
    std::string name;

    name = (EXPAND_AND_QUOTE(MODULE_NAME) + (_T("_") + Thunder::Core::ToString(Thunder::Core::Time::Now().Ticks() / Thunder::Core::Time::TicksPerMillisecond)));

    return (name);
}
}

namespace Thunder {
namespace CEC {
    namespace Adapter {
        struct conversion_entry {
            int from; // Local value
            int to; // External value
        };

        static constexpr conversion_entry _table_role_to_NEXUS_CecDeviceType[] = {
            { .from = CEC_DEVICE_TV, .to = NEXUS_CecDeviceType_eTv },
            { .from = CEC_DEVICE_RECORDER, .to = NEXUS_CecDeviceType_eRecordingDevice },
            { .from = CEC_DEVICE_TUNER, .to = NEXUS_CecDeviceType_eTuner },
            { .from = CEC_DEVICE_PLAYBACK, .to = NEXUS_CecDeviceType_ePlaybackDevice },
            { .from = CEC_DEVICE_AUDIOSYSTEM, .to = NEXUS_CecDeviceType_eAudioSystem },
            { .from = CEC_DEVICE_SWITCH, .to = NEXUS_CecDeviceType_ePureCecSwitch },
            { .from = CEC_DEVICE_VIDEOPROCESSOR, .to = NEXUS_CecDeviceType_eVideoProcessor },
            { .from = CEC_DEVICE_UNKNOWN, .to = NEXUS_CecDeviceType_eMax }
        };

        static constexpr uint16_t TxMaxTimeout = 1500;

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

        public:
            CECAdapterImplementation(CECAdapterImplementation&) = delete;
            CECAdapterImplementation& operator=(const CECAdapterImplementation&) = delete;
            CECAdapterImplementation() = delete;

            virtual ~CECAdapterImplementation()
            {
                TRACE(Thunder::Trace::Information, (_T("CEC adapter detaching from Nexus...")));

                _txSignal.SetEvent();

                NxClient_Uninit();
            }

        private:
        public:
            uint32_t ClaimRole(const cec_adapter_role_t role)
            {
                TRACE(Trace::Information, ("%s adapter[%p], role: 0x%02x [0x%02x]", __FUNCTION__, this, Convert(_table_role_to_NEXUS_CecDeviceType, role, uint8_t(~0)), role));

                int result(HDMI_CEC_ADAPTER_ERROR_GENERAL);

                NEXUS_CecSettings cecSettings;

                NEXUS_Cec_GetSettings(_cecHandle, &cecSettings);

                if (cecSettings.deviceType == NEXUS_CecDeviceType_eMax) {
                    cecSettings.enabled = true;
                    cecSettings.deviceType = static_cast<NEXUS_CecDeviceType>(Convert(_table_role_to_NEXUS_CecDeviceType, role, uint8_t(~0)));

                    if (NEXUS_Cec_SetSettings(_cecHandle, &cecSettings) == NEXUS_SUCCESS) {
                        result = HDMI_CEC_ADAPTER_ERROR_OK;
                    } else {
                        result = HDMI_CEC_ADAPTER_ERROR_SYSTEM;
                    }
                } else {
                    result = HDMI_CEC_ADAPTER_ERROR_EXHAUSTED;
                }

                TRACE(Trace::Information, ("%s adapter[%p], result: %d", __FUNCTION__, this, result));

                return result;
            }

            uint32_t ReleaseRole(cec_adapter_role_t role)
            {
                cec_adapter_error_t result = HDMI_CEC_ADAPTER_ERROR_NOT_FOUND;

                NEXUS_CecSettings cecSettings;
                NEXUS_Cec_GetSettings(_cecHandle, &cecSettings);

                if (cecSettings.deviceType == static_cast<NEXUS_CecDeviceType>(Convert(_table_role_to_NEXUS_CecDeviceType, role, uint8_t(~0)))) {
                    cecSettings.enabled = false;
                    cecSettings.deviceType = NEXUS_CecDeviceType_eMax;

                    if (NEXUS_Cec_SetSettings(_cecHandle, &cecSettings) == NEXUS_SUCCESS) {
                        result = HDMI_CEC_ADAPTER_ERROR_OK;
                    } else {
                        result = HDMI_CEC_ADAPTER_ERROR_SYSTEM;
                    }
                }

                return result;
            }

            uint32_t Transmit(const cec_adapter_role_t /*initiator*/, const uint8_t follower, const uint8_t length, const uint8_t data[])
            {
                cec_adapter_error_t result = HDMI_CEC_ADAPTER_ERROR_INVALID_ARGUMENT;

                NEXUS_CecStatus cecStatus;

                if (NEXUS_Cec_GetStatus(_cecHandle, &cecStatus) == NEXUS_SUCCESS) {

                    ASSERT(length < Message::MaxLength);

                    if (length < Message::MaxLength) {
                        if (cecStatus.logicalAddress != 0xFF) {
                            NEXUS_CecMessage transmitMessage;

                            NEXUS_Cec_GetDefaultMessageData(&transmitMessage);

                            transmitMessage.destinationAddr = follower;
                            transmitMessage.initiatorAddr = cecStatus.logicalAddress;
                            transmitMessage.length = length;
                            memcpy(transmitMessage.buffer, data, length);

                            string msgBuffer;
                            Core::ToHexString(transmitMessage.buffer, transmitMessage.length, msgBuffer);
                            TRACE(Trace::Information, ("Transmit Message InitiatorAddr=0x%02X, DestinationAddr=0x%02X, Length=%d Transmit=\'%s\'", transmitMessage.initiatorAddr, transmitMessage.destinationAddr, transmitMessage.length, msgBuffer.c_str()));

                            if (NEXUS_Cec_TransmitMessage(_cecHandle, &transmitMessage) == NEXUS_SUCCESS) {
                                uint32_t tx = _txSignal.Lock(_duration);

                                if (tx == Core::ERROR_TIMEDOUT) {
                                    result = HDMI_CEC_ADAPTER_ERROR_TIMEOUT;
                                } else if (tx == Core::ERROR_NONE) {
                                    NEXUS_CecStatus status;
                                    NEXUS_Cec_GetStatus(_cecHandle, &status);

                                    TRACE(Trace::Information, ("Transmit Message Status for Phys/Logical Addrs: %X.%X.%X.%X / %d", (status.physicalAddress[0] & 0xF0) >> 4, (status.physicalAddress[0] & 0x0F), (status.physicalAddress[1] & 0xF0) >> 4, (status.physicalAddress[1] & 0x0F), status.logicalAddress));
                                    TRACE(Trace::Information, ("Transmit Message Acknowledged: %s", status.transmitMessageAcknowledged ? "Yes" : "No"));
                                    TRACE(Trace::Information, ("Transmit Message Pending: %s", status.messageTransmitPending ? "Yes" : "No"));

                                    // Awnser a ping to the upper layers if possible.... nexus whats to keep if for it's self.
                                    // if ((_callbacks.received != nullptr) && (transmitMessage.length == 0) && (status.transmitMessageAcknowledged)) {
                                    //     cec_adapter_role_t role = static_cast<cec_adapter_role_t>(Convert(static_cast<logical_address_t>(transmitMessage.initiatorAddr)));
                                    //     _callbacks.received(_callbackData, role, static_cast<logical_address_t>(transmitMessage.destinationAddr), transmitMessage.length, transmitMessage.buffer);
                                    // }

                                    result = status.transmitMessageAcknowledged ? HDMI_CEC_ADAPTER_ERROR_OK : HDMI_CEC_ADAPTER_ERROR_UNAVAILABLE;
                                } else {
                                    result = HDMI_CEC_ADAPTER_ERROR_GENERAL;
                                }

                                _txSignal.ResetEvent();
                            } else {
                                result = HDMI_CEC_ADAPTER_ERROR_SYSTEM;
                            }
                        } else {
                            result = HDMI_CEC_ADAPTER_ERROR_UNSUPORTED;
                        }
                    }
                } else {
                    result = HDMI_CEC_ADAPTER_ERROR_SYSTEM;
                }

                TRACE(Trace::Information, ("Transmit result=0x%04X", result));

                return result;
            }

            uint32_t Receive(cec_adapter_callbacks_t* callbacks, void* cb_data)
            {
                Core::SafeSyncType<Core::CriticalSection> scopedLock(_lock);

                _callbacks.received = (callbacks != NULL) ? callbacks->received : NULL;
                _callbacks.claimed_roles = (callbacks != NULL) ? callbacks->claimed_roles : NULL;

                _callbackData = cb_data;

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

            void MessageReceived()
            {
                NEXUS_Error rc;
                NEXUS_CecStatus status;
                NEXUS_CecReceivedMessage receivedMessage;

                NEXUS_Cec_GetStatus(_cecHandle, &status);

                rc = NEXUS_Cec_ReceiveMessage(_cecHandle, &receivedMessage);

                if (rc == NEXUS_SUCCESS) {
                    string msgBuffer;
                    Core::ToHexString(receivedMessage.data.buffer, receivedMessage.data.length, msgBuffer);

                    TRACE(Trace::Information, ("Message Received: %s", status.messageReceived ? "Yes" : "No"));
                    TRACE(Trace::Information, ("Message Received Status from Phys/Logical Addrs: %X.%X.%X.%X / %d", (status.physicalAddress[0] & 0xF0) >> 4, (status.physicalAddress[0] & 0x0F), (status.physicalAddress[1] & 0xF0) >> 4, (status.physicalAddress[1] & 0x0F), status.logicalAddress));
                    TRACE(Trace::Information, ("Message Received InitiatorLogicalAddr=0x%02X, DestinationLogicalAddr=0x%02X, Length=%d Received=%s", receivedMessage.data.initiatorAddr, receivedMessage.data.destinationAddr, receivedMessage.data.length, msgBuffer.c_str()));

                    Core::SafeSyncType<Core::CriticalSection> scopedLock(_lock);

                    if (_callbacks.received != nullptr) {
                        cec_adapter_role_t role = Convert(static_cast<logical_address_t>(receivedMessage.data.destinationAddr)));
                        _callbacks.received(_callbackData, role, static_cast<logical_address_t>(receivedMessage.data.initiatorAddr), receivedMessage.data.length, receivedMessage.data.buffer);
                    }
                }
            }

            void MessageTransmitted()
            {
                _txSignal.SetEvent();
            }

            void DeviceReady()
            {
                NEXUS_CecStatus status;

                /* print informative information */
                NEXUS_Cec_GetStatus(_cecHandle, &status);

                TRACE(Trace::Information, ("BCM%d Logical Address <%d> Acquired, CEC Version: 0x%02X", BCHP_CHIP, status.logicalAddress, status.cecVersion));

                TRACE(Trace::Information, ("BCM%d Physical Address: %X.%X.%X.%X", BCHP_CHIP, (status.physicalAddress[0] & 0xF0) >> 4, (status.physicalAddress[0] & 0x0F), (status.physicalAddress[1] & 0xF0) >> 4, (status.physicalAddress[1] & 0x0F)));
            }

        private:
            CECAdapterImplementation(const std::string& connector)
                : _lock()
                , _callbacks()
                , _callbackData(nullptr)
                , _connector(~0)
                , _txSignal(false, true)
                , _duration(TxMaxTimeout)
            {
                NEXUS_Error rc;
                NxClient_JoinSettings joinSettings;
                NxClient_GetDefaultJoinSettings(&joinSettings);

                _txSignal.ResetEvent();

                _connector = Core::NumberType<uint8_t, false, BASE_DECIMAL>(Core::TextFragment(connector)).Value();

                snprintf(joinSettings.name, NXCLIENT_MAX_NAME, _T("%s"), ClientName().c_str());

                TRACE(Trace::Information, (_T("Connect CEC %s on index %d"), joinSettings.name, _connector));

                rc = NxClient_Join(&joinSettings);

                if (rc != NEXUS_SUCCESS) {
                    TRACE(Trace::Error, (_T("Failed to join Nexus [%08x]"), rc));
                } else {
                    TRACE(Trace::Information, (_T("CEC adapter joined Nexus successfully")));

                    NEXUS_CecSettings cecSettings;
                    NEXUS_Cec_GetDefaultSettings(&cecSettings);

                    _cecHandle = NEXUS_Cec_Open(_connector, &cecSettings);

                    if (_cecHandle) {
                        NEXUS_Cec_GetSettings(_cecHandle, &cecSettings);

                        cecSettings.deviceType = NEXUS_CecDeviceType_eMax;

                        cecSettings.messageReceivedCallback.callback = msgReceivedCallback;
                        cecSettings.messageReceivedCallback.context = this;

                        cecSettings.messageTransmittedCallback.callback = msgTransmittedCallback;
                        cecSettings.messageTransmittedCallback.context = this;

                        cecSettings.logicalAddressAcquiredCallback.callback = deviceReadyCallback;
                        cecSettings.logicalAddressAcquiredCallback.context = this;

                        rc = NEXUS_Cec_SetSettings(_cecHandle, &cecSettings);
                    }
                }
            }

            static void msgTransmittedCallback(void* context, int param)
            {
                BSTD_UNUSED(param);
                CECAdapterImplementation* adapter = reinterpret_cast<CECAdapterImplementation*>(context);

                if (adapter) {
                    adapter->MessageTransmitted();
                }
            }

            static void msgReceivedCallback(void* context, int param)
            {
                BSTD_UNUSED(param);
                CECAdapterImplementation* adapter = reinterpret_cast<CECAdapterImplementation*>(context);

                if (adapter) {
                    adapter->MessageReceived();
                }
            }

            static void deviceReadyCallback(void* context, int param)
            {
                BSTD_UNUSED(param);
                CECAdapterImplementation* adapter = reinterpret_cast<CECAdapterImplementation*>(context);

                if (adapter) {
                    adapter->DeviceReady();
                }
            }

        private:
            Core::CriticalSection _lock;
            cec_adapter_callbacks_t _callbacks;
            void* _callbackData;
            NEXUS_CecHandle _cecHandle;
            uint8_t _connector;
            Core::Event _txSignal;
            uint32_t _duration;
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
