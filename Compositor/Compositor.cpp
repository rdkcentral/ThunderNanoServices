#include "Compositor.h"

namespace WPEFramework {

namespace Plugin {
    SERVICE_REGISTRATION(Compositor, 1, 0);

    static Core::ProxyPoolType<Web::Response> responseFactory(2);
    static Core::ProxyPoolType<Web::JSONBodyType<Compositor::Data> > jsonResponseFactory(2);

    Compositor::Compositor()
        : _adminLock()
        , _skipURL()
        , _notification(this)
        , _composition(nullptr)
        , _service(nullptr)
        , _pid()
    {
    }

    Compositor::~Compositor()
    {
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

            TRACE(Trace::Information, (_T("XDG_RUNTIME_DIR is set to %s "), runTimeDir));
        }

        _composition = service->Root<Exchange::IComposition>(_pid, 2000, _T("CompositorImplementation"));
        
        if (_composition == nullptr) {
            message = "Instantiating the compositor failed. Could not load: CompositorImplementation";
        }
        else {
            _service = service;

            _notification.Initialize(service, _composition);

            _composition->Configure(_service);
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
    /* virtual */ Core::ProxyType<Web::Response> Compositor::Process(const Web::Request& request) {
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
                Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
                Clients(response->Clients);
                result->Body(Core::proxy_cast<Web::IBody>(response));
            }
            // http://<ip>/Service/Compositor/ZOrder (top to bottom)
            else if (index.Current() == "ZOrder") {
                Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
                ZOrder(response->Clients) ;
                result->Body(Core::proxy_cast<Web::IBody>(response));
            }
            // http://<ip>/Service/Compositor/Resolution
            else if (index.Current() == "Resolution") {
                Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
                response->Resolution = Resolution();
                result->Body(Core::proxy_cast<Web::IBody>(response));
            }

            // http://<ip>/Service/Compositor/Geometry/[Callsign]
            else if (index.Current() == "Geometry") {
                if ( index.Next() == true ) {
                    Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
                    Exchange::IComposition::Rectangle rectangle = Geometry(index.Current().Text());
                    if( rectangle.x == 0 && rectangle.y == 0 && rectangle.width == 0 && rectangle.height == 0 ) {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = _T("Could not retrieve Geometry, could not find Client ") + index.Current().Text();                          
                    }
                    response->X = rectangle.x;
                    response->Y = rectangle.x;
                    response->Width = rectangle.width;
                    response->Height = rectangle.height;
                    result->Body(Core::proxy_cast<Web::IBody>(response));
                }
                else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = _T("Could not retrieve Geometry, Client not specified");                    
                }
            }

            result->ContentType = Web::MIMETypes::MIME_JSON;
        }
        else if (request.Verb == Web::Request::HTTP_POST) {
            Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());

