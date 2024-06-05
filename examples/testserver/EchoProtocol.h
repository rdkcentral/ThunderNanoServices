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

#ifndef __ECHOPROTOCOL_H
#define __ECHOPROTOCOL_H

#include "DataContainer.h"

namespace Thunder {
namespace TestSystem {
    class StressTextFactory {
    private:
        StressTextFactory(const StressTextFactory&);
        StressTextFactory& operator=(const StressTextFactory&);

    public:
        StressTextFactory()
            : _minLength(static_cast<uint16_t>(~0))
            , _maxLength(0)
        {
            _testSequence = "theTHEquickQUICKbrownBROWNfoxFOXjumpsJUMPSoverOVERlazyLAZYdogDOG!";
        }
        ~StressTextFactory()
        {
        }

    public:
        inline uint16_t MinSize() const
        {
            return (_minLength);
        }
        inline uint16_t MaxSize() const
        {
            return (_maxLength);
        }
        string GenerateData()
        {
            // Generate the message
            uint16_t index = 0;
            uint16_t requiredLength;
            Crypto::Random(requiredLength, static_cast<uint16_t>(8), static_cast<uint16_t>(2048));
            string myMessage = Core::NumberType<uint16_t, false, BASE_HEXADECIMAL>(requiredLength).Text();

            if (requiredLength < _minLength) {
                _minLength = requiredLength;
            }
            if (requiredLength > _maxLength) {
                _maxLength = requiredLength;
            }
            myMessage += " ";

            while (myMessage.length() < requiredLength) {
                myMessage += _testSequence[index & 0x3F];
                index++;
            }

            return (myMessage);
        }
        bool VerifyData(const string& data) const
        {
            bool result = false;

            // Length should be in the beginning, validate that firs
            Core::NumberType<uint16_t, false> expectedLength(data.c_str(), 7, BASE_HEXADECIMAL);

            if (expectedLength.Value() == data.length()) {
                const TCHAR* buffer = data.c_str();
                uint16_t lengthToValidate = 0;

                // Oke length is oke, now check the pattern..
                while ((lengthToValidate <= 7) && (*buffer != ' ')) {
                    lengthToValidate++;
                    buffer++;
                }

                if (*buffer == ' ') {
                    // We are still good to go !!!
                    lengthToValidate = data.length() - lengthToValidate - 1;
                    buffer++;
                    result = true;

                    while ((lengthToValidate-- != 0) && (result == true)) {
                        result = (buffer[lengthToValidate] == _testSequence[lengthToValidate & 0x3F]);
                    }

                    if (result == false) {
                        _minLength += 1;
                    }
                }
            }

            if (result == false) {
                _minLength -= 1;
            }

            return (result);
        }

    private:
        const TCHAR* _testSequence;
        mutable uint16_t _minLength;
        uint16_t _maxLength;
    };

    // -----------------------------------------------------------------------------------------------
    // Create a resource allocator for all JSON objects used in these tests
    // -----------------------------------------------------------------------------------------------
    template <typename INTERFACE>
    class JSONObjectFactory : public Core::FactoryType<INTERFACE, string> {
    public:
        inline JSONObjectFactory()
            : Core::FactoryType<INTERFACE, string>()
            , _jsonRPCFactory(5)
        {
        }
        JSONObjectFactory(const JSONObjectFactory&);
        JSONObjectFactory& operator=(const JSONObjectFactory&);
        ~JSONObjectFactory() = default;

    public:
        static JSONObjectFactory& Instance()
        {
            static JSONObjectFactory<INTERFACE> _singleton;

            return (_singleton);
        }

        Core::ProxyType<INTERFACE> Element(const string& identifier)
        {
            (void)identifier;
            Core::ProxyType<Web::JSONBodyType<Core::JSONRPC::Message>> message = _jsonRPCFactory.Element();
            return Core::ProxyType<INTERFACE>(message);
        }
    private:
        Core::ProxyPoolType<Web::JSONBodyType<Core::JSONRPC::Message>> _jsonRPCFactory;
    };
    // -----------------------------------------------------------------------------------------------
    // TEST SET 1
    // ----------
    // Direct connection for transfering text in an echo fashion.
    // -----------------------------------------------------------------------------------------------
    class TextConnector : public Core::StreamTextType<Core::SocketStream, Core::TerminatorCarriageReturn> {
    private:
        typedef Core::StreamTextType<Core::SocketStream, Core::TerminatorCarriageReturn> BaseClass;

        TextConnector();
        TextConnector(const TextConnector& copy);
        TextConnector& operator=(const TextConnector&);

