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

#include "open_cdm.h"
#include "open_cdm_adapter.h"
#include <plugins/Request.h>
#include <vector>
#include <websocket/WebLink.h>
#include <websocket/WebTransfer.h>

namespace WPEFramework {
namespace {

    // TODO: Figure out a better name
    class Challenger {

    private:
        static void process_challenge_callback(OpenCDMSession* session, void* userData, const char url[], const uint8_t challenge[], const uint16_t challengeLength)
        {
            Challenger* comm = reinterpret_cast<Challenger*>(userData);
            string challengeData(reinterpret_cast<const char*>(challenge), challengeLength);
            comm->ProcessChallengeCallback(session, url, challengeData);
        }

        static void key_update_callback(OpenCDMSession* session, void* userData, const uint8_t keyId[], const uint8_t length)
        {
            Challenger* comm = reinterpret_cast<Challenger*>(userData);
            comm->KeyUpdateCallback(session, userData, keyId, length);
        }

        static void error_message_callback(OpenCDMSession* session, void* userData, const char message[])
        {
            Challenger* comm = reinterpret_cast<Challenger*>(userData);
            comm->ErrorMessageCallback();
        }

        static void keys_updated_callback(const OpenCDMSession* session, void* userData)
        {
            Challenger* comm = reinterpret_cast<Challenger*>(userData);
            comm->KeysUpdatedCallback();
        }

        void ProcessChallengeCallback(OpenCDMSession* session, const string& url, const string& challenge)
        {
            TRACE_L1("Processing challenge to url: %s", url.c_str());

            string challengeResponse;
            _server.Exchange(url, challenge);
        }

        void KeyUpdateCallback(OpenCDMSession* session, void* userData, const uint8_t keyId[], const uint8_t length)
        {
        }

        void ErrorMessageCallback()
        {
            TRACE_L1("Error Message Callback called");
        }

        void KeysUpdatedCallback()
        {
            TRACE_L1("Keys Updated Callback called");
        }

    private:
        class LicenseServerLink : public Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, Challenger&> {

            using WebLinkClass = Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, Challenger&>;

        public:
            LicenseServerLink() = delete;
            LicenseServerLink(const LicenseServerLink& copy) = delete;
            LicenseServerLink& operator=(const LicenseServerLink&) = delete;

            LicenseServerLink(Challenger& parent)
                : WebLinkClass(2, parent, false, Core::NodeId(), Core::NodeId(), 2048, 2048)
                , _challengeRequest(Core::ProxyType<Web::Request>::Create())
                , _bodyRequest(Core::ProxyType<Web::TextBody>::Create())
                , _bodyResponse(Core::ProxyType<Web::TextBody>::Create())
                , _waitForEvent(false, true)
            {
                _challengeRequest->Verb = Web::Request::HTTP_POST;
                _challengeRequest->Connection = Web::Request::CONNECTION_CLOSE;
            }

            ~LicenseServerLink() override
            {
                Close(Core::infinite);
            }

            void Exchange(const string& rawUrl, const string& challenge)
            {
                Core::URL url(rawUrl);
                _challengeRequest->Path = '/' + url.Path().Value();
                _challengeRequest->Host = url.Host().Value();

                Core::NodeId remoteNode(url.Host().Value().c_str(), 80, Core::NodeId::TYPE_IPV4);
                if (remoteNode.IsValid() == false) {
                    TRACE_L1("Connection to %s unavailable", rawUrl.c_str());
                } else {

                    InitializeBody(challenge);

                    Link().RemoteNode(remoteNode);
                    Link().LocalNode(remoteNode.AnyInterface());
                    uint32_t result = Open(0);

                    if (_waitForEvent.Lock(3000) != Core::ERROR_NONE) {
                        TRACE_L1("Unhandled error while sending challenge request: %d", result);
                    }
                }
            }

            void KeyResponse(string& challengeResponse)
            {
                challengeResponse.assign(*_bodyResponse);
            }

        private:
            /* TODO:
            *   Is this what the EME spec calls sanitizing?
            *   What is this ':Type:' string ?
            */
            void InitializeBody(const string& challenge)
            {
                size_t index = challenge.find(":Type:");
                size_t offset = 0;

                if (index != std::string::npos)
                    offset = index + strlen(":Type:");

                _bodyRequest->assign(challenge.substr(offset));
                _challengeRequest->Body<Web::TextBody>(_bodyRequest);
                _challengeRequest->ContentType = Web::MIMETypes::MIME_TEXT_XML;
                _challengeRequest->ContentLength = challenge.substr(offset).length();
            }

            // Web::WebLinkType methods
            // --------------------------------------
            void LinkBody(Core::ProxyType<Web::Response>& element) override
            {
                // Send an event if the response is partially wrong
                element->Body<Web::TextBody>(_bodyResponse);
            }

            void Received(Core::ProxyType<Web::Response>& response) override
            {
                TRACE_L1("Received challenge response");
                _waitForEvent.SetEvent();
            }

            void Send(const Core::ProxyType<Web::Request>& request) override
            {
                ASSERT(request == _challengeRequest);
            }

            void StateChange() override
            {
                if (IsOpen()) {
                    Submit(_challengeRequest);
                }
            }

        private:
            mutable Core::CriticalSection _adminLock;
            Core::ProxyType<Web::Request> _challengeRequest;
            Core::ProxyType<Web::TextBody> _bodyRequest;
            Core::ProxyType<Web::TextBody> _bodyResponse;
            Core::Event _waitForEvent;
        };

    public:
        Challenger()
            : _server(*this)
            , _callbacks{ process_challenge_callback, key_update_callback, error_message_callback, keys_updated_callback }
        {
        }

        Core::ProxyType<Web::Response> Element()
        {
            return (PluginHost::Factories::Instance().Response());
        }

        OpenCDMSessionCallbacks& OcdmCallbacks()
        {
            return _callbacks;
        }

        void KeyResponse(string& response)
        {
            _server.KeyResponse(response);
        }

    private:
        OpenCDMSessionCallbacks _callbacks;
        LicenseServerLink _server;
    };
}
}
