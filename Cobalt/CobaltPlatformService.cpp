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

#include "Module.h"

#include "extension/platform_service.h"

namespace WPEFramework {
namespace Plugin {

    class CobaltPlatformService {
    private:
        CobaltPlatformService() = delete;
        CobaltPlatformService(const CobaltPlatformService&) = delete;
        CobaltPlatformService& operator=(const CobaltPlatformService&) = delete;

    public:
        explicit CobaltPlatformService(void* context, const char* name, ReceiveMessageCallback receiveCallback)
            : _context(context)
            , _name(name)
            , _receiveCallback(receiveCallback)
        {
        }

        ~CobaltPlatformService()
        {
            if (_name) {
                delete _name;
            }
        }

        void* Send(void* data, uint64_t length, uint64_t* output_length)
        {
            *output_length = length;
            uint8_t* output = new uint8_t[*output_length];
            for (uint64_t i = 0; i < length; i++) {
                output[i] = static_cast<uint8_t*>(data)[i] * 2;
            }
            return output;
        }

    private:
        void* _context;
        const char* _name;
        ReceiveMessageCallback _receiveCallback;
    };

    bool HasPlatformService(VARIABLE_IS_NOT_USED const char* name)
    {
        return true;
    }

    CobaltExtensionPlatformService OpenPlatformService(void* context, const char* name, ReceiveMessageCallback receiveCallback)
    {
        if (!HasPlatformService(name)) {
            return kCobaltExtensionPlatformServiceInvalid;
        }
        CobaltPlatformService* platformservice = new CobaltPlatformService(context, name, receiveCallback);
        return reinterpret_cast<CobaltExtensionPlatformService>(platformservice);
    }

    void ClosePlatformService(CobaltExtensionPlatformService service)
    {
        auto platformservice = reinterpret_cast<CobaltPlatformService*>(service);
        delete platformservice;
    }

    void* SendToPlatformService(CobaltExtensionPlatformService service, void* data, uint64_t length, uint64_t* output_length, VARIABLE_IS_NOT_USED bool* invalid_state)
    {
        auto platformservice = reinterpret_cast<CobaltPlatformService*>(service);
        return (platformservice->Send(data, length, output_length));
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