    public:
        TextConnector(const Thunder::Core::NodeId& remoteNode)
            : BaseClass(false, remoteNode.AnyInterface(), remoteNode, 1024, 1024)
            , _serverSocket(false)
            , _dataPending(false, false)
        {
        }
        TextConnector(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<TextConnector>*)
            : BaseClass(false, connector, remoteId, 1024, 1024)
            , _serverSocket(true)
            , _dataPending(false, false)
        {
        }
        ~TextConnector() override
        {
            Close(Thunder::Core::infinite);
        }

    public:
        virtual void Received(string& text)
        {
            printf(_T("Received: %s\n"), text.c_str());
            printf(_T("   Bytes: %d\n"), static_cast<uint32_t>(text.size()));

            // prevent singing around, only sockets created in a serve context should reply!!
            if (_serverSocket == true) {
PUSH_WARNING(DISABLE_WARNING_DEPRECATED_USE)

                TCHAR buffer[128];
                sprintf(buffer, _T("Handled [%d] bytes."), static_cast<uint32_t>(text.size()));

POP_WARNING()

                //Submit(string(buffer));
                Submit(text);
            }
            else {
                _dataReceived = text;
                _dataPending.SetEvent();
            }
        }

        int Wait(unsigned int milliseconds) const
        {
            return _dataPending.Lock(milliseconds);
        }

        bool Retrieve(string& text)
        {
            text = _dataReceived;
            bool isEmpty = _dataReceived.empty();
            // clear internal receive buffer once obtained
            _dataReceived.clear();
            return !isEmpty;
        }

        virtual void Send(const string& text)
        {
            printf(_T("Bytes: %d\n"), static_cast<uint32_t>(text.size()));
            printf(_T(" Send: %s\n"), text.c_str());
        }
        virtual void StateChange()
        {
            printf(_T("State change: "));
            if (IsOpen()) {
                printf(_T("[1] Open - OK\n"));
            }
            else {
                printf(_T("[1] Closed - %s\n"), (IsSuspended() ? _T("SUSPENDED") : _T("OK")));
            }
        }

    private:
        bool _serverSocket;
        string _dataReceived;
        mutable Thunder::Core::Event _dataPending;
    };

    // -----------------------------------------------------------------------------------------------
    // TEST SET 2
    // ----------
    // Direct connection for transfering JSON in an echo fashion.
    // -----------------------------------------------------------------------------------------------
    template <typename INTERFACE>
    class JSONConnector : public Core::StreamJSONType<Core::SocketStream, JSONObjectFactory<INTERFACE>&, INTERFACE> {
    private:
        typedef Core::StreamJSONType<Core::SocketStream, JSONObjectFactory<INTERFACE>&, INTERFACE> BaseClass;

        JSONConnector();
        JSONConnector(const JSONConnector& copy);
        JSONConnector& operator=(const JSONConnector&);

    public:
        JSONConnector(const Thunder::Core::NodeId& remoteNode)
            : BaseClass(5, JSONObjectFactory<INTERFACE>::Instance(), false, remoteNode.AnyInterface(), remoteNode, 1024, 1024)
            , _serverSocket(false)
            , _dataPending(false, false)
        {
        }
        JSONConnector(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<JSONConnector>*)
            : BaseClass(5, JSONObjectFactory<INTERFACE>::Instance(), false, connector, remoteId, 1024, 1024)
            , _serverSocket(true)
            , _dataPending(false, false)
        {
        }
        ~JSONConnector() override
        {
            this->Close(Thunder::Core::infinite);
        }

    public:
        virtual void Received(Core::ProxyType<INTERFACE>& jsonObject)
        {
            if (jsonObject.IsValid() == false) {
                printf("Invalid Json object Received\n");
            }
            else {
                Core::ProxyType<Core::JSON::IElement> newElement = Core::ProxyType<Core::JSON::IElement>(jsonObject);
                string textElement;
                newElement->ToString(textElement);

                printf(_T("   Bytes: %d\n"), static_cast<uint32_t>(textElement.size()));
                printf(_T("Received: %s\n"), textElement.c_str());

                // prevent singing around, only sockets created in a serve context should reply!!
                if (_serverSocket == true) {
                    // As this is the server, send back the Element we received...
                    this->Submit(newElement);
                }
                else {
                    _dataReceived = textElement;
                    _dataPending.SetEvent();
                }
            }


        }

        int Wait(unsigned int milliseconds) const
        {
            return _dataPending.Lock(milliseconds);
        }

