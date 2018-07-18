#include "Compositor.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Exchange::IComposition::ScreenResolution)

    { Exchange::IComposition::ScreenResolution_Unknown,   _TXT("Unknown")   },
    { Exchange::IComposition::ScreenResolution_480i,      _TXT("480i")      },
    { Exchange::IComposition::ScreenResolution_480p,      _TXT("480p")      },
    { Exchange::IComposition::ScreenResolution_720p,      _TXT("720p")      },
    { Exchange::IComposition::ScreenResolution_720p50Hz,  _TXT("720p50Hz")  },
    { Exchange::IComposition::ScreenResolution_1080p24Hz, _TXT("1080p24Hz") },
    { Exchange::IComposition::ScreenResolution_1080i50Hz, _TXT("1080i50Hz") },
    { Exchange::IComposition::ScreenResolution_1080p50Hz, _TXT("1080p50Hz") },
    { Exchange::IComposition::ScreenResolution_1080p60Hz, _TXT("1080p60Hz") },

ENUM_CONVERSION_END(Exchange::IComposition::ScreenResolution)

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

        if (config.OutOfProcess.Value() == true) {
            _composition = service->Instantiate<Exchange::IComposition>(
                2000, _T("CompositorImplementation"), static_cast<uint32_t>(~0), _pid, config.Locator.Value());
            TRACE(Trace::Information,
                (_T("Compositor started out of process %s implementation"), config.Locator.Value().c_str()));
        }
        else {
            Core::Library resource(config.Locator.Value().c_str());
            if (!resource.IsLoaded())
            {
                string path = service->DataPath() + config.Locator.Value();
                resource = Core::Library(path.c_str());
            }
            if (resource.IsLoaded() == true) {
                _composition = Core::ServiceAdministrator::Instance().Instantiate<Exchange::IComposition>(
                    resource, _T("CompositorImplementation"), static_cast<uint32_t>(~0));
                TRACE(Trace::Information,
                    (_T("Compositor started in process %s implementation"), config.Locator.Value().c_str()));
            }
            else {
                message = _T("Could not load the PlatformPlugin library [") + config.Locator.Value() + _T("]");
            }
        }

        if (_composition == nullptr) {
            message = "Instantiating the compositor failed. Could not load: " + config.Locator.Value();
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
                Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
                Clients(response->Clients);
                result->Body(Core::proxy_cast<Web::IBody>(response));
            }

            // http://<ip>/Service/Compositor/Resolution
            else if (index.Current() == "Resolution") {
                Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
                response->Resolution = GetResolution();
                result->Body(Core::proxy_cast<Web::IBody>(response));
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
                            SetResolution(format);
                        }
                        else {
                            result->ErrorCode = Web::STATUS_BAD_REQUEST;
                            result->Message = _T("invalid parameter for resolution: ") + index.Current().Text();
                        }
                    }
                }
                else {
                    string clientName(index.Current().Text());

                    if (index.Next() == true) {
                        if (index.Current() == _T("Kill")) { /* http://<ip>/Service/Compositor/Netflix/Kill */
                            Kill(clientName);
                        }
                        if (index.Current() == _T("Opacity") &&
                            index.Next() == true) { /* http://<ip>/Service/Compositor/Netflix/Opacity/128 */
                            const uint32_t opacity(std::stoi(index.Current().Text()));
                            Opacity(clientName, opacity);
                        }
                        if (index.Current() == _T("Visible") &&
                            index.Next() == true) { /* http://<ip>/Service/Compositor/Netflix/Visible/Hide */
                            if (index.Current() == _T("Hide")) {
                                Visible(clientName, false);
                            } else if (index.Current() == _T("Show")) {
                                Visible(clientName, true);
                            }
                        }
                        if (index.Current() == _T("Geometry")) { /* http://<ip>/Service/Compositor/Netflix/Geometry/0/0/1280/720 */
                            uint32_t x(0), y(0), width(0), height(0);

                            if (index.Next() == true) {
                                x = Core::NumberType<uint32_t>(index.Current()).Value();
                            }
                            if (index.Next() == true) {
                                y = Core::NumberType<uint32_t>(index.Current()).Value();
                            }
                            if (index.Next() == true) {
                                width  = Core::NumberType<uint32_t>(index.Current()).Value();
                            }
                            if (index.Next() == true) {
                                height = Core::NumberType<uint32_t>(index.Current()).Value();
                            }

                            Geometry(clientName, x, y, width, height);
                        }
                        if (index.Current() == _T("Top")) { /* http://<ip>/Service/Compositor/Netflix/Top */
                            Top(clientName);
                        }
                        if (index.Current() == _T("Input")) { /* http://<ip>/Service/Compositor/Netflix/Input */
                            Input(clientName);
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

    void Compositor::Attached(Exchange::IComposition::IClient* client)
    {
        ASSERT(client != nullptr);

        string name(client->Name());

        _adminLock.Lock();
        std::map<string, Exchange::IComposition::IClient*>::iterator it = _clients.find(name);

        if (it != _clients.end()) {
            it->second->Release();
            _clients.erase(it);
        }

        _clients[name] = client;
        client->AddRef();

        _adminLock.Unlock();

        TRACE(Trace::Information, (_T("Client %s attached"), name.c_str()));
    }
    void Compositor::Detached(Exchange::IComposition::IClient* client)
    {
        ASSERT(client != nullptr);

        string name(client->Name());

        _adminLock.Lock();
        std::map<string, Exchange::IComposition::IClient*>::iterator it = _clients.find(name);

        if (it != _clients.end()) {
            it->second->Release();
            _clients.erase(it);
        }
        _adminLock.Unlock();

        TRACE(Trace::Information, (_T("Client %s detached"), name.c_str()));
    }
    void Compositor::Clients(Core::JSON::ArrayType<Core::JSON::String>& clients) const
    {
        _adminLock.Lock();
        for (auto const& client : _clients) {
            TRACE(Trace::Information, (_T("Client %s added to the JSON array"), client.first.c_str()));
            Core::JSON::String& element(clients.Add());
            element = client.first;
        }
        _adminLock.Unlock();
    }
    void Compositor::Kill(const string& client) const
    {
        ASSERT(_composition != nullptr);

        if (_composition != nullptr) {
            Exchange::IComposition::IClient* composition = _composition->Client(client);
            if (composition != nullptr) {
                composition->Kill();
            }
            else {
                TRACE(Trace::Information, (_T("Client %s not found."), client.c_str()));
            }
        }
    }
    void Compositor::Opacity(const string& client, const uint32_t value) const
    {
        ASSERT(_composition != nullptr);

        if (_composition != nullptr) {
            Exchange::IComposition::IClient* composition = _composition->Client(client);
            if (composition != nullptr) {
                composition->Opacity(value);
            }
            else {
                TRACE(Trace::Information, (_T("Client %s not found."), client.c_str()));
            }
        }
    }

    void Compositor::Geometry(const string& client, const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height) const
    {
        ASSERT(_composition != nullptr);

        if (_composition != nullptr) {
            Exchange::IComposition::IClient* composition = _composition->Client(client);
            if (composition != nullptr) {
                composition->Geometry(x, y, width, height);
            }
            else {
                TRACE(Trace::Information, (_T("Client %s not found."), client.c_str()));
            }
        }
    }

    void Compositor::SetResolution(const Exchange::IComposition::ScreenResolution format) const
    {
        ASSERT(_composition != nullptr);

        if (_composition != nullptr) {
            _composition->SetResolution(format);
        }
    }

    const Exchange::IComposition::ScreenResolution  Compositor::GetResolution() const
    {
        ASSERT(_composition != nullptr);

        if (_composition != nullptr) {
            return (_composition->GetResolution());
        }
        return Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown;
    }

    void Compositor::Visible(const string& client, const bool visible) const
    {
        ASSERT(_composition != nullptr);

        if (_composition != nullptr) {
            Exchange::IComposition::IClient* composition = _composition->Client(client);
            if (composition != nullptr) {
                composition->Visible(visible);
            }
            else {
                TRACE(Trace::Information, (_T("Client %s not found."), client.c_str()));
            }
        }
    }
    void Compositor::Top(const string& client) const
    {
        ASSERT(_composition != nullptr);

        if (_composition != nullptr) {
            Exchange::IComposition::IClient* composition = _composition->Client(client);
            if (composition != nullptr) {
                composition->SetTop();
            }
            else {
                TRACE(Trace::Information, (_T("Client %s not found."), client.c_str()));
            }
        }
    }
    void Compositor::Input(const string& client) const
    {
        ASSERT(_composition != nullptr);

        if (_composition != nullptr) {
            Exchange::IComposition::IClient* composition = _composition->Client(client);
            if (composition != nullptr) {
                composition->SetInput();
            }
            else {
                TRACE(Trace::Information, (_T("Client %s not found."), client.c_str()));
            }
        }
    }

} // namespace Plugin
} // namespace WPEFramework
