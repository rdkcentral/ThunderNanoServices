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
#include "ServiceDiscovery.h"

namespace WPEFramework {

namespace A2DP {

    void ServiceDiscovery::DumpProfile() const
    {
        TRACE(DiscoveryFlow, (_T("Discovered %d service(s)"), _profile.Services().size()));

        uint16_t cnt = 1;
        for (auto const& service : _profile.Services()) {
            TRACE(DiscoveryFlow, (_T("Service #%i"), cnt++));
            TRACE(DiscoveryFlow, (_T("  Handle: 0x%08x"), service.Handle()));

            if (service.Classes().empty() == false) {
                TRACE(DiscoveryFlow, (_T("  Classes:")));
                for (auto const& clazz : service.Classes()) {
                    TRACE(DiscoveryFlow, (_T("    - %s '%s'"),
                                    clazz.Type().ToString().c_str(), clazz.Name().c_str()));
                }
            }
            if (service.Profiles().empty() == false) {
                TRACE(DiscoveryFlow, (_T("  Profiles:")));
                for (auto const& profile : service.Profiles()) {
                    TRACE(DiscoveryFlow, (_T("    - %s '%s', version: %d.%d"),
                                    profile.Type().ToString().c_str(), profile.Name().c_str(),
                                    (profile.Version() >> 8), (profile.Version() & 0xFF)));
                }
            }
            if (service.Protocols().empty() == false) {
                TRACE(DiscoveryFlow, (_T("  Protocols:")));
                for (auto const& protocol : service.Protocols()) {
                    TRACE(DiscoveryFlow, (_T("    - %s '%s', parameters: %s"),
                                    protocol.Type().ToString().c_str(), protocol.Name().c_str(),
                                    Bluetooth::Record(protocol.Parameters()).ToString().c_str()));
                }
            }
            if (service.Attributes().empty() == false) {
                TRACE(DiscoveryFlow, (_T("  Attributes:")));
                for (auto const& attribute : service.Attributes()) {
                    TRACE(DiscoveryFlow, (_T("    - %04x '%s', value: %s"),
                                    attribute.second.Type(), attribute.second.Name().c_str(),
                                    Bluetooth::Record(attribute.second.Value()).ToString().c_str()));
                }
            }
        }
    }

} // namespace A2DP

}
