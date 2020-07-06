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
 
#include "Compositor.h"
#include <interfaces/IInputSwitch.h>

namespace WPEFramework {

namespace Plugin {

    SERVICE_REGISTRATION(Compositor, 1, 0);

    static string PrimaryName(const string& layerName) 
    {
        size_t pos = layerName.find(':', 0);
        if (pos != string::npos) {
            return(layerName.substr(0, pos));
        }
        return (layerName);
    }

    struct client_info {
        uint16_t layer;
        string   name;
        Exchange::IComposition::IClient* access;
    };

    static void SetZOrderList (std::list<client_info>& list, uint16_t index = 0) {

        // Time to set the new ZOrder. All effected clients have been listed...
        std::list<client_info>::iterator loop(list.begin());
        while (loop != list.end()) {
            loop->access->ZOrder(index);
            index++;
            loop++;
        }
    }

    static void GetZOrderList(const Compositor::Clients& entries, std::list<client_info>& list) {
        Compositor::Clients::const_iterator index (entries.cbegin());
        while (index != entries.cend()) {
            client_info entry = { static_cast<uint16_t>(index->second->ZOrder()), index->first, index->second };

            std::list<client_info>::iterator loop (list.begin());

            while ((loop != list.end()) && (loop->layer <= entry.layer)) {
                loop++;
            }
            if (loop == list.end()) {
                list.push_back(entry);
            }
            else {
                list.insert(loop, entry);
            }
            index++;
        }
    }

    static void Rearrange(std::list<client_info>& list, uint16_t index, uint16_t location, uint16_t start, uint16_t end) {
        // Now we have all the information, reconstruct the zOrder branch..
        std::list<client_info> set;

        ASSERT (location != start);

        // Now create a list with the items swapped.
        if (location == 0) {

            // Moving the callsign up in the order
            std::list<client_info>::iterator loop(list.begin());

            while (loop != list.end()) {
                if (start != 0) { 
                    start--; 
                    loop++; 
                }
                else if (end   != 0) { 
                    end--; 
                    set.push_back(*loop); 
                    loop = list.erase(loop); 
                }
                else { 
                    loop = list.erase(loop); 
                }
            }

            // Now push the safed entries in front, in the same order :-)
            set.splice(set.end(), list);
                
            SetZOrderList(set, index);
        }
        else {
            ASSERT (location >= end);

            // Moving the callsign down in the order
            std::list<client_info>::iterator loop(list.begin());
               
            while (loop != list.end()) {
                if (end != 0) { 
                    end--;
                    location--;
                    set.push_back(*loop);
                    loop = list.erase(loop); 
                }
                else if (location != 0) { 
                    location--; 
                    loop++;
                }
                else {
                    loop = list.erase(loop); 
                }
            }

            // If we are already in the right order, I guess there is nothing to do :-)
            if (list.size() != 0) {

                // Now push the safed entries at the back, in the same order :-)
                list.splice(list.end(), set);

                SetZOrderList(list, index);
            }
        }
    }

    static Core::ProxyPoolType<Web::Response> responseFactory(2);
    static Core::ProxyPoolType<Web::JSONBodyType<Compositor::Data>> jsonResponseFactory(2);

    Compositor::Compositor()
        : _adminLock()
        , _skipURL()
        , _notification(this)
        , _composition(nullptr)
        , _service(nullptr)
        , _connectionId()
        , _inputSwitch(nullptr)
        , _inputSwitchCallsign()

    {
        RegisterAll();
    }

    Compositor::~Compositor()
    {
        UnregisterAll();
    }