        bool Retrieve(Core::ProxyType<Core::JSON::IElement>&& newElement)
        {
            newElement->FromString(_dataReceived);
            bool isEmpty = _dataReceived.empty();
            // clear internal receive buffer once obtained
            _dataReceived.clear();
            return !isEmpty;
        }

        virtual void Send(Core::ProxyType<INTERFACE>& jsonObject)
        {
            if (jsonObject.IsValid() == false) {
                printf("Invalid Json object Received\n");
            }
            else {
                Core::ProxyType<Core::JSON::IElement> newElement = Core::ProxyType<Core::JSON::IElement>(jsonObject);
                string textElement;
                newElement->ToString(textElement);

                printf(_T("Bytes: %d\n"), static_cast<uint32_t>(textElement.size()));
                printf(_T(" Send: %s\n"), textElement.c_str());
            }

        }
        virtual void StateChange()
        {
            printf(_T("State change: "));
            if (this->IsOpen()) {
                printf(_T("[2] Open - OK\n"));
            }
            else {
                printf(_T("[2] Closed - %s\n"), (this->IsSuspended() ? _T("SUSPENDED") : _T("OK")));
            }
        }
        virtual bool IsIdle() const
        {
            return (true);
        }

    private:
        bool _serverSocket;
        string _dataReceived;
        mutable Thunder::Core::Event _dataPending;
    };

    // -----------------------------------------------------------------------------------------------
    // TEST SET 3
    // ----------
    // Basic Web Echo Service, What is coming in, is send out (WebServer)
    // Basic Web Client connector. Allows you to send out a Request and receive response (WebClient)
    // -----------------------------------------------------------------------------------------------
    class EXTERNAL WebServer : public Web::WebLinkType<Core::SocketStream, Web::Request, Web::Response, Thunder::Core::ProxyPoolType<Web::Request> > {
    private:
        typedef Web::WebLinkType<Core::SocketStream, Web::Request, Web::Response, Thunder::Core::ProxyPoolType<Web::Request> > BaseClass;

        WebServer();
        WebServer(const WebServer& copy);
        WebServer& operator=(const WebServer&);

    public:
        WebServer(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<WebServer>*)
            : BaseClass(5, false, connector, remoteId, 2048, 2048)
        {
        }
        ~WebServer() override
        {
            Close(Thunder::Core::infinite);
        }

    public:
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(Core::ProxyType<Thunder::Web::Request>& element)
        {
            // Time to attach a String Body
            element->Body<Web::TextBody>(_textBodyFactory.Element());
        }
        virtual void Received(Core::ProxyType<Thunder::Web::Request>& request)
        {
            string received;
            request->ToString(received);
            printf(_T("Received: %s\n"), received.c_str());

            // What we get in, we should send it back :-)
            Core::ProxyType<Thunder::Web::Response> response = _responseFactory.Element();
            Core::ProxyType<Web::TextBody> textBody = request->Body<Web::TextBody>();

            // We attached it so it should be available!!!
            ASSERT(textBody.IsValid());

            response->Date = Core::Time::Now();
            response->ErrorCode = 200;
            *textBody = "Here is your shit back [" + *textBody + "]";
            response->Body<Web::TextBody>(textBody);
            Submit(response);
        }
        virtual void Send(const Core::ProxyType<Thunder::Web::Response>& response)
        {
            string send;
            response->ToString(send);
            printf(_T("Send: %s\n"), send.c_str());
        }
        virtual void StateChange()
        {
            printf(_T("State change: "));
            if (IsOpen()) {
                printf(_T("[3] Open - OK\n"));
            }
            else {
                printf(_T("[3] Closed - %s\n"), (IsSuspended() ? _T("SUSPENDED") : _T("OK")));
            }
        }

    private:
        static Core::ProxyPoolType<Thunder::Web::Response> _responseFactory;
        static Core::ProxyPoolType<Web::TextBody> _textBodyFactory;
    };
    class EXTERNAL WebClient : public Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, Thunder::Core::ProxyPoolType<Web::Response>&> {
    private:
        typedef Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, Thunder::Core::ProxyPoolType<Web::Response>&> BaseClass;

        // All requests needed by any instance of this socket class, is coming from this static factory.
        // This means that all Requests, received are shared among all WebServer sockets, hopefully limiting
        // the number of requests that need to be created.
        static Thunder::Core::ProxyPoolType<Web::Response> _responseFactory;

        WebClient();
        WebClient(const WebClient& copy);
        WebClient& operator=(const WebClient&);

