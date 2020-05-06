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

#include <plugins/Request.h>
#include <websocket/WebLink.h>
#include <websocket/WebTransfer.h>

namespace WPEFramework {
class IExchange {
public:
    class ICallback {
    public:
        virtual void Response(Core::ProxyType<Web::Request>, Core::ProxyType<Web::Response>) = 0;

        virtual ~ICallback(){};
    };

    virtual uint32_t Submit(Core::ProxyType<Web::Request>,
        Core::ProxyType<IExchange::ICallback>, uint32_t waitTime) = 0;

    virtual uint32_t Revoke(Core::ProxyType<IExchange::ICallback>) = 0;

    virtual ~IExchange(){};
};
}