            if (index.Next() == true) {
                if (index.Current() == _T("Resolution")) { /* http://<ip>/Service/Compositor/Resolution/3 --> 720p*/
                    if (index.Next() == true) {
                        Exchange::IComposition::ScreenResolution format (Exchange::IComposition::ScreenResolution_Unknown);
                        uint32_t number(Core::NumberType<uint32_t>(index.Current()).Value());

                        if ((number != 0) && (number < 100)) {
                             format = static_cast<Exchange::IComposition::ScreenResolution>(number);
                        }
                        else {
                            Core::EnumerateType<Exchange::IComposition::ScreenResolution> value (index.Current());

                            if (value.IsSet() == true) {
                                format = value.Value();
                            }
                        }
                        if (format != Exchange::IComposition::ScreenResolution_Unknown) {
                            Resolution(format);
                        }
                        else {
                            result->ErrorCode = Web::STATUS_BAD_REQUEST;
                            result->Message = _T("invalid parameter for resolution: ") + index.Current().Text();
                        }
                    }
                }
                else {
                    string clientName(index.Current().Text());

                    if( clientName.empty() == true )
                    {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = string(_T("Client was not provided (empty)"));
                    }
                    else {

                        if (index.Next() == true) {
                            uint32_t error = Core::ERROR_NONE;
                            if (index.Current() == _T("Kill")) { /* http://<ip>/Service/Compositor/Netflix/Kill */
                                error = Kill(clientName);
                            }
                            else if (index.Current() == _T("Opacity") &&
                                index.Next() == true) { /* http://<ip>/Service/Compositor/Netflix/Opacity/128 */
                                const uint32_t opacity(std::stoi(index.Current().Text()));
                                error = Opacity(clientName, opacity);
                            }
                            else if (index.Current() == _T("Visible") &&
                                index.Next() == true) { /* http://<ip>/Service/Compositor/Netflix/Visible/Hide  or Show */
                                if (index.Current() == _T("Hide")) {
                                    error = Visible(clientName, false);
                                } else if (index.Current() == _T("Show")) {
                                    error = Visible(clientName, true);
                                }           
                            }
                            else if (index.Current() == _T("Geometry")) { /* http://<ip>/Service/Compositor/Netflix/Geometry/0/0/1280/720 */
                                Exchange::IComposition::Rectangle rectangle = Exchange::IComposition::Rectangle();

                                uint32_t rectangleError = Core::ERROR_INCORRECT_URL;

                                if (index.Next() == true) {
                                    rectangle.x = Core::NumberType<uint32_t>(index.Current()).Value();
                                }
                                if (index.Next() == true) {
                                    rectangle.y = Core::NumberType<uint32_t>(index.Current()).Value();
                                }
                                if (index.Next() == true) {
                                    rectangle.width  = Core::NumberType<uint32_t>(index.Current()).Value();
                                }
                                if (index.Next() == true) {
                                    rectangle.height = Core::NumberType<uint32_t>(index.Current()).Value();
                                    rectangleError = Core::ERROR_NONE;
                                }

                                if ( rectangleError == Core::ERROR_NONE ) {
                                    error = Geometry(clientName, rectangle);
                                }
                                else {
                                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                                    result->Message = string(_T("Could not set rectangle for Client ")) + clientName + _T(". Not all required information provided");
                                }

                                error = Geometry(clientName, rectangle);
                            }
                        else if (index.Current() == _T("Top")) { /* http://<ip>/Service/Compositor/Netflix/Top */
                                error = ToTop(clientName);
                            }
                            else if (index.Current() == _T("PutBelow")) { /* http://<ip>/Service/Compositor/Netflix/PutBelow/Youtube */
                                if (index.Next() == true) {
                                    error = PutBelow(index.Current().Text(), clientName);
                                    if( error != Core::ERROR_NONE ) {
                                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                                        result->Message = string(_T("Could not change z-order for Client "));
                                        if ( error == Core::ERROR_FIRST_RESOURCE_NOT_FOUND ) {
                                            result->Message += _T(". Client is not registered");
                                        }
                                        else if ( error == Core::ERROR_SECOND_RESOURCE_NOT_FOUND ) {
                                            result->Message += _T(". Client relative to which the operation should be executed is not registered");
                                        } 
                                        else {
                                            result->Message += _T(". Unspecified problem");
                                        }
                                    }
                                }
                                else {
                                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                                    result->Message = string(_T("Could not change z-order for Client ")) + clientName + _T(". Not specified relative to which Client.");
                                }
                            }

                            if( error != Core::ERROR_NONE && result->ErrorCode == Web::STATUS_OK ) {
                                if ( error == Core::ERROR_FIRST_RESOURCE_NOT_FOUND ) {
                                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                                    result->Message = string(_T("Client ")) + clientName + _T(" is not registered.");                                
                                }
                                else {
                                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                                    result->Message = string(_T("Unspecified error"));                                
                                }
                            }
                        }
                        else {
                            result->ErrorCode = Web::STATUS_BAD_REQUEST;
                            result->Message = string(_T("Client is not provided"));
                        
                        }
                    }
                }
            }

