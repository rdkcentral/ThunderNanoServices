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

#include "ResponseCallback.hpp"

namespace WPEFramework {
namespace CENCDecryptor {
    OCDMResponseCallback::OCDMResponseCallback(OpenCDMSession*& session, Core::CriticalSection& sessionLock)
        : _session(session)
        , _sessionLock(sessionLock)
    {
    }

    void OCDMResponseCallback::Response(Core::ProxyType<Web::Request> req,
        Core::ProxyType<Web::Response> res)
    {
        ASSERT(_session != nullptr)
        if (res->HasBody()) {
            Core::ProxyType<Web::TextBody> body = res->Body<Web::TextBody>();
            auto keyResponse = reinterpret_cast<const uint8_t*>(body->c_str());
            
            _sessionLock.Lock();
            
            opencdm_session_update(_session, keyResponse, body->length());
            
            _sessionLock.Unlock();
        } else {
            TRACE_L1("Challenge response without a body");
        }
    }
}
}