    public:
        WebClient(const Thunder::Core::NodeId& remoteNode)
            : BaseClass(5, _responseFactory, false, remoteNode.AnyInterface(), remoteNode, 2048, 208)
        {
        }
        ~WebClient() override
        {
            Close(Thunder::Core::infinite);
        }

    public:
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(Core::ProxyType<Thunder::Web::Response>& element)
        {
            // Time to attach a String Body
            element->Body<Web::TextBody>(_textBodyFactory.Element());
        }
        virtual void Received(Core::ProxyType<Thunder::Web::Response>& element)
        {
            string received;
            element->ToString(received);
            printf(_T("Received: %s\n"), received.c_str());
        }
        virtual void Send(const Core::ProxyType<Thunder::Web::Request>& response)
        {
            string send;
            response->ToString(send);
            printf(_T("Send: %s\n"), send.c_str());
        }
        virtual void StateChange()
        {
            printf(_T("State change: "));
            if (IsOpen()) {
                printf(_T("[3] Open - OK\n"));
            }
            else {
                printf(_T("[3] Closed - %s\n"), (IsSuspended() ? _T("SUSPENDED") : _T("OK")));
            }
        }
        static Core::ProxyPoolType<Web::TextBody> _textBodyFactory;
    };

    // -----------------------------------------------------------------------------------------------
    // TEST SET 4
    // ----------
    // JSON Webrequest server, composit JSON object to the request/response. (JSONWebServer)
    // JSON Webrequest client, composit JSON object to the request/response. (JSONWebClient)
    // -----------------------------------------------------------------------------------------------
    typedef Thunder::Web::JSONBodyType<Thunder::DataContainer::Command> CommandBody;

    class JSONWebServer : public Web::WebLinkType<Core::SocketStream, Web::Request, Web::Response, Thunder::Core::ProxyPoolType<Web::Request>&> {
    private:
        typedef Web::WebLinkType<Core::SocketStream, Web::Request, Web::Response, Thunder::Core::ProxyPoolType<Web::Request>&> BaseClass;

        // All requests needed by any instance of this socket class, is coming from this static factory.
        // This means that all Requests, received are shared among all WebServer sockets, hopefully limiting
        // the number of requests that need to be created.
        static Thunder::Core::ProxyPoolType<Web::Request> _requestFactory;
        static Thunder::Core::ProxyPoolType<CommandBody> _commandBodyFactory;

        JSONWebServer();
        JSONWebServer(const JSONWebServer& copy);
        JSONWebServer& operator=(const JSONWebServer&);

    public:
        JSONWebServer(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<JSONWebServer>*)
            : BaseClass(5, _requestFactory, false, connector, remoteId, 2048, 2048)
        {
        }
        ~JSONWebServer() override
        {
            Close(Thunder::Core::infinite);
        }

    public:
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(Core::ProxyType<Thunder::Web::Request>& element)
        {
            // Time to attach a Command Body
            element->Body<CommandBody>(_commandBodyFactory.Element());
        }
        virtual void Received(Core::ProxyType<Web::Request>& request)
        {
            string received;
            request->ToString(received);
            printf(_T("Received: %s\n"), received.c_str());
        }
        virtual void Send(const Core::ProxyType<Web::Response>& response)
        {
            string send;
            response->ToString(send);
            printf(_T("Send: %s\n"), send.c_str());
        }
        virtual void StateChange()
        {
            printf(_T("State change: "));
            if (IsOpen()) {
                printf(_T("[4] Open - OK\n"));
            }
            else {
                printf(_T("[4] Closed - %s\n"), (IsSuspended() ? _T("SUSPENDED") : _T("OK")));
            }
        }
    };
    class JSONWebClient : public Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, Thunder::Core::ProxyPoolType<Web::Response>&> {
    private:
        typedef Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, Thunder::Core::ProxyPoolType<Web::Response>&> BaseClass;

        // All requests needed by any instance of this socket class, is coming from this static factory.
        // This means that all Requests, received are shared among all WebServer sockets, hopefully limiting
        // the number of requests that need to be created.
        static Thunder::Core::ProxyPoolType<Web::Response> _responseFactory;
        static Thunder::Core::ProxyPoolType<CommandBody> _commandBodyFactory;

        JSONWebClient();
        JSONWebClient(const JSONWebClient& copy);
        JSONWebClient& operator=(const JSONWebClient&);

    public:
        JSONWebClient(const Thunder::Core::NodeId& remoteNode)
            : BaseClass(5, _responseFactory, false, remoteNode.AnyInterface(), remoteNode, 2048, 208)
        {
        }
        ~JSONWebClient() override
        {
            Close(Thunder::Core::infinite);
        }