    /* virtual */ const string Compositor::Initialize(PluginHost::IShell* service)
    {
        string message(EMPTY_STRING);
        string result;

        ASSERT(service != nullptr);

        Compositor::Config config;
        config.FromString(service->ConfigLine());

        _skipURL = service->WebPrefix().length();

        // See if the mandatory XDG environment variable is set, otherwise we will set it.
        if (Core::SystemInfo::GetEnvironment(_T("XDG_RUNTIME_DIR"), result) == false) {

            string runTimeDir((config.WorkDir.Value()[0] == '/') ? config.WorkDir.Value() : service->PersistentPath() + config.WorkDir.Value());

            Core::SystemInfo::SetEnvironment(_T("XDG_RUNTIME_DIR"), runTimeDir);

            TRACE(Trace::Information, (_T("XDG_RUNTIME_DIR is set to %s "), runTimeDir.c_str()));
        }

        _composition = service->Root<Exchange::IComposition>(_connectionId, 2000, _T("CompositorImplementation"));

        if (_composition == nullptr) {
            message = "Instantiating the compositor failed. Could not load: CompositorImplementation";
        } else {
            _service = service;

            _notification.Initialize(service, _composition);

            _composition->Configure(_service);

            _inputSwitch = _composition->QueryInterface<Exchange::IInputSwitch>();
            _inputSwitchCallsign = config.InputSwitch.Value();
        }

        // On succes return empty, to indicate there is no error text.
        return message;
    }
    /* virtual */ void Compositor::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service == _service);

        // We would actually need to handle setting the Graphics event in the CompositorImplementation. For now, we do it here.
        PluginHost::ISubSystem* subSystems = _service->SubSystems();

        ASSERT(subSystems != nullptr);

        if (subSystems != nullptr) {
            // Set Graphics event. We need to set up a handler for this at a later moment
            subSystems->Set(PluginHost::ISubSystem::NOT_GRAPHICS, nullptr);
            subSystems->Release();
        }

        if (_inputSwitch != nullptr) {
            _inputSwitch->Release();
            _inputSwitch = nullptr;
        }
        if (_composition != nullptr) {
            _composition->Release();
            _composition = nullptr;
        }