            result->ContentType = Web::MIMETypes::MIME_JSON;
            result->Body(Core::proxy_cast<Web::IBody>(response));
        }
        else {
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = string(_T("Unsupported request for the service."));
        }
        // <PUT> ../Server/[start,stop]
        return result;
    }

    void Compositor::Attached(Exchange::IComposition::IClient* client) {
        ASSERT(client != nullptr);

        if ( client != nullptr ) {
            string name(client->Name());

            _adminLock.Lock();
            std::map<string, Exchange::IComposition::IClient*>::iterator it = _clients.find(name);

            if (it != _clients.end()) {
                TRACE(Trace::Information, (_T("Client %s was already attached, old instance removed"), name.c_str()));
                it->second->Release();
                _clients.erase(it);
            }

            _clients[name] = client;
            client->AddRef();

            _adminLock.Unlock();

            TRACE(Trace::Information, (_T("Client %s attached"), name.c_str()));
        }
    }

    void Compositor::Detached(Exchange::IComposition::IClient* client) {
        // note do not release by looking up the name, client might live in another process and the name call might fail if the connection is gone
        ASSERT(client != nullptr);

        TRACE(Trace::Information, (_T("Client detached starting")));
        Exchange::IComposition::IClient* removedclient = nullptr;

        _adminLock.Lock();
        auto it = _clients.begin();
        while( it != _clients.end() ) {
            if( it->second == client )
            {
                removedclient = it->second;
                TRACE(Trace::Information, (_T("Client %s detached"), it->first));
                _clients.erase(it);
                break;
            }
            ++it;
        }
        _adminLock.Unlock();

//        if( removedclient != nullptr ) {
//            removedclient->Release();
//        }

        TRACE(Trace::Information, (_T("Client detached completed")));
    }

    template<typename ClientOperation>
    uint32_t Compositor::CallOnClientByCallsign(const string& callsign, ClientOperation&& operation) const {
        ASSERT(_composition != nullptr);   

        uint32_t error = Core::ERROR_NONE;

        if (_composition != nullptr) {
            Exchange::IComposition::IClient* client;

            _adminLock.Lock();
            auto it = _clients.find(callsign);

            if (it != _clients.end()) {
                client = it->second;
                ASSERT(client != nullptr);
                client->AddRef();
            }
            else {
                error = Core::ERROR_FIRST_RESOURCE_NOT_FOUND;
                TRACE(Trace::Information, (_T("Client %s not found in CallOnClientByCallsign."), callsign.c_str()));
            }
            _adminLock.Unlock();
            std::forward<ClientOperation>(operation)(*client);
            client->Release();
        }
        return error;
    }

    void Compositor::Clients(Core::JSON::ArrayType<Core::JSON::String>& callsigns) const {
        _adminLock.Lock();
        for (auto const& client : _clients) {
            TRACE(Trace::Information, (_T("Client %s added to the JSON array"), client.first.c_str()));
            Core::JSON::String& element(callsigns.Add());
            element = client.first;
        }
        _adminLock.Unlock();
    }

    uint32_t Compositor::Kill(const string& callsign) const {
        return CallOnClientByCallsign(callsign, [](Exchange::IComposition::IClient& client) { client.Kill(); });
    }

    uint32_t Compositor::Opacity(const string& callsign, const uint32_t value) const  {
        return CallOnClientByCallsign(callsign, [&](Exchange::IComposition::IClient& client) { client.Opacity(value); } );    
    }

    void Compositor::Resolution(const Exchange::IComposition::ScreenResolution format) {
        ASSERT(_composition != nullptr);

        if (_composition != nullptr) {
            _composition->Resolution(format);
        }
    }

   Exchange::IComposition::ScreenResolution  Compositor::Resolution() const {
        ASSERT(_composition != nullptr);

        if (_composition != nullptr) {
            return (_composition->Resolution());
        }
        return Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown;
    }

    uint32_t Compositor::Visible(const string& callsign, const bool visible) const {
        return Opacity(callsign, visible == true ? Exchange::IComposition::maxOpacity : Exchange::IComposition::minOpacity );
    }

    uint32_t Compositor::Geometry(const string& callsign, const Exchange::IComposition::Rectangle& rectangle) {
        ASSERT(_composition != nullptr);

        uint32_t error = Core::ERROR_NONE;

        if (_composition != nullptr) {
           error = _composition->Geometry(callsign, rectangle); 
        }

        return error;
    }

    Exchange::IComposition::Rectangle Compositor::Geometry(const string& callsign) const {
        Exchange::IComposition::Rectangle rectangle = Exchange::IComposition::Rectangle();

        ASSERT(_composition != nullptr);

        if (_composition != nullptr) {
            rectangle = _composition->Geometry(callsign); 
        }
        return rectangle;
    }

    uint32_t Compositor::ToTop(const string& callsign) {
        ASSERT(_composition != nullptr);

        uint32_t error = Core::ERROR_NONE;

        if (_composition != nullptr) {
            error = _composition->ToTop(callsign); 
        }
        return error;    
    }

    uint32_t Compositor::PutBelow(const string& callsignRelativeTo, const string& callsignToReorder) {
         ASSERT(_composition != nullptr);

         ASSERT(false); // not implemeted for now, we'll do that when it is needed

         return Core::ERROR_UNAVAILABLE;

        uint32_t error = Core::ERROR_NONE;

        if (_composition != nullptr) {
            error = _composition->PutBelow(callsignRelativeTo, callsignToReorder); 
        }
        return error;             
    }

    void Compositor::ZOrder(Core::JSON::ArrayType<Core::JSON::String>& callsigns) const {
        ASSERT(_composition != nullptr);
        RPC::IStringIterator* iterator = _composition->ClientsInZorder();

        ASSERT(iterator != nullptr)

        if(iterator != nullptr) {
            while (iterator->Next() == true) {
                Core::JSON::String& element(callsigns.Add());
                element = iterator->Current();
            }
            iterator->Release();
        }
    }


} // namespace Plugin
} // namespace WPEFramework