    public:
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(Core::ProxyType<Thunder::Web::Response>& element)
        {
            // Time to attach a Command Body
            element->Body<CommandBody>(_commandBodyFactory.Element());
        }
        virtual void Received(Core::ProxyType<Web::Response>& response)
        {
            string received;
            response->ToString(received);
            printf(_T("Received: %s\n"), received.c_str());
        }
        virtual void Send(const Core::ProxyType<Web::Request>& request)
        {
            string send;
            request->ToString(send);
            printf(_T("Send: %s\n"), send.c_str());
        }
        virtual void StateChange()
        {
            printf(_T("State change: "));
            if (IsOpen()) {
                printf(_T("[4] Open - OK\n"));
            }
            else {
                printf(_T("[4] Closed - %s\n"), (IsSuspended() ? _T("SUSPENDED") : _T("OK")));
            }
        }
    };

    // -----------------------------------------------------------------------------------------------
    // TEST SET 5
    // ----------
    // WebSocket Echo service (TEXT), send back what we got + postfix. This is the native WebSockets.
    // -----------------------------------------------------------------------------------------------
    class EchoWebSocketServer : public Core::StreamTextType<Web::WebSocketServerType<Core::SocketStream>, Core::TerminatorCarriageReturn> {
    private:
        typedef Core::StreamTextType<Web::WebSocketServerType<Core::SocketStream>, Core::TerminatorCarriageReturn> BaseClass;

    private:
        EchoWebSocketServer(const EchoWebSocketServer&);
        EchoWebSocketServer& operator=(const EchoWebSocketServer&);

    public:
        EchoWebSocketServer(const SOCKET& socket, const Thunder::Core::NodeId& remoteNode, Thunder::Core::SocketServerType<EchoWebSocketServer>*)
            : BaseClass(false, true, false, socket, remoteNode, 1024, 1024)
        {
        }
        ~EchoWebSocketServer() override = default;

    private:
        virtual void Received(string& text)
        {
            printf(_T("Received: %s\n"), text.c_str());

            // Send back what we receive
            Submit(_T("Here is your shit back [") + text + _T("]"));
        }
        virtual void Send(const string& text)
        {
            printf(_T("Send: %s\n"), text.c_str());
        }
        virtual void StateChange()
        {
            printf(_T("State change: "));
            if (IsOpen()) {
                printf(_T("[5] Open - OK\n"));
            }
            else {
                printf(_T("[5] Closed - %s\n"), (IsSuspended() ? _T("SUSPENDED") : _T("OK")));
            }
        }
    };
    class EchoWebSocketClient : public Core::StreamTextType<Web::WebSocketClientType<Core::SocketStream>, Core::TerminatorCarriageReturn> {
    private:
        typedef Core::StreamTextType<Web::WebSocketClientType<Core::SocketStream>, Core::TerminatorCarriageReturn> BaseClass;

    private:
        EchoWebSocketClient(const EchoWebSocketClient&);
        EchoWebSocketClient& operator=(const EchoWebSocketClient&);

    public:
        EchoWebSocketClient(const Core::NodeId& remoteNode) //TODO: Fix Me ! Pass correct value for 3rd & 4th params
            : BaseClass(_T("/"), _T("echo"), _T(""), _T(""), false, true, false, remoteNode.AnyInterface(), remoteNode, 1024, 1024)
        {
        }
        ~EchoWebSocketClient() override = default;

    private:
        virtual void Received(string& text)
        {
            printf(_T("Received: %s\n"), text.c_str());
        }
        virtual void Send(const string& text)
        {
            printf(_T("Send: %s\n"), text.c_str());
        }
        virtual void StateChange()
        {
            printf(_T("State change: "));
            if (IsOpen()) {
                printf(_T("[5] Open - OK\n"));
            }
            else {
                printf(_T("[5] Closed - %s\n"), (IsSuspended() ? _T("SUSPENDED") : _T("OK")));
            }
        }
    };

    // -----------------------------------------------------------------------------------------------
    // TEST SET 6
    // ----------
    // WebSocket Echo service (JSON), send back what we got.
    // -----------------------------------------------------------------------------------------------
    template <typename INTERFACE>
    class JSONWebSocketServer : public Core::StreamJSONType<Web::WebSocketServerType<Core::SocketStream>, JSONObjectFactory<INTERFACE>&, INTERFACE> {
    private:
        typedef Core::StreamJSONType<Web::WebSocketServerType<Core::SocketStream>, Thunder::TestSystem::JSONObjectFactory<INTERFACE>&, INTERFACE> BaseClass;

