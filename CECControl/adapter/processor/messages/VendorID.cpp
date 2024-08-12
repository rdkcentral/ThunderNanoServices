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

// VendorID' are IEEE Organizationally Unique Identifiers
// IDs list http://standards-oui.ieee.org/oui/oui.txt

namespace Thunder {
namespace CEC {
    namespace Message {
        namespace Service {
            class VendorID : public ServiceType<GIVE_DEVICE_VENDOR_ID, DEVICE_VENDOR_ID, VendorID, true> {
                friend ServiceType;

            private:
                static constexpr char EnviroinmentVendorId[] = "PLUGIN_CEC_VENDOR_ID";
                static constexpr uint8_t VendorIdLength = (24 / 8); // 24 bit ID

            public:
                VendorID(const std::string& id)
                    : ServiceType()
                    , _vendorId()
                {
                    ASSERT(id.length() == VendorIdLength * 2);

                    Core::FromHexString(id, _vendorId, sizeof(_vendorId));
                }

                ~VendorID() override = default;

            private:
                uint8_t Process(const uint8_t length VARIABLE_IS_NOT_USED, uint8_t buffer[])
                {
                    ASSERT(length >= sizeof(_vendorId));

                    memcpy(buffer, _vendorId, sizeof(_vendorId));

                    return sizeof(_vendorId);
                }

            private:
                uint8_t _vendorId[3];
            };
        } // VendorID

        static Service::VendorID service_vendor_id("0CF9C0"); // 0C F9 C0 --> BSkyB Ltd. OUI

    } // namespace Message
} // namespace CEC
} // namespace Thunder