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

#include <securityagent/securityagent.h>

namespace WPEFramework {
namespace Plugin {

    class Token : public IHandler {
    private:
        Token() = delete;
        Token(const Token&) = delete;
        Token& operator=(const Token&) = delete;

        using Job = Core::ThreadPool::JobType<Token&>;

    public:
        Token(void* context, const char* name, ReceiveMessageCallback receiveCallback)
            : _context(context)
            , _name(name)
            , _receiveCallback(receiveCallback)
            , _url()
            , _job(*this)
        {
        }
        ~Token() override
        {
            // Revoke job
            Core::ProxyType<Core::IDispatch> job(_job.Revoke());
            if (job.IsValid()) {
                Core::IWorkerPool::Instance().Revoke(job);
                _job.Revoked();
            }
            if (_name) {
                delete _name;
            }
        }
        void Dispatch()
        {
            uint16_t outputLength = 2 * 1024;
            uint8_t* output = new uint8_t[outputLength];

            ::memset(output, 0, outputLength);
            ::memcpy(output, _url.c_str(), _url.length());

            outputLength = GetToken(outputLength, _url.length(), output);
            if (outputLength > 0) {
                _receiveCallback(_context, output, outputLength);
            } else {
                delete output;
            }
        }
        void* Send(void* data, uint64_t length, uint64_t* outputLength) override
        {
            _url = string(static_cast<const char*>(data), length);
            if (!_url.empty()) {
                // Schedule job
                Core::ProxyType<Core::IDispatch> job(_job.Submit());
                if (job.IsValid()) {
                    Core::IWorkerPool::Instance().Submit(job);
                }
            }
            *outputLength = 0;
            return nullptr;
        }

    private:
        void* _context;
        const char* _name;
        ReceiveMessageCallback _receiveCallback;
        string _url;
        Job _job;
    };

    static HandlerAdministrator::Entry<Token> handler;

} // namespace Plugin
} // namespace WPEFramework