    private:
        JSONWebSocketServer(const JSONWebSocketServer&);
        JSONWebSocketServer& operator=(const JSONWebSocketServer&);

    public:
        JSONWebSocketServer(const SOCKET& socket, const Thunder::Core::NodeId& remoteNode, Thunder::Core::SocketServerType<JSONWebSocketServer>*)
            : BaseClass(5, Thunder::TestSystem::JSONObjectFactory<INTERFACE>::Instance(), false, true, false, socket, remoteNode, 256, 256)
        {
        }
        ~JSONWebSocketServer() override = default;

    public:
        virtual void Received(Core::ProxyType<INTERFACE>& jsonObject)
        {
            if (jsonObject.IsValid() == false) {
                printf("Oops");
            }
            else {
                Core::ProxyType<Core::JSON::IElement> element = Core::ProxyType<Core::JSON::IElement>(jsonObject);
                string textElement;
                element->ToString(textElement);

                printf(_T("   Bytes: %d\n"), static_cast<uint32_t>(textElement.size()));
                printf(_T("Received: %s\n"), textElement.c_str());

                // As this is the server, send back the Element we received...
                this->Submit(jsonObject);
            }
        }

        virtual void Send(Core::ProxyType<INTERFACE>& jsonObject)
        {
            if (jsonObject.IsValid() == false) {
                printf("Oops");
            }
            else {
                Core::ProxyType<Core::JSON::IElement> element = Core::ProxyType<Core::JSON::IElement>(jsonObject);

                string textElement;
                element->ToString(textElement);

                printf(_T("Bytes: %d\n"), static_cast<uint32_t>(textElement.size()));
                printf(_T(" Send: %s\n"), textElement.c_str());
            }
        }
        virtual void StateChange()
        {
            printf(_T("State change: "));
            if (this->IsOpen()) {
                printf(_T("[6] Open - OK\n"));
            }
            else {
                printf(_T("[6] Closed - %s\n"), (this->IsSuspended() ? _T("SUSPENDED") : _T("OK")));
            }
        }
    };
    template <typename INTERFACE>
    class JSONWebSocketClient : public Core::StreamJSONType<Web::WebSocketClientType<Core::SocketStream>, Thunder::TestSystem::JSONObjectFactory<INTERFACE>&, INTERFACE> {
    private:
        typedef Core::StreamJSONType<Web::WebSocketClientType<Core::SocketStream>, Thunder::TestSystem::JSONObjectFactory<INTERFACE>&, INTERFACE> BaseClass;

    private:
        JSONWebSocketClient(const JSONWebSocketClient&);
        JSONWebSocketClient& operator=(const JSONWebSocketClient&);

    public:
        JSONWebSocketClient(const Core::NodeId& remoteNode)
            : BaseClass(5, Thunder::TestSystem::JSONObjectFactory<INTERFACE>::Instance(), _T("/"), _T("echo"), _T(""), _T(""), false, true, false, remoteNode.AnyInterface(), remoteNode, 256, 256)
        {
        }
        ~JSONWebSocketClient() override = default;

    public:
        virtual void Received(Core::ProxyType<INTERFACE>& jsonObject)
        {
            if (jsonObject.IsValid() == false) {
                printf("Oops");
            }
            else {
                Core::ProxyType<Core::JSON::IElement> element = Core::ProxyType<Core::JSON::IElement>(jsonObject);

                string textElement;
                element->ToString(textElement);

                printf(_T("   Bytes: %d\n"), static_cast<uint32_t>(textElement.size()));
                printf(_T("Received: %s\n"), textElement.c_str());
            }
        }
        virtual void Send(Core::ProxyType<INTERFACE>& jsonObject)
        {
            if (jsonObject.IsValid() == false) {
                printf("Oops");
            }
            else {
                Core::ProxyType<Core::JSON::IElement> element = Core::ProxyType<Core::JSON::IElement>(jsonObject);

                string textElement;
                element->ToString(textElement);

		printf(_T("Bytes: %d\n"), static_cast<uint32_t>(textElement.size()));
		printf(_T(" Send: %s\n"), textElement.c_str());
            }
        }
        virtual void StateChange()
        {
            printf(_T("State change: "));
            if (this->IsOpen()) {
                printf(_T("[6] Open - OK\n"));
            }
            else {
                printf(_T("[6] Closed - %s\n"), (this->IsSuspended() ? _T("SUSPENDED") : _T("OK")));
            }
        }
        virtual bool IsIdle() const
        {
            return (true);
        }
    };
    // -----------------------------------------------------------------------------------------------
    // STRESS TEST SET 8
    // ----------
    // Direct connection for transfering text in an echo fashion.
    // -----------------------------------------------------------------------------------------------
    class StressTextConnector : public Core::StreamTextType<Core::SocketStream, Core::TerminatorCarriageReturn> {
    private:
        typedef Core::StreamTextType<Core::SocketStream, Core::TerminatorCarriageReturn> BaseClass;

