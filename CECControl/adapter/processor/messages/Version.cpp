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

namespace Thunder {
namespace CEC {
    namespace Message {
        namespace Service {
            class Version : public ServiceType<GET_CEC_VERSION, CEC_VERSION, Version, false> {
                friend ServiceType;

            public:
                Version(version_t version)
                    : ServiceType()
                    , _version(version)
                {
                }

                ~Version() override = default;

            private:
                uint8_t Process(const uint8_t /*length*/, uint8_t buffer[])
                {
                    buffer[0] = _version;
                    return 1;
                }

            private:
                version_t _version;
            };
        } // namespace Response

        static Service::Version service_version(VERSION_CEC_2_0);

    } // namespace Message
} // namespace CEC
} // namespace Thunder