        _notification.Deinitialize();
    }
    /* virtual */ string Compositor::Information() const
    {
        // No additional info to report.
        return (_T(""));
    }

    /* virtual */ void Compositor::Inbound(Web::Request& /* request */)
    {
    }
    /* virtual */ Core::ProxyType<Web::Response> Compositor::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result(responseFactory.Element());
        Core::TextSegmentIterator
            index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

        // If there is an entry, the first one will always be a '/', skip this one..
        index.Next();

        result->ErrorCode = Web::STATUS_OK;
        result->Message = "OK";

        // <GET> ../
        if (request.Verb == Web::Request::HTTP_GET) {

            // http://<ip>/Service/Compositor/
            // http://<ip>/Service/Compositor/Clients
            if (index.Next() == false || index.Current() == "Clients") {
                Core::ProxyType<Web::JSONBodyType<Data>> response(jsonResponseFactory.Element());
                ZOrder(response->Clients);
                result->Body(Core::proxy_cast<Web::IBody>(response));
            }
            // http://<ip>/Service/Compositor/ZOrder (top to bottom)
            else if (index.Current() == "ZOrder") {
                Core::ProxyType<Web::JSONBodyType<Data>> response(jsonResponseFactory.Element());
                ZOrder(response->Clients);
                result->Body(Core::proxy_cast<Web::IBody>(response));
            }
            // http://<ip>/Service/Compositor/Resolution
            else if (index.Current() == "Resolution") {
                Core::ProxyType<Web::JSONBodyType<Data>> response(jsonResponseFactory.Element());
                response->Resolution = Resolution();
                result->Body(Core::proxy_cast<Web::IBody>(response));
            }

            // http://<ip>/Service/Compositor/Geometry/[Callsign]
            else if (index.Current() == "Geometry") {
                if (index.Next() == true) {
                    Core::ProxyType<Web::JSONBodyType<Data>> response(jsonResponseFactory.Element());
                    Exchange::IComposition::Rectangle rectangle = Geometry(index.Current().Text());
                    if (rectangle.x == 0 && rectangle.y == 0 && rectangle.width == 0 && rectangle.height == 0) {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = _T("Could not retrieve Geometry, could not find Client ") + index.Current().Text();
                    }
                    response->X = rectangle.x;
                    response->Y = rectangle.x;
                    response->Width = rectangle.width;
                    response->Height = rectangle.height;
                    result->Body(Core::proxy_cast<Web::IBody>(response));
                } else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = _T("Could not retrieve Geometry, Client not specified");
                }
            }

            result->ContentType = Web::MIMETypes::MIME_JSON;
        } else if (request.Verb == Web::Request::HTTP_POST) {
            Core::ProxyType<Web::JSONBodyType<Data>> response(jsonResponseFactory.Element());

            if (index.Next() == true) {
                if (index.Current() == _T("Resolution")) { /* http://<ip>/Service/Compositor/Resolution/3 --> 720p*/
                    if (index.Next() == true) {
                        Exchange::IComposition::ScreenResolution format(Exchange::IComposition::ScreenResolution_Unknown);
                        uint32_t number(Core::NumberType<uint32_t>(index.Current()).Value());

                        if ((number != 0) && (number < 100)) {
                            format = static_cast<Exchange::IComposition::ScreenResolution>(number);
                        } else {
                            Core::EnumerateType<Exchange::IComposition::ScreenResolution> value(index.Current());

                            if (value.IsSet() == true) {
                                format = value.Value();
                            }
                        }
                        if (format != Exchange::IComposition::ScreenResolution_Unknown) {
                            Resolution(format);
                        } else {
                            result->ErrorCode = Web::STATUS_BAD_REQUEST;
                            result->Message = _T("invalid parameter for resolution: ") + index.Current().Text();
                        }
                    }
                } else {
                    string clientName(index.Current().Text());

                    if (clientName.empty() == true) {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = string(_T("Client was not provided (empty)"));
                    } else {

                        if (index.Next() == true) {
                            uint32_t error = Core::ERROR_NONE;
                            if (index.Current() == _T("Opacity") && index.Next() == true) { /* http://<ip>/Service/Compositor/Netflix/Opacity/128 */
                                const uint32_t opacity(std::stoi(index.Current().Text()));
                                error = Opacity(clientName, opacity);
                            } else if (index.Current() == _T("Visible") && index.Next() == true) { /* http://<ip>/Service/Compositor/Netflix/Visible/Hide  or Show */
                                if (index.Current() == _T("Hide")) {
                                    error = Visible(clientName, false);
                                } else if (index.Current() == _T("Show")) {
                                    error = Visible(clientName, true);
                                }
                            } else if (index.Current() == _T("Geometry")) { /* http://<ip>/Service/Compositor/Netflix/Geometry/0/0/1280/720 */
                                Exchange::IComposition::Rectangle rectangle = Exchange::IComposition::Rectangle();

                                uint32_t rectangleError = Core::ERROR_INCORRECT_URL;

                                if (index.Next() == true) {
                                    rectangle.x = Core::NumberType<uint32_t>(index.Current()).Value();
                                }
                                if (index.Next() == true) {
                                    rectangle.y = Core::NumberType<uint32_t>(index.Current()).Value();
                                }
                                if (index.Next() == true) {
                                    rectangle.width = Core::NumberType<uint32_t>(index.Current()).Value();
                                }
                                if (index.Next() == true) {
                                    rectangle.height = Core::NumberType<uint32_t>(index.Current()).Value();
                                    rectangleError = Core::ERROR_NONE;
                                }

                                if (rectangleError == Core::ERROR_NONE) {
                                    error = Geometry(clientName, rectangle);
                                } else {
                                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                                    result->Message = string(_T("Could not set rectangle for Client ")) + clientName + _T(". Not all required information provided");
                                }

                                error = Geometry(clientName, rectangle);
                            } else if (index.Current() == _T("Top")) { /* http://<ip>/Service/Compositor/Netflix/Top */
                                error = ToTop(clientName);
                            } 

                            if (error != Core::ERROR_NONE && result->ErrorCode == Web::STATUS_OK) {
                                if (error == Core::ERROR_UNAVAILABLE) {
                                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                                    result->Message = string(_T("Client ")) + clientName + _T(" is not registered.");
                                } else {
                                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                                    result->Message = string(_T("Unspecified error"));
                                }
                            }
                        } else {
                            result->ErrorCode = Web::STATUS_BAD_REQUEST;
                            result->Message = string(_T("Client is not provided"));
                        }
                    }
                }
            }

            result->ContentType = Web::MIMETypes::MIME_JSON;
            result->Body(Core::proxy_cast<Web::IBody>(response));
        } else {
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = string(_T("Unsupported request for the service."));
        }
        // <PUT> ../Server/[start,stop]
        return result;
    }

    void Compositor::Attached(const string& name, Exchange::IComposition::IClient* client)
    {
        ASSERT(client != nullptr);

        if (client != nullptr) {
            client_info entry = { 0, name, client };
            std::list<client_info> list;

            _adminLock.Lock();

            Clients::iterator it = _clients.find(name);

            if (it != _clients.end()) {
                TRACE(Trace::Information, (_T("Client %s was already attached, old instance removed"), name.c_str()));
                it->second->Release();
                _clients.erase(it);
            }

            GetZOrderList(_clients, list);

            list.push_front(entry);

            SetZOrderList(list, 0);

            _clients[name] = client;

            client->AddRef();

            _adminLock.Unlock();

            TRACE(Trace::Information, (_T("Client %s attached"), name.c_str()));
        }
    }

    void Compositor::Detached(const string& name)
    {
        // note do not release by looking up the name, client might live in another process and the name call might fail if the connection is gone
        _adminLock.Lock();

        Clients::iterator it = _clients.find(name);
        if (it != _clients.end()) {

            Exchange::IComposition::IClient* removedclient = it->second;
            
            TRACE(Trace::Information, (_T("Client %s detached"), it->first.c_str()));
            _clients.erase(it);

            removedclient->Release();
        }

        _adminLock.Unlock();
    }

    uint32_t Compositor::Resolution(const Exchange::IComposition::ScreenResolution format)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        ASSERT(_composition != nullptr);

        if (_composition != nullptr) {
            result = _composition->Resolution(format);

            TRACE(Trace::Information, (_T("Current resolution of the screen is set to %d"), format));
        }

        return (result);
    }

    Exchange::IComposition::ScreenResolution Compositor::Resolution() const
    {
        ASSERT(_composition != nullptr);

        if (_composition != nullptr) {
            return (_composition->Resolution());
        }
        return Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown;
    }

    void Compositor::ZOrder(std::list<string>& zOrderedList, const bool primary) const {

        std::list<client_info> list;

        _adminLock.Lock();

        GetZOrderList(_clients, list);

        // Now copy the sorted list to the actual list..
        std::list<client_info>::const_iterator loop (list.begin());
        while (loop != list.end()) {
            if (primary == false) {
                zOrderedList.push_back(loop->name);
            }
            else {
                string layerName = PrimaryName(loop->name);
                if (std::find(zOrderedList.begin(), zOrderedList.end(), layerName) == zOrderedList.end()) {
                    zOrderedList.push_back(layerName);
                }
            }
                
            loop++;
        }

        _adminLock.Unlock();
    }

    uint32_t Compositor::Opacity(const string& callsign, const uint32_t value)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        Clients::iterator it = _clients.begin();

        while (it != _clients.end()) {
            string current (PrimaryName(it->first));
            if (callsign == current) {
                it->second->Opacity(value);
                result = Core::ERROR_NONE;

                TRACE(Trace::Information, (_T("Opacity level %d is set for client surface %s"), value, callsign.c_str()));
            }
            it++;
        }

        _adminLock.Unlock();

        return (result);
    }

    uint32_t Compositor::Visible(const string& callsign, const bool visible)
    {
        if (visible) {
            TRACE(Trace::Information, (_T("Client surface %s is set to visible"), callsign.c_str()));
        } else {
            TRACE(Trace::Information, (_T("Client surface %s is set to hidden"), callsign.c_str()));
        }
        return (Opacity(callsign, visible == true ? Exchange::IComposition::maxOpacity : Exchange::IComposition::minOpacity));
    }

    uint32_t Compositor::Geometry(const string& callsign, const Exchange::IComposition::Rectangle& rectangle)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        Clients::iterator it = _clients.begin();

        while (it != _clients.end()) {
            string current (PrimaryName(it->first));
            if (callsign == current) {
                it->second->Geometry(rectangle);
                result = Core::ERROR_NONE;

                TRACE(Trace::Information, (_T("Geometry x=%d y=%d width=%d height=%d is set for client surface %s"), rectangle.x, rectangle.y, rectangle.width, rectangle.height, callsign.c_str()));
            }
            it++;
        }

        _adminLock.Unlock();

        return (result);
    }

    Exchange::IComposition::Rectangle Compositor::Geometry(const string& callsign) const
    {
        Exchange::IComposition::Rectangle result { 0,0,0,0 };
        Exchange::IComposition::IClient* client(InterfaceByCallsign(callsign));

        if (client != nullptr) {
            result = client->Geometry();
            client->Release();
        }

        return (result);
    }

    uint32_t Compositor::Select(const string& callsign) {

        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_inputSwitch) {
            result = _inputSwitch->Select(callsign);

            TRACE(Trace::Information, (_T("Input is directed to client %s"), callsign.c_str()));
        }
        else {
            Exchange::IInputSwitch* switcher = _service->QueryInterfaceByCallsign<Exchange::IInputSwitch>(_inputSwitchCallsign);
            if (switcher != nullptr) {
                result = switcher->Select(callsign);
                switcher->Release();

                TRACE(Trace::Information, (_T("Input is directed to client %s"), callsign.c_str()));
            }
        }
        return (result);
    }

    uint32_t Compositor::PutBefore(const string& relative, const string& callsign) {
        uint32_t result = Core::ERROR_NONE;

        std::list<client_info> list;

        uint16_t relativeIndex      (relative.empty() ? 0 : ~0);
        uint16_t callsignStartIndex (~0);
        uint16_t callsignEndIndex   (~0);

        _adminLock.Lock();

        GetZOrderList(_clients, list);

        // Find the index of what we need to move
        uint32_t current = 0;
        std::list<client_info>::iterator loop (list.begin());

        while (loop != list.end()) {
            string layerName (PrimaryName(loop->name));

            if ( (relativeIndex == static_cast<uint16_t>(~0)) && (layerName == relative) ) {
                relativeIndex = current;
            }
            else if (layerName == callsign) {
                if (callsignStartIndex == static_cast<uint16_t>(~0)) {
                    callsignStartIndex  = current;
                }
                callsignEndIndex = current;
            }
            current++;
            loop++;
        }

        // Now we have all the information, reconstruct the zOrder branch..
        if ((relativeIndex != static_cast<uint16_t>(~0)) && (callsignStartIndex != static_cast<uint16_t>(~0)) && (relativeIndex != callsignStartIndex)) {
            uint16_t startIndex = std::min(relativeIndex, callsignStartIndex);

            // We have something we can move. First remove the stuff in front that does not move..
            for (uint16_t current = 0; current < startIndex; current++) {
                list.pop_front();
            }

            if (relativeIndex < callsignStartIndex) {
                Rearrange(list, startIndex, 0, callsignStartIndex - startIndex, callsignEndIndex - callsignStartIndex + 1);
            }
            else {
                Rearrange(list, startIndex, relativeIndex - startIndex, 0, callsignEndIndex - callsignStartIndex + 1);
            }
            TRACE(Trace::Information, (_T("Client surface %s is put below surface %s"), callsign.c_str(), relative.c_str()));

        } else {
            result = Core::ERROR_UNAVAILABLE;
        }
 
        _adminLock.Unlock();
 
        return (result);
    }

    uint32_t Compositor::ToTop(const string& callsign)
    {
        return PutBefore(EMPTY_STRING, callsign);
    }

    Exchange::IComposition::IClient* Compositor::InterfaceByCallsign(const string& callsign) const
    {
        Exchange::IComposition::IClient* client = nullptr;

        _adminLock.Lock();

        Clients::const_iterator it = _clients.cbegin();

        while ((client == nullptr) && (it != _clients.cend())) {
            if (callsign == PrimaryName(it->first)) {
                client = it->second;
                ASSERT(client != nullptr);
                client->AddRef();
            }
            it++;
        }

        _adminLock.Unlock();

        return client;
    }

} // namespace Plugin
} // namespace WPEFramework
