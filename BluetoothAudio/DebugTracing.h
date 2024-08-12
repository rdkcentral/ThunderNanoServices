/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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

namespace Thunder {

namespace Plugin {

    DEFINE_MESSAGING_CATEGORY(Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TRACING>, SinkFlow)
    DEFINE_MESSAGING_CATEGORY(Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TRACING>, SourceFlow)
    DEFINE_MESSAGING_CATEGORY(Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TRACING>, ServerFlow)

    DEFINE_MESSAGING_CATEGORY(Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TRACING>, DiscoveryFlow)
    DEFINE_MESSAGING_CATEGORY(Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TRACING>, SignallingFlow)
    DEFINE_MESSAGING_CATEGORY(Messaging::BaseCategoryType<Core::Messaging::Metadata::type::TRACING>, TransportFlow)

    template<typename FLOW>
    void Dump(const Bluetooth::AVDTP::StreamEndPoint& sep)
    {
        TRACE_GLOBAL(FLOW, (_T("%s"), sep.AsString().c_str()));

        if (sep.Capabilities().empty() == false) {
            TRACE_GLOBAL(FLOW, (_T("Capabilities:")));

            for (auto const& caps : sep.Capabilities()) {
                TRACE_GLOBAL(FLOW, (_T(" - %s"), caps.second.AsString().c_str()));
            }
        }
    }

    template<typename FLOW>
    void Dump(const Bluetooth::SDP::Tree& tree)
    {
        using namespace Bluetooth::SDP;

        uint16_t cnt = 1;
        for (auto const& service : tree.Services()) {
            string name = _T("<unknown>");
            string description;
            string provider;

            if (service.Metadata(name, description, provider, _T("en"), CHARSET_US_ASCII) == false) {
                service.Metadata(name, description, provider, _T("en"), CHARSET_UTF8);
            }

            TRACE_GLOBAL(FLOW, (_T("Service #%i - %s '%s'"), cnt++, provider.c_str(), name.c_str()));

            TRACE_GLOBAL(FLOW, (_T("  Handle:")));
            TRACE_GLOBAL(FLOW, (_T("    - 0x%08x"), service.Handle()));

            if (service.ServiceClassIDList() != nullptr) {
                TRACE_GLOBAL(FLOW, (_T("  Service Class ID List:")));
                for (auto const& clazz : service.ServiceClassIDList()->Classes()) {
                    TRACE_GLOBAL(FLOW, (_T("    - %s '%s'"),
                                clazz.Type().ToString().c_str(), clazz.Name().c_str()));
                }
            }

            if (service.BrowseGroupList() != nullptr) {
                TRACE_GLOBAL(FLOW, (_T("  Browse Group List:")));
                for (auto const& group : service.BrowseGroupList()->Classes()) {
                    TRACE_GLOBAL(FLOW, (_T("    - %s '%s'"),
                                group.Type().ToString().c_str(), group.Name().c_str()));
                }
            }

            if (service.ProtocolDescriptorList() != nullptr) {
                TRACE_GLOBAL(FLOW, (_T("  Protocol Descriptor List:")));
                for (auto const& protocol : service.ProtocolDescriptorList()->Protocols()) {
                    TRACE_GLOBAL(FLOW, (_T("    - %s '%s', parameters: %s"),
                                protocol.first.Type().ToString().c_str(), protocol.first.Name().c_str(),
                                Bluetooth::DataRecord(protocol.second).ToString().c_str()));
                }
            }

            if (service.LanguageBaseAttributeIDList() != nullptr) {
                TRACE_GLOBAL(FLOW, (_T("  Language Base Attribute IDs:")));
                for (auto const& lb : service.LanguageBaseAttributeIDList()->LanguageBases()) {
                    TRACE_GLOBAL(FLOW, (_T("    - 0x%04x, 0x%04x, 0x%04x"),
                                lb.Language(), lb.Charset(), lb.Base()));
                }
            }

            if (service.ProfileDescriptorList() != nullptr) {
                TRACE_GLOBAL(FLOW, (_T("  Profile Descriptor List:")));
                for (auto const& profile : service.ProfileDescriptorList()->Profiles()) {
                    TRACE_GLOBAL(FLOW, (_T("    - %s '%s', version: %d.%d"),
                                profile.first.Type().ToString().c_str(), profile.first.Name().c_str(),
                                (profile.second.Version() >> 8), (profile.second.Version() & 0xFF)));
                }
            }

            if (service.Attributes().empty() == false) {
#ifdef __DEBUG__
                TRACE_GLOBAL(FLOW, (_T("  All attributes:")));
#else
                TRACE_GLOBAL(FLOW, (_T("  Unknown attributes:")));
#endif
                for (auto const& attribute : service.Attributes()) {
                    TRACE_GLOBAL(FLOW, (_T("    - %04x '%s', value: %s"),
                                attribute.Id(), attribute.Name().c_str(),
                                Bluetooth::DataRecord(attribute.Value()).ToString().c_str()));
                }
            }
        }
    }

} // namespace Plugin

}
