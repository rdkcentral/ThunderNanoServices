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
 
#include "InputSwitch.h"
#include <interfaces/json/JsonData_InputSwitch.h>

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(InputSwitch, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType< Core::JSON::ArrayType < InputSwitch::Data > > > jsonResponseFactory(1);

    /* virtual */ const string InputSwitch::Initialize(PluginHost::IShell* service)
    {
        string message;

        ASSERT(service != nullptr);
        ASSERT(_handler == nullptr);

        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
        _handler = PluginHost::InputHandler::Handler();

        if (dynamic_cast<PluginHost::IPCUserInput*>(_handler) == nullptr) {
            message = _T("This plugin requires the VirtualInput (IPC Relay) to be instantiated");
        }

        // On success return empty, to indicate there is no error text.
        return (EMPTY_STRING);
    }

    /* virtual */ void InputSwitch::Deinitialize(PluginHost::IShell* service)
    {
        _handler = nullptr;
    }

    /* virtual */ string InputSwitch::Information() const
    {
        // No additional info to report.
        return (EMPTY_STRING);
    }

    /* virtual */ void InputSwitch::Inbound(Web::Request& /* request */)
    {
    }

    /* virtual */ Core::ProxyType<Web::Response> InputSwitch::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());

        // By default, we assume everything works..
        result->ErrorCode = Web::STATUS_OK;
        result->Message = "OK";

        // <GET> - currently, only the GET command is supported, returning system info
        if (request.Verb == Web::Request::HTTP_GET) {

            Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length()) - _skipURL), false, '/');

            // Always skip the first one, it is an empty part because we start with a '/' if there are more parameters.
            index.Next();

            if (index.Next() == false) {
                Core::ProxyType<Web::JSONBodyType< Core::JSON::ArrayType <Data> > > response(jsonResponseFactory.Element());

                // Insert all channels with there status..
                PluginHost::VirtualInput::Iterator index (_handler->Consumers());
                while (index.Next() == true) {
                    Data& element (response->Add());

                    const string& callsign (index.Name());

                    element.Callsign = callsign;
                    element.Enabled = Consumer(callsign);
                }

                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(Core::proxy_cast<Web::IBody>(response));
            }
            else {
                string name (index.Current().Text());
                if (ChannelExists(name) == false) {
                    result->ErrorCode = Web::STATUS_MOVED_TEMPORARY;
                    result->Message = _T("Could not find the inidicated channel.");
                }
                else {
                    Core::ProxyType<Web::JSONBodyType< Core::JSON::ArrayType <Data> > > response(jsonResponseFactory.Element());

                    // Insert the requested channel with its status..
                    Data& element(response->Add());

                    element.Callsign = name;
                    element.Enabled = Consumer(name);

                    result->ContentType = Web::MIMETypes::MIME_JSON;
                    result->Body(Core::proxy_cast<Web::IBody>(response));
                }
            }
        }
        else if (request.Verb == Web::Request::HTTP_PUT) {
            Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length()) - _skipURL), false, '/');

            // Always skip the first one, it is an empty part because we start with a '/' if there are more parameters.
            index.Next();

            if (index.Next() == false) {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = _T("Need at least a channel name and a requested state.");
            }
            else if (ChannelExists(index.Current().Text()) == false) {
                result->ErrorCode = Web::STATUS_MOVED_TEMPORARY;
                result->Message = _T("Could not find the indicated channel.");
            }
            else {
                string name (index.Current().Text());

                if (index.Next() == false) {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = _T("Need at least a state wich is applicable to the channel.");
                }
                else if ((index.Remainder() != _T("On")) || (index.Remainder() != _T("Off")) || (index.Remainder() != _T("Slave"))) {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = _T("The requested state should be <On>, <Off> or <Slave>.");
                }
                else {
                    Core::ProxyType<Web::JSONBodyType< Core::JSON::ArrayType <Data> > > response(jsonResponseFactory.Element());

                    // Insert the requested channel with its status..
                    Data& element(response->Add());
                    Exchange::IInputSwitch::mode mode = (index.Remainder() == _T("On")  ? 
                                                         Exchange::IInputSwitch::ENABLED : 
                                                         (index.Remainder() == _T("Slave")   ? 
                                                          Exchange::IInputSwitch::SLAVE      :
                                                          Exchange::IInputSwitch::DISABLED) );
                    Consumer(name, mode);

                    element.Callsign = name;
                    element.Enabled  = (mode == Exchange::IInputSwitch::ENABLED);

                    result->ContentType = Web::MIMETypes::MIME_JSON;
                    result->Body(Core::proxy_cast<Web::IBody>(response));
                }
            }
        } else {
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = _T("Unsupported request for the [InputSwitch] service.");
        }

        return result;
    }

    bool InputSwitch::ChannelExists(const string& name) const {
        PluginHost::VirtualInput::Iterator index (_handler->Consumers());
        while ( (index.Next() == true) && (index.Name() != name) ) /* INTENTIONALLY LEFT EMPTY */ ;
        return (index.IsValid());
    }

    RPC::IStringIterator* InputSwitch::Consumers() const /* override */ {
        return (Core::Service<RPC::StringIterator>::Create<RPC::IStringIterator>(_handler->Consumers().Container()));
    }

    bool InputSwitch::Consumer(const string& name) const /* override */ {
        return(_handler->Consumer(name));
    }

    uint32_t InputSwitch::Consumer(const string& name, const Exchange::IInputSwitch::mode value) /* override */ {
        ImunityList::iterator index (std::find(_imunityList.begin(), _imunityList.end(), name));
        
        if (value == Exchange::IInputSwitch::SLAVE) {
            // Remove it from the list of agnostic consumers.
            if (index != _imunityList.end()) {
                _imunityList.erase(index);
                _handler->Consumer(name, false);
            }
        }
        else {
            // Add it to the list
            if (index == _imunityList.end()) {
                _imunityList.push_front(name);
            }
            _handler->Consumer(name, (value == Exchange::IInputSwitch::ENABLED));
        }

        return (Core::ERROR_NONE);
    }

    uint32_t InputSwitch::Select(const string& name) /* override */ {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        PluginHost::VirtualInput::Iterator index (_handler->Consumers());
        while (index.Next() == true) {
            const string& current(index.Name());
            ImunityList::iterator index (std::find(_imunityList.begin(), _imunityList.end(), current));
            if (name == current) {
                if (index != _imunityList.end()) {
                    _imunityList.erase(index);
                }
                _handler->Consumer(current, true);
                result = Core::ERROR_NONE;
            }
            else if (index == _imunityList.end()) {
                // Seems the consumer has no imunity :-)
                _handler->Consumer(current, false);
            }
        }

        return (result);
    }

} // namespace Plugin
} // namespace WPEFramework
