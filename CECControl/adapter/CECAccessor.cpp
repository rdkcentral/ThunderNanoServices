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

#include "CECAccessor.h"

namespace Thunder {
namespace CEC {
    IAccessor* IAccessor::Instance()
    {
        static CECAccessor& _instance = Core::SingletonType<CECAccessor>::Instance();
        return &_instance;
    }

    uint32_t CECAccessor::Announce(const std::string& id, const std::string& config)
    {
        Config info;
        info.FromString(config);

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        AdapterMap::iterator index = _adapters.find(id);

        if (index == _adapters.end()) {
            _adapters.emplace(std::piecewise_construct,
                std::forward_as_tuple(id),
                std::forward_as_tuple(Core::ProxyType<AdapterImplementation>(Core::ProxyType<AdapterImplementation>::Create(info))));
            TRACE(Trace::Information, ("Anounced %s.", id.c_str()));
        } else {
            TRACE(Trace::Error, ("Skipped id %s, it was allready annouced.", id.c_str()));
        }

        return 0;
    }

    uint32_t CECAccessor::Revoke(const std::string& id)
    {
        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        AdapterMap::iterator index = _adapters.find(id);

        if (index != _adapters.end()) {
            _adapters.erase(index);
            TRACE(Trace::Information, ("Revoked %s.", id.c_str()));
        } else {
            TRACE(Trace::Error, ("Skipped id %s it was not found.", id.c_str()));
        }

        return 0;
    }

    Adapter CECAccessor::GetAdapter(const string& id, const cec_adapter_role_t role)
    {
        Adapter adapter;

        AdapterMap::const_iterator index = _adapters.find(id);

        if (index != _adapters.cend()) {
            if ((*index->second).IsSupported(role) == true) {
                adapter = Adapter(role, Core::ProxyType<IDeviceAdapter>(*index->second));
            } else {
                TRACE(Trace::Error, ("Interface %s doesn't support role 0x%02X", id.c_str(), role));
            }
        } else {
            TRACE(Trace::Error, ("Interface %s doesn't exist", id.c_str()));
        }

        return adapter;
    }

} // namespace CEC
} // namespace Thunder