        StressTextConnector();
        StressTextConnector(const StressTextConnector& copy);
        StressTextConnector& operator=(const StressTextConnector&);

    public:
        StressTextConnector(const Thunder::Core::NodeId& remoteNode)
            : BaseClass(false, remoteNode.AnyInterface(), remoteNode, 1024, 1024)
            , _serverSocket(false)
            , _pending(0)
            , _errors(0)
            , _sizeReceived(0)
            , _sizeSend(0)
            , _messagesReceived(0)
            , _messagesSend(0)
        {
        }
        StressTextConnector(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<StressTextConnector>*)
            : BaseClass(false, connector, remoteId, 1024, 1024)
            , _serverSocket(true)
            , _pending(0)
            , _errors(0)
            , _sizeReceived(0)
            , _sizeSend(0)
            , _messagesReceived(0)
            , _messagesSend(0)
        {
        }
        ~StressTextConnector() override
        {
            Close(Thunder::Core::infinite);
        }

    public:
        inline uint32_t Pending() const
        {
            return (_pending);
        }
        inline uint32_t Errors() const
        {
            return (_errors);
        }
        inline uint32_t MinSize() const
        {
            return (_factory.MinSize());
        }
        inline uint32_t MaxSize() const
        {
            return (_factory.MaxSize());
        }
        inline uint32_t MessagesReceived() const
        {
            return (_messagesReceived);
        }
        inline uint32_t MessagesSend() const
        {
            return (_messagesSend);
        }
        inline uint32_t SizeReceived() const
        {
            return (static_cast<uint32_t>(_sizeReceived / (1024 * 1024)));
        }
        inline uint32_t SizeSend() const
        {
            return (static_cast<uint32_t>(_sizeSend / (1024 * 1024)));
        }

        void Fire(const uint32_t runCount)
        {
            uint32_t current = 0;

            printf("Firing: %d requests\n", runCount);

            while (current < runCount) {
                string data = _factory.GenerateData();

                Submit(data);

                current++;
            }
        }
        virtual void Received(string& text)
        {
            _messagesReceived++;
            _sizeReceived += text.length();

            if ((_messagesReceived % 1000) == 0) {
                printf("Received: %d on %s\n", _messagesReceived, LocalId().c_str());
            }

            if (_factory.VerifyData(text) == false) {
                Core::InterlockedIncrement(_errors);
                printf("Failure: %s", text.c_str());
            }
            else if (_serverSocket == true) {
                Submit(_factory.GenerateData());
            }

            if (_serverSocket == false) {
                Core::InterlockedDecrement(_pending);
            }
        }
        virtual void Send(const string& text)
        {
            _messagesSend++;
            _sizeSend += text.length();

            if ((_messagesSend % 1000) == 0) {
                printf("Send: %d on %s\n", _messagesSend, LocalId().c_str());
            }

            if (_serverSocket == false) {
                Core::InterlockedIncrement(_pending);
            }
        }
        virtual void StateChange()
        {
            printf(_T("State change: "));
            if (IsOpen()) {
                printf(_T("[8] Open - OK\n"));
            }
            else {
                printf(_T("[8] Closed - %s\n"), (IsSuspended() ? _T("SUSPENDED") : _T("OK")));
            }
        }

    private:
        bool _serverSocket;
        uint32_t _pending;
        uint32_t _errors;
        uint64_t _sizeReceived;
        uint64_t _sizeSend;
        uint32_t _messagesReceived;
        uint32_t _messagesSend;
        StressTextFactory _factory;
    };

    // -----------------------------------------------------------------------------------------------
    // FILE SERVER TEST SET 9
    // ----------
    // Offer a file client to the server
    // -----------------------------------------------------------------------------------------------
    //class FileClientConnector : public Web::ClientTransferType<Core::SocketStream, Web::SignedFileBodyType<Crypto::SHA256HMAC>> {
    class FileClientConnector : public Web::ClientTransferType<Core::SocketStream, Web::SignedFileBodyType<Crypto::SHA256>> {
    private:
        typedef Web::ClientTransferType<Core::SocketStream, Web::SignedFileBodyType<Crypto::SHA256>> BaseClass;

