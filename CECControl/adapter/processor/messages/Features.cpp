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

/*
The <Report Features> message is used by a device to broadcast its features: a combination of its CEC version,
the collection of Device Types in the device (operand [All Device Types]) and several other characteristics
(operands [RC Profile] and [Device Features], see Section 11.6.4 and 11.2.2). It shall be sent by a device
during/after the logical address allocation (see Section 11.3.3), and when requested by another device (this
request can be done with a <Give Features> message to a particular device).

When a device makes such updates that one or more of the operands of the <Report Features> message change value,
it shall broadcast a <Report Features> message with the up-to-date operand values.
*/

namespace Thunder {
namespace CEC {
    namespace Message {
        namespace Service {
            class Features : public ServiceType<REPORT_FEATURES, GIVE_FEATURES, Features, true> {
                friend ServiceType;
                /*
                The CEC version, flags on certain features and all the device types of a device.
                Note operands [RC Profile] and [Device Features] are variable length
                */
                class FeaturesFrame : public Core::FrameType<0, false, uint8_t> {
                private:
                    using BaseClass = Core::FrameType<0, false, uint8_t>;

                public:
                    static constexpr uint8_t MaxLength = 14;
                    static constexpr uint8_t MinLength = 14;

                public:
                    FeaturesFrame(FeaturesFrame&) = delete;
                    FeaturesFrame& operator=(const FeaturesFrame&) = delete;

                    FeaturesFrame()
                        : BaseClass(_buffer, sizeof(_buffer), 0)
                    {
                        memset(_buffer, 0, sizeof(_buffer));
                        BaseClass::Size(MinLength);
                    }

                    ~FeaturesFrame() = default;

                public:
                    version_t Version() const
                    {
                        return static_cast<version_t>(BaseClass::Data()[0]);
                    }

                    void Version(version_t version)
                    {
                        BaseClass::Writer writer(*this, 0);
                        writer.Number(version);
                    }

                    uint8_t DeviceTypes() const
                    {
                        return (BaseClass::Data()[1]);
                    }

                    void AddDevice(uint8_t deviceMask)
                    {
                        BaseClass::Writer writer(*this, 1);
                        writer.Number(opcode);
                    }

                    void ClearDevice(uint8_t deviceMask)
                    {
                        if (opcode != NO_OPCODE) {
                            BaseClass::Writer writer(*this, 1);
                            writer.Number(opcode);
                        }
                    }

                private:
                    uint8_t _buffer[14];
                };

            public:
                Features()
                    : ServiceType()
                    , _features()
                {
                }

                ~Features() override = default;

            private:
                uint8_t Process(const uint8_t length, uint8_t buffer[])
                {
                    ASSERT(_features.Size() <= length);

                    if (_features.Size() <= length) {
                        memcpy(buffer, _features.Data(), _features.Size());
                    }

                    return (_features.Size() <= length) ? _features.Size() : 0;
                }

            private:
                FeaturesFrame _features;
            }; // class Features
        } // namespace Service

        static Service::Features service_features;
    } // namespace Message
} // namespace CEC
} // namespace Thunder