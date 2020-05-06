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

#include "Exchanger.hpp"
#include <plugins/System.h>

namespace WPEFramework {
namespace CENCDecryptor {
    Core::ProxyPoolType<Web::TextBody> Exchanger::Challenger::_bodyFactory(2);

    Exchanger::Exchanger(const std::string& url)
        : _url(url)
        , _reqTrigger(false, true)
        , _resReceived(false, true)
        , _challenger(*this, _resReceived)
        , _reqOverwatch()
    {
    }

    Core::ProxyType<Web::Response> Exchanger::Element()
    {
        return (PluginHost::Factories::Instance().Response());
    }

    uint32_t Exchanger::Submit(Core::ProxyType<Web::Request> request,
        Core::ProxyType<IExchange::ICallback> onResponse, uint32_t waitTime)
    {
        if (_reqOverwatch.IsValid() && _reqOverwatch->IsRunning()) {
            return Core::ERROR_INPROGRESS;
        } else {
            if (_reqOverwatch.IsValid()) {
                _reqOverwatch.Release();
            }

            _reqOverwatch = Core::ProxyType<Overwatch>::Create(_challenger,
                LicenseRequestData({ request, onResponse, waitTime, _url }),
                _resReceived);
            _reqOverwatch->Run();

            return Core::ERROR_NONE;
        }
    }

    uint32_t Exchanger::Revoke(Core::ProxyType<IExchange::ICallback> onResponse)
    {
        // TODO
    }

    Exchanger::Challenger::Challenger(Exchanger& parent, Core::Event& resReceived)
        : Exchanger::Challenger::WebLinkClass(2, parent, false, Core::NodeId(), Core::NodeId(), 2048, 2048)
        , _resReceived(resReceived)
    {
    }

    void Exchanger::Challenger::Send(const Core::ProxyType<Web::Request>& request, const Core::URL& url)
    {
        _request = request;
        _request->Path = '/' + url.Path().Value();
        _request->Host = url.Host().Value();

        Core::NodeId remoteNode(url.Host().Value().c_str(), 80, Core::NodeId::TYPE_IPV4);
        if (remoteNode.IsValid() == false) {
            TRACE_L1("Connection to %s unavailable", url.Host().Value().c_str());
        } else {
            Link().RemoteNode(remoteNode);
            Link().LocalNode(remoteNode.AnyInterface());
            uint32_t result = Open(0);
        }
    }

    Core::ProxyType<Web::Response> Exchanger::Challenger::Response()
    {
        return _response;
    }

    void Exchanger::Challenger::LinkBody(Core::ProxyType<Web::Response>& element)
    {
        element->Body<Web::TextBody>(_bodyFactory.Element());
    }

    void Exchanger::Challenger::Received(Core::ProxyType<Web::Response>& res)
    {
        _response = res;
        std::string str;
        _response->ToString(str);
        _resReceived.SetEvent();
    }

    void Exchanger::Challenger::Send(const Core::ProxyType<Web::Request>& req)
    {
        ASSERT(req == _request);
    }

    void Exchanger::Challenger::StateChange()
    {
        std::string str;
        _request->ToString(str);
        if (IsOpen()) {
            Submit(_request);
        }
    }

    Exchanger::Overwatch::Overwatch(Challenger& challenger, const LicenseRequestData& requestData, Core::Event& resReceived)
        : _challenger(challenger)
        , _requestData(requestData)
        , _resReceived(resReceived)
    {
    }

    uint32_t Exchanger::Overwatch::Worker()
    {
        _challenger.Send(_requestData.licenseRequest, this->_requestData.url);
        uint32_t waitResult = _resReceived.Lock(this->_requestData.timeout);

        if (waitResult != Core::ERROR_NONE) {
            TRACE_L1("Error while sending challenge request <%d>", waitResult);
        } else {
            this->_requestData.licenseHandler->Response(this->_requestData.licenseRequest,
                this->_challenger.Response());
        }

        Stop();
        return Core::infinite;
    }
}
}
