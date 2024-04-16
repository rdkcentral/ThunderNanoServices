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

#pragma once

#include "Module.h"

#include <CECIAccessor.h>
#include <CECIAdapterAccessor.h>
#include <CECIDeviceAdapter.h>

#include <CECAdapter.h>
#include <CECMessage.h>
#include <CECOperationFrame.h>
#include <CECTypes.h>

#include <cec_device_adapter.h>
#include <messaging/messaging.h>

#include <string>

namespace Thunder {
namespace CEC {
    class CECAccessor : public IAccessor {
    private:
        CECAccessor() = default;
        ~CECAccessor()
        {
            TRACE(Trace::Information, ("Exit %s", __FUNCTION__));

            for (auto index = _adapters.begin(); index != _adapters.end();) {
                TRACE(Trace::Information, ("Release %s %p", index->first.c_str(), &index->second));
                if ((index->second).IsValid()) {
                    (index->second).Release();
                }
                _adapters.erase(index++);
            }
        }

        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Node()
                , Roles()
            {
                Add(_T("node"), &Node);
                Add(_T("roles"), &Roles);
            }

            ~Config() = default;

        public:
            Core::JSON::String Node;
            Core::JSON::ArrayType<Core::JSON::EnumType<cec_adapter_role_t>> Roles;
        };

        class AdapterImplementation : public IDeviceAdapter {
        private:
            typedef std::unordered_map<cec_adapter_role_t, bool> role_status_map_t;

            uint32_t Convert(cec_adapter_error_t errorCode)
            {
                uint32_t error(Core::ERROR_NOT_SUPPORTED);

                switch (errorCode) {
                case HDMI_CEC_ADAPTER_ERROR_INVALID_ARGUMENT:
                    error = Core::ERROR_INVALID_RANGE;
                    break;
                case HDMI_CEC_ADAPTER_ERROR_GENERAL:
                    error = Core::ERROR_GENERAL;
                    break;
                case HDMI_CEC_ADAPTER_ERROR_UNAVAILABLE:
                    error = Core::ERROR_UNAVAILABLE;
                    break;
                case HDMI_CEC_ADAPTER_ERROR_OK:
                    error = Core::ERROR_NONE;
                    break;
                default:
                    break;
                }

                return error;
            }

        public:
            AdapterImplementation() = delete;
            AdapterImplementation(const AdapterImplementation&) = delete;
            AdapterImplementation& operator=(const AdapterImplementation&) = delete;

        public:
            AdapterImplementation(const Config& config)
                : _adminLock()
                , _adapter(nullptr)
                , _adapterCallbacks()
                , _roles()
                , _observersLock()
                , _observers()
            {
                auto index(config.Roles.Elements());

                while (index.Next() == true) {
                    _roles.emplace(std::piecewise_construct,
                        std::forward_as_tuple(index.Current().Value()),
                        std::forward_as_tuple(false));
                }

                _adapter = cec_adapter_create(config.Node.Value().c_str());

                TRACE(Trace::Information, ("Construct adapter %p for %s", _adapter, config.Node.Value().c_str()));

                ASSERT(_adapter != nullptr);

                _adapterCallbacks.received = AdapterReceived;
                _adapterCallbacks.claimed_roles = AdapterRoles;

                cec_adapter_receive(_adapter, &_adapterCallbacks, this);

                for (auto role : _roles) {
                    cec_adapter_claim_role(_adapter, static_cast<cec_adapter_role_t>(role.first));
                }
            }

        public:
            virtual ~AdapterImplementation()
            {
                TRACE(Trace::Information, ("Exit %s", __FUNCTION__));

                Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

                cec_adapter_receive(_adapter, nullptr, nullptr);

                for (auto role : _roles) {
                    cec_adapter_release_role(_adapter, static_cast<cec_adapter_role_t>(role.first));
                }

                cec_adapter_destroy(_adapter);

                _adapter = nullptr;
            }

            uint32_t Transmit(const cec_adapter_role_t initiator, const logical_address_t follower, const uint8_t length, const uint8_t data[]) override
            {
                cec_adapter_error_t result = cec_adapter_transmit(_adapter, static_cast<cec_adapter_role_t>(initiator), static_cast<uint8_t>(follower), length, data);

                string hexData;
                Core::ToHexString(data, length, hexData);
                TRACE(Trace::Information, ("%p transmitted[result=0x%04X]: \'%s\'", _adapter, result, hexData.c_str()));

                return Convert(result);
            }

            uint32_t Register(INotification* notification) override
            {
                ASSERT(notification != nullptr);

                Core::SafeSyncType<Core::CriticalSection> scopedLock(_observersLock);

                // Make sure a sink is not registered multiple times.
                auto index(std::find(_observers.begin(), _observers.end(), notification));
                ASSERT(index == _observers.end());

                if (index == _observers.end()) {
                    _observers.push_back(notification);
                }
                // notification->AddRef();

                return Core::ERROR_NONE;
            }

            uint32_t Unregister(INotification* notification) override
            {
                ASSERT(notification != nullptr);

                Core::SafeSyncType<Core::CriticalSection> scopedLock(_observersLock);

                auto index(std::find(_observers.begin(), _observers.end(), notification));

                // Make sure you do not unregister something you did not register !!!
                ASSERT(index != _observers.end());

                if (index != _observers.end()) {
                    // (*index)->Release();
                    _observers.erase(index);
                }

                return Core::ERROR_NONE;
            }

            bool IsSupported(const cec_adapter_role_t role) const
            {
                return (_roles.find(role) != _roles.end());
            }

        private:
            static void AdapterReceived(void* cb_data, const cec_adapter_role_t follower, const uint8_t initiator, const uint8_t length, const uint8_t data[])
            {
                AdapterImplementation* implementation = reinterpret_cast<AdapterImplementation*>(cb_data);

                if (implementation != nullptr) {
                    implementation->Received(static_cast<cec_adapter_role_t>(follower), static_cast<logical_address_t>(initiator), length, data);
                }
            }

            static void AdapterRoles(void* cb_data, const uint8_t roles)
            {
                AdapterImplementation* implementation = reinterpret_cast<AdapterImplementation*>(cb_data);

                if (implementation != nullptr) {
                    implementation->UpdateRoles(roles);
                }
            }

            void UpdateRoles(const uint8_t roles)
            {
                TRACE(Trace::Information, ("Claimed Roles 0x%02X", roles));

                for (auto role : _roles) {
                    role.second = ((roles & role.first) > 0);
                }
            }

            void Received(const cec_adapter_role_t follower, const logical_address_t initiator, const uint8_t length, const uint8_t data[])
            {
                OperationFrame msg(length, data);

                string hexData;
                Core::ToHexString(msg.Data(), msg.Size(), hexData);
                TRACE(Trace::Information, ("%p received: %s", _adapter, hexData.c_str()));

                _observersLock.Lock();

                std::list<INotification*>::iterator index = _observers.begin();

                uint8_t result(0);

                // this is for Exchange, for now all adapters receive all incomming data
                while (index != _observers.end()) {
                    result = std::max((*index)->Received(follower, initiator, msg.Size(), msg.Data()), result);
                    index++;
                }

                _observersLock.Unlock();

                // not processed seems like we need to awnser
                if ((result < msg.Size()) && (msg.Size() >= 1)) {
                    bool broadcast(false);

                    Thunder::CEC::Processor::Instance().Process(msg, broadcast);

                    uint32_t result = Transmit(follower, (!broadcast) ? initiator : CEC_LOGICAL_ADDRESS_BROADCAST, msg.Size(), msg.Data());

                    TRACE(Trace::Information, ("Received: Transmit[result=0x%04X]", result));
                }
            }

        private:
            mutable Core::CriticalSection _adminLock;
            cec_adapter_handle_t _adapter;
            cec_adapter_callbacks_t _adapterCallbacks;
            role_status_map_t _roles;
            mutable Core::CriticalSection _observersLock;
            std::list<INotification*> _observers; // Fix/Check lifetime
        };

        friend class Core::SingletonType<CECAccessor>;

    public:
        CECAccessor(const CECAccessor&) = delete;
        CECAccessor& operator=(const CECAccessor&) = delete;

        /**
         * @brief Announce a queriable messages.
         *
         * @param node - A unique id for the CEC device
         * @param config -  JSON config for the device
         * @return  uint32_t possible error codes:
         *  - ERROR_NONE - All went well.
         */
        uint32_t Announce(const string& id, const string& config);

        /**
         * @brief Revoke a queriable message.
         *
         * @param node - A unique id for the CEC device
         * @return  uint32_t possible error codes:
         *  - ERROR_NONE - All went well.
         */
        uint32_t Revoke(const string& id);

        /**
         * @brief Get the Adapter's accessor.
         *
         * @param node - A unique id for the CEC device
         * @return  IAdapterAccessor* or nullptr if error.
         */
        Adapter GetAdapter(const string& id, const cec_adapter_role_t role);

        typedef std::unordered_map<string, Core::ProxyType<AdapterImplementation>> AdapterMap;

    private:
        mutable Core::CriticalSection _adminLock;
        AdapterMap _adapters;
    }; // class CECAccessor
} // namespace CEC
} // namespace Thunder
