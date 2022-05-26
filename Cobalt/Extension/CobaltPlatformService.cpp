/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include "Handler.h"

namespace WPEFramework {
namespace Plugin {

    bool HasPlatformService(const char* name)
    {
        return (HandlerAdministrator::Instance().Exists(name));
    }

    CobaltExtensionPlatformService OpenPlatformService(void* context, const char* name, ReceiveMessageCallback receiveCallback)
    {
        if (!HasPlatformService(name)) {
            return kCobaltExtensionPlatformServiceInvalid;
        }
        IHandler* platformservice = HandlerAdministrator::Instance().Handler(context, name, receiveCallback);
        return reinterpret_cast<CobaltExtensionPlatformService>(platformservice);
    }

    void ClosePlatformService(CobaltExtensionPlatformService service)
    {
        auto platformservice = reinterpret_cast<IHandler*>(service);
        delete platformservice;
    }

    void* SendToPlatformService(CobaltExtensionPlatformService service, void* data, uint64_t length, uint64_t* outputLength, VARIABLE_IS_NOT_USED bool* invalid_state)
    {
        auto platformservice = reinterpret_cast<IHandler*>(service);
        return (platformservice->Send(data, length, outputLength));
    }

} // namespace Plugin
} // namespace WPEFramework

const CobaltExtensionPlatformServiceApi kPlatformServiceApi = {
    kCobaltExtensionPlatformServiceName,
    1,
    &WPEFramework::Plugin::HasPlatformService,
    &WPEFramework::Plugin::OpenPlatformService,
    &WPEFramework::Plugin::ClosePlatformService,
    &WPEFramework::Plugin::SendToPlatformService,
};

EXTERNAL const void* CobaltPlatformServiceApi()
{
    return &kPlatformServiceApi;
}
