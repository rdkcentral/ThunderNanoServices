#include "Compositor.h"

namespace WPEFramework {
namespace Plugin {
    SERVICE_REGISTRATION(Compositor, 1, 0);

    static Core::ProxyPoolType<Web::Response> responseFactory(4);
    static Core::ProxyPoolType<Web::JSONBodyType<Compositor::Data> > jsonResponseFactory(4);

    Compositor::Compositor()
        : _adminLock()
        , _skipURL()
        , _notification(this)
        , _resourceCenter(nullptr)
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
        printf("Compositor::Initialize\n"); fflush(stdout);
        string message(EMPTY_STRING);
        string result;

        ASSERT(service != nullptr);
        ASSERT(_resourceCenter == nullptr);

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
            printf("Compositor start out-of-process\n"); fflush(stdout);
            _resourceCenter = service->Instantiate<Exchange::IResourceCenter>(
                2000, _T("PlatformImplementation"), static_cast<uint32_t>(~0), _pid, config.Locator.Value());
            TRACE(Trace::Information,
                (_T("Compositor started out of process %s implementation"), config.Locator.Value().c_str()));
        }
        else {
            printf("Compositor start in-process\n"); fflush(stdout);
            Core::Library resource(config.Locator.Value().c_str());
            if (!resource.IsLoaded())
            {
                string path = service->DataPath() + config.Locator.Value();
                resource = Core::Library(path.c_str());
            }
            if (resource.IsLoaded() == true) {
                _resourceCenter = Core::ServiceAdministrator::Instance().Instantiate<Exchange::IResourceCenter>(
                    resource, _T("PlatformImplementation"), static_cast<uint32_t>(~0));
                TRACE(Trace::Information,
                    (_T("Compositor started in process %s implementation"), config.Locator.Value().c_str()));
            }
            else {
                message = _T("Could not load the PlatformPlugin library [") + config.Locator.Value() + _T("]");
            }
        }

        if (_resourceCenter == nullptr) {
            message = "Instantiating the compositor failed. Could not load: " + config.Locator.Value();
        }
        else {
            _service = service;

            _composition = _resourceCenter->QueryInterface<Exchange::IComposition>();

            _notification.Initialize(service, _composition);

            _resourceCenter->Configure(_service);

            // We would actually need to handle setting the Graphics event in the PlatformImplementation. For now, we do it here.
            PluginHost::ISubSystem* subSystems = _service->SubSystems();

            ASSERT(subSystems != nullptr);

            if (subSystems != nullptr) {
                // Set Graphics event. We need to set up a handler for this at a later moment
                subSystems->Set(PluginHost::ISubSystem::GRAPHICS, nullptr);
                subSystems->Release();
            }
        }

        // On succes return empty, to indicate there is no error text.
        return message;
    }
    /* virtual */ void Compositor::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service == _service);

        // We would actually need to handle setting the Graphics event in the PlatformImplementation. For now, we do it here.
        PluginHost::ISubSystem* subSystems = _service->SubSystems();

        ASSERT(subSystems != nullptr);

        if (subSystems != nullptr) {
            // Set Graphics event. We need to set up a handler for this at a later moment
            subSystems->Set(PluginHost::ISubSystem::NOT_GRAPHICS, nullptr);
            subSystems->Release();
        }
 
        if (_resourceCenter != nullptr) {
            _resourceCenter->Release();
            _resourceCenter = nullptr;
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
            Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
            // http://<ip>/Service/Compositor/
            if (index.Next() == false) {
                Clients(response->Clients);
            }

            // http://<ip>/Service/Compositor/Clients
            else if (index.Current() == "Clients") {
                Clients(response->Clients);
            }

            result->ContentType = Web::MIMETypes::MIME_JSON;
            result->Body(Core::proxy_cast<Web::IBody>(response));
        }
        else if (request.Verb == Web::Request::HTTP_POST) {
            Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());

            if (index.Next() == true) {
                string clientName(index.Current().Text());

                if (index.Next() == true) {
                    if (index.Current().Text() == "Kill") { /* http://<ip>/Service/Compositor/Netflix/Kill */
                        Kill(clientName);
                    }
                    if (index.Current().Text() == "Opacity" && index.Next() == true) { /* http://<ip>/Service/Compositor/Netflix/Opacity/128 */
                        const uint32_t opacity(std::stoi(index.Current().Text()));
                        Opacity(clientName, opacity);
                    }
                    if (index.Current().Text() == "Visible" && index.Next() == true) { /* http://<ip>/Service/Compositor/Netflix/Visible/Hide */
                        if (index.Current().Text() == _T("Hide")) {
                            Visible(clientName, false);
                        }
                        else if (index.Current().Text() == _T("Show")) {
                            Visible(clientName, true);
                        }
                    }
                    if (index.Current().Text() == "Geometry") { /* http://<ip>/Service/Compositor/Netflix/Geometry/0/0/1280/720 */
                        uint32_t x(0), y(0), width(0), height(0);

                        if (index.Next() == true) {
                            x = std::stoi(index.Current().Text());
                        }
                        if (index.Next() == true) {
                            y = std::stoi(index.Current().Text());
                        }
                        if (index.Next() == true) {
                            width = std::stoi(index.Current().Text());
                        }
                        if (index.Next() == true) {
                            height = std::stoi(index.Current().Text());
                        }

                        Geometry(clientName, x, y, width, height);
                    }
                    if (index.Current().Text() == "Top") { /* http://<ip>/Service/Compositor/Netflix/Top */
                        Top(clientName);
                    }
                    if (index.Current().Text() == "Input") { /* http://<ip>/Service/Compositor/Netflix/Input */
                        Input(clientName);
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
