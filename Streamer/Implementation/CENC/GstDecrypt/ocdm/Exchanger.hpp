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

namespace WPEFramework {
namespace CENCDecryptor {

    class Exchanger : public IExchange {
    public:
        explicit Exchanger(const std::string& url);
        Exchanger() = delete;
        Exchanger(const Exchanger&) = delete;
        Exchanger& operator=(const Exchanger&) = delete;

        ~Exchanger() override
        {
            // _challenger.
        };

        uint32_t Submit(Core::ProxyType<Web::Request>,
            Core::ProxyType<IExchange::ICallback>, uint32_t waitTime) override;

        uint32_t Revoke(Core::ProxyType<IExchange::ICallback>) override;
        Core::ProxyType<Web::Response> Element();

    private:
        class Challenger : public Web::WebLinkType<
                               Core::SocketStream,
                               Web::Response,
                               Web::Request,
                               Exchanger&> {

            using WebLinkClass = Web::WebLinkType<
                Core::SocketStream,
                Web::Response,
                Web::Request,
                Exchanger&>;

        public:
            Challenger() = delete;
            explicit Challenger(Exchanger& parent, Core::Event& resReceived);

            void Send(const Core::ProxyType<Web::Request>&, const Core::URL&);
            Core::ProxyType<Web::Response> Response();

        private:
            void LinkBody(Core::ProxyType<Web::Response>& element) override;
            void Received(Core::ProxyType<Web::Response>& text) override;
            void Send(const Core::ProxyType<Web::Request>& text) override;
            void StateChange() override;

            Core::Event& _resReceived;
            Core::ProxyType<Web::Response> _response;
            Core::ProxyType<Web::Request> _request;
            static Core::ProxyPoolType<Web::TextBody> _bodyFactory;
        };

        struct LicenseRequestData {
            Core::ProxyType<Web::Request> licenseRequest;
            Core::ProxyType<IExchange::ICallback> licenseHandler;
            uint32_t timeout;
            Core::URL url;
        };

        class Overwatch : public Core::Thread {
        public:
            Overwatch() = delete;
            Overwatch(Challenger& challenger,
                const LicenseRequestData& reqData,
                Core::Event& resReceived);

            uint32_t Worker() override;

        private:
            Challenger& _challenger;
            LicenseRequestData _requestData;
            Core::Event& _resReceived;
        };

    private:
        Core::URL _url;
        Core::Event _reqTrigger;
        Core::Event _resReceived;
        Challenger _challenger;
        LicenseRequestData _requestData;
        Core::ProxyType<Overwatch> _reqOverwatch;
    };
}
}
