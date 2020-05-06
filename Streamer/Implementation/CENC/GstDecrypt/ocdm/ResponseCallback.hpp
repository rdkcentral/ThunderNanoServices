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
#pragma once

#include "IExchange.h"
#include <ocdm/open_cdm.h>

namespace WPEFramework {
namespace CENCDecryptor {
    class OCDMResponseCallback : public IExchange::ICallback {
    public:
        OCDMResponseCallback() = delete;
        OCDMResponseCallback(const OCDMResponseCallback&) = delete;
        OCDMResponseCallback& operator=(const OCDMResponseCallback&) = delete;

        OCDMResponseCallback(OpenCDMSession*&, Core::CriticalSection& sessionLock);
        ~OCDMResponseCallback() override{};

        void Response(Core::ProxyType<Web::Request>,
            Core::ProxyType<Web::Response>) override;

    private:
        OpenCDMSession*& _session;
        Core::CriticalSection& _sessionLock;
    };
}
}
