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

#include "../include/Module.h"

#include <CECMessage.h>

#include <interfaces/IPower.h>
#include <plugins/Types.h>

namespace Thunder {
namespace RPCLink {
    class Power : protected RPC::SmartInterfaceType<Exchange::IPower> {
    private:
        static constexpr const TCHAR* Callsign = _T("Power");

        using BaseClass = RPC::SmartInterfaceType<Exchange::IPower>;
        friend class Thunder::Core::SingletonType<Power>;

    public:
        Power()
            : BaseClass()
        {
            BaseClass::Open(RPC::CommunicationTimeOut, BaseClass::Connector(), Callsign);
        }

    public:
        Power(const Power&) = delete;
        Power& operator=(const Power&) = delete;

    private:
        void Operational(const bool upAndRunning) override
        {
            if (upAndRunning) {
                if (_power == nullptr) {
                    _power = BaseClass::Interface();
                }
            } else {
                if (_power != nullptr) {
                    _power->Release();
                    _power = nullptr;
                }
            }
        }

    private:
        Exchange::IPower* _power;

    public:
        ~Power()
        {
            if (_power != nullptr) {
                _power->Release();
                _power = nullptr;
            }

            BaseClass::Close(Core::infinite);
        }

        static Power& Instance()
        {
            return Core::SingletonType<Power>::Instance();
        }

    public:
        uint32_t SetPowerState(const Thunder::Exchange::IPower::PCState& state)
        {
            uint32_t errorCode = Core::ERROR_UNAVAILABLE;

            if (_power != nullptr) {
                errorCode = _power->SetState(state, 0);
            }

            return errorCode;
        }
        uint32_t GetPowerState(Exchange::IPower::PCState& outState) const
        {
            uint32_t errorCode = Core::ERROR_UNAVAILABLE;

            if (_power != nullptr) {
                outState = _power->GetState();
                errorCode = Core::ERROR_NONE;
            }

            return errorCode;
        }
    }; // class Power
} // namespace RPCLink

namespace CEC {
    namespace Message {
        namespace Service {
            class Standby : public ServiceType<STANDBY, NO_OPCODE, Standby, false> {
                friend ServiceType;

            public:
                Standby()
                    : ServiceType()
                {
                }

                virtual ~Standby() = default;

            private:
                uint8_t Process(const uint8_t /*length*/, uint8_t /*buffer[]*/)
                {
                    RPCLink::Power::Instance().SetPowerState(Thunder::Exchange::IPower::ActiveStandby);

                    return 0;
                }
            };

            class PowerStatus : public ServiceType<GIVE_DEVICE_POWER_STATUS, REPORT_POWER_STATUS, PowerStatus, false> {
                friend ServiceType;

            public:
                PowerStatus()
                    : ServiceType()
                {
                }

                ~PowerStatus() override = default;

            private:
                uint8_t Process(const uint8_t /*length*/, uint8_t buffer[])
                {
                    Thunder::Exchange::IPower::PCState state = Thunder::Exchange::IPower::PowerOff;

                    RPCLink::Power::Instance().GetPowerState(state);

                    switch (state) {
                    case Thunder::Exchange::IPower::On:
                        buffer[0] = POWER_STATUS_ON;
                        break;
                    case Thunder::Exchange::IPower::ActiveStandby:
                    case Thunder::Exchange::IPower::PassiveStandby:
                    case Thunder::Exchange::IPower::SuspendToRAM:
                    case Thunder::Exchange::IPower::Hibernate:
                        buffer[0] = POWER_STATUS_STANDBY;
                        break;
                    default:
                        buffer[0] = POWER_STATUS_UNKNOWN;
                        break;
                    };

                    return 1;
                }
            };
        } // namespace Response

        static Service::Standby service_standby;
        static Service::PowerStatus service_powerstatus;

    } // namespace Message
} // namespace CEC
} // namespace Thunder