        FileClientConnector(const FileClientConnector&);
        FileClientConnector& operator=(const FileClientConnector&);

    public:
        FileClientConnector()
            : BaseClass(false, Core::NodeId(_T("0.0.0.0")), Core::NodeId(), 1024, 1024) //TODO: Check this!
        {
        }
        ~FileClientConnector() override = default;

    private:
        bool Setup(const Core::URL& remote) override
        {
            bool result = false;

            if (remote.Host().IsSet() == true) {
                uint16_t portNumber(remote.Port().IsSet() ? remote.Port().Value() : 80);

                BaseClass::Link().RemoteNode(Core::NodeId(remote.Host().Value().c_str(), portNumber));

                result = true;
            }
            return (result);
        }
        void Transferred(const uint32_t result, const Web::SignedFileBodyType<Crypto::SHA256>& destination) override
        {
            (void)destination;
            printf(_T("Transfered file with result: %d\n"), result);
        }

        void InfoCollected(const uint32_t result, const Core::ProxyType<Web::Response>& info) override
        {
            (void)result;
            (void)info;
        }
    };
    // -----------------------------------------------------------------------------------------------
    // FILE SERVER TEST SET 9
    // ----------
    // Offer a file server
    // -----------------------------------------------------------------------------------------------
    class FileServerConnector : public Web::ServerTransferType<Core::SocketStream, Web::SignedFileBodyType<Crypto::SHA256> > {
    private:
        inline static const string& PathPrefix()
        {
            static const string result(_T("F:/windows/TestArea/"));

            return (result);
        }

        inline static const string& HashKey()
        {
            static const string result(_T("MySecretTestKey"));

            return (result);
        }

    private:
        FileServerConnector(const FileServerConnector&);
        FileServerConnector& operator=(const FileServerConnector&);

    public:
        FileServerConnector(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<FileServerConnector>*)
            : Web::ServerTransferType<Core::SocketStream, Web::SignedFileBodyType<Crypto::SHA256> >(PathPrefix(), false, connector, remoteId, 1024, 1024)
        {
        }
        ~FileServerConnector() override = default;

    public:
        virtual string Authorize(const Web::Request& request)
        {
            if (request.Verb == Web::Request::HTTP_PUT) {
                printf("Upload request received for [%s]\n", request.Path.c_str());
            }
            else {
                printf("Download request received for [%s]\n", request.Path.c_str());
            }

            return (EMPTY_STRING);
        }
    };

    template <typename CONNECTOR>
    class StressInstanceType : public Core::Thread {
    private:
        StressInstanceType();
        StressInstanceType(const StressInstanceType<CONNECTOR>&);
        StressInstanceType<CONNECTOR>& operator=(const StressInstanceType<CONNECTOR>&);

    public:
        StressInstanceType(const Thunder::Core::NodeId& remoteNode)
            : Thread(0, _T("StressTestEngine"))
            , _connector(remoteNode)
            , _fireSet(0)
        {
            _connector.Open(0);
            TRACE_L1("StressEngine instance: %d", Core::Thread::Id());
        }
        ~StressInstanceType()
        {
            _connector.Close(Thunder::Core::infinite);
        }

    public:
        inline uint32_t Pending() const
        {
            return (_connector.Pending());
        }
        inline uint32_t Errors() const
        {
            return (_connector.Errors());
        }
        inline uint32_t MinSize() const
        {
            return (_connector.MinSize());
        }
        inline uint32_t MaxSize() const
        {
            return (_connector.MaxSize());
        }
        inline uint32_t MessagesReceived() const
        {
            return (_connector.MessagesReceived());
        }
        inline uint32_t MessagesSend() const
        {
            return (_connector.MessagesSend());
        }
        inline uint32_t SizeReceived() const
        {
            return (_connector.SizeReceived());
        }
        inline uint32_t SizeSend() const
        {
            return (_connector.SizeSend());
        }
        inline void Fire(const uint32_t setSize)
        {
            if (IsRunning() == false) {
                _fireSet = setSize;
                Run();
            }
        }
        inline void DirectFire(const uint32_t setSize)
        {
            _connector.Fire(setSize);
        }

    private:
        virtual uint32_t Worker()
        {
            _connector.Fire(_fireSet);

            Block();

            return (Thunder::Core::infinite);
        }

    private:
        CONNECTOR _connector;
        uint32_t _fireSet;
    };
}
} // Namepsace Thunder.TestSystem

#endif
