#include <interfaces/IComposition.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace Plugin {

class CompositorImplementation :
        public Exchange::IComposition,
        public Exchange::IComposition::INotification {
private:
    CompositorImplementation(const CompositorImplementation&) = delete;
    CompositorImplementation& operator=(const CompositorImplementation&) = delete;

    class ExternalAccess : public RPC::Communicator {
    private:
        ExternalAccess() = delete;
        ExternalAccess(const ExternalAccess &) = delete;
        ExternalAccess & operator=(const ExternalAccess &) = delete;

    public:
        ExternalAccess(
                const Core::NodeId & source,
                IComposition::INotification* parentInterface,
                const string& proxyStub)
        : RPC::Communicator(source, Core::ProxyType< RPC::InvokeServerType<8, 1> >::Create(), proxyStub)
        , _parentInterface (parentInterface) {
        }

        ~ExternalAccess() {
            Close(Core::infinite);
        }

    private:
        void* Instance(const string& className, const uint32_t interfaceId, const uint32_t versionId) override {
            void* result = nullptr;
            // Currently we only support version 1 of the IRPCLink :-)
            if (((versionId == 1) || (versionId == static_cast<uint32_t>(~0))) &&
                 (interfaceId == IComposition::INotification::ID)) {
                // Reference count our parent
                _parentInterface->AddRef();
                result = _parentInterface;
            }
            return (result);
        }
        
    private:
        IComposition::INotification* _parentInterface;
    };

public:
    CompositorImplementation()
    : _adminLock()
    , _service(nullptr)
    , _externalAccess()
    , _observers()
    , _clients() {
    }

    ~CompositorImplementation() {
        if (_service != nullptr) {
            _service->Release();
        }
    }

    BEGIN_INTERFACE_MAP(CompositorImplementation)
    INTERFACE_ENTRY(Exchange::IComposition)
    INTERFACE_ENTRY(Exchange::IComposition::INotification)
    END_INTERFACE_MAP

private:
    class Config : public Core::JSON::Container {
    private:
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

    public:
        Config()
        : Core::JSON::Container()
        , Connector(_T("/tmp/compositor")) {
            Add(_T("connector"), &Connector);
        }

        ~Config() {
        }

    public:
        Core::JSON::String Connector;
    };

    struct ClientData
    {
        ClientData(Exchange::IComposition::IClient* client, Exchange::IComposition::ScreenResolution resolution)
        : currentRectangle()
        , clientInterface(client) {
            currentRectangle.x = 0;
            currentRectangle.y = 0;
            currentRectangle.width = Exchange::IComposition::WidthFromResolution(resolution);
            currentRectangle.height = Exchange::IComposition::HeightFromResolution(resolution);
        }

        Exchange::IComposition::Rectangle currentRectangle = Exchange::IComposition::Rectangle();
        Exchange::IComposition::IClient* clientInterface = nullptr;
    };

public:
    uint32_t Configure(PluginHost::IShell* service) override {
        _service = service;
        _service->AddRef();

        string configuration(service->ConfigLine());
        Config config;
        config.FromString(service->ConfigLine());

        _externalAccess.reset(
                new ExternalAccess(
                        Core::NodeId(config.Connector.Value().c_str()), this, service->ProxyStubPath()));
        uint32_t result = _externalAccess->Open(Core::infinite);
        if (result == Core::ERROR_NONE) {
            // Announce the port on which we are listening
            Core::SystemInfo::SetEnvironment(
                    _T("COMPOSITOR"), config.Connector.Value(), true);
            PlatformReady();
        }
        else {
            _externalAccess.reset();
            TRACE(Trace::Error,
                    (_T("Could not open RPI Compositor RPCLink server. Error: %s"),
                            Core::NumberType<uint32_t>(result).Text()));
        }
        return  result;
    }

    void Register(Exchange::IComposition::INotification* notification) override {
        _adminLock.Lock();
        ASSERT(std::find(_observers.begin(),
                _observers.end(), notification) == _observers.end());
        notification->AddRef();
        _observers.push_back(notification);
        auto index(_clients.begin());
        while (index != _clients.end()) {
            notification->Attached((*index).clientInterface);
            index++;
        }
        _adminLock.Unlock();
    }

    void Unregister(Exchange::IComposition::INotification* notification) override {
        _adminLock.Lock();
        std::list<Exchange::IComposition::INotification*>::iterator index(
                std::find(_observers.begin(), _observers.end(), notification));
        ASSERT(index != _observers.end());
        if (index != _observers.end()) {
            _observers.erase(index);
            notification->Release();
        }
        _adminLock.Unlock();
    }

    Exchange::IComposition::IClient* Client(const uint8_t id) override {
        Exchange::IComposition::IClient* result = nullptr;
        _adminLock.Lock();
        auto index(_clients.begin());
        std::advance(index, id);
        if (index != _clients.end()) {
            result = ((*index).clientInterface);
            ASSERT(result != nullptr);
            result->AddRef();
        }
        _adminLock.Unlock();
        return (result);
    }

    Exchange::IComposition::IClient* Client(const string& name) override {
        return FindClient(name);
    }

    virtual void Geometry(const string& callsign, const Exchange::IComposition::Rectangle& rectangle) override {
        Exchange::IComposition::IClient* client = FindClient(callsign);
        if( client != nullptr ) {
            client->ChangedGeometry(rectangle);
            client->Release();
         }   
    }

    virtual Exchange::IComposition::Rectangle Geometry(const string& callsign) const override {
        return FindClientRectangle(callsign);
    }

    void Resolution(const Exchange::IComposition::ScreenResolution format) override {
        fprintf(stderr, "CompositorImplementation::Resolution %d\n", format);
    }

    Exchange::IComposition::ScreenResolution Resolution() const override {
        return Exchange::IComposition::ScreenResolution::ScreenResolution_720p;
    }

private:
    using ConstClientDataIterator = std::list<ClientData>::const_iterator;

    ConstClientDataIterator GetClientIterator(const string& name) const {
        auto iterator =  std::find_if(_clients.begin(), _clients.end(), [&](const ClientData& value)
            {
                ASSERT(value.clientInterface != nullptr);
                return (value.clientInterface->Name() == name);
            });
        return iterator;
    }    

    void Add(Exchange::IComposition::IClient* client) {

        ASSERT (client != nullptr);
        if (client != nullptr) {

            const string name(client->Name());
            if (name.empty() == true) {
                ASSERT(false);
                TRACE(Trace::Information,
                        (_T("Registration of a nameless client.")));
            }
            else {
                _adminLock.Lock();

                const ClientData* clientdata = FindClientData(name); 
                if (clientdata != nullptr) {
                    ASSERT (false);
                    TRACE(Trace::Information,
                            (_T("Client already registered %s."), name.c_str()));
                }
                else {
                    client->AddRef();
                    
                    _clients.push_back(ClientData(client, Resolution()));
                    TRACE(Trace::Information, (_T("Added client %s."), name.c_str()));

                    if (_observers.size() > 0) {
                        std::list<Exchange::IComposition
                        ::INotification*>::iterator index(_observers.begin());
                        while (index != _observers.end()) {
                            (*index)->Attached(client);
                            index++;
                        }
                    }
                }

                _adminLock.Unlock();

            }
        }
    }

    void Remove(const string& clientName) {
        _adminLock.Lock();

        auto iterator =  GetClientIterator(clientName);
        if(iterator != _clients.end()) {
            Exchange::IComposition::IClient* client = (*iterator).clientInterface;
            _clients.erase(iterator);
            TRACE(Trace::Information, (_T("Removed client %s."), clientName.c_str()));

            ASSERT(client != nullptr);

            if (_observers.size() > 0) {
                std::list<Exchange::IComposition
                ::INotification*>::iterator index(_observers.begin());
                while (index != _observers.end()) {
                    (*index)->Detached(client);
                    index++;
                }
            }

            client->Release();        
        }
        else {
            ASSERT (false);
            TRACE(Trace::Information, (_T("Client %s could not be removed, not found."), clientName.c_str()));
        }

        _adminLock.Unlock();
   }

    void PlatformReady() {
        PluginHost::ISubSystem* subSystems(_service->SubSystems());
        ASSERT(subSystems != nullptr);
        if (subSystems != nullptr) {
            subSystems->Set(PluginHost::ISubSystem::PLATFORM, nullptr);
            subSystems->Set(PluginHost::ISubSystem::GRAPHICS, nullptr);
            subSystems->Release();
        }
    }

    void Attached(IClient* client) override {
        Add(client);
    }

    void Detached(IClient* client) override {
        ASSERT(client != nullptr);
        Remove(client->Name());
    }

    Exchange::IComposition::Rectangle FindClientRectangle(const string& name) const {
        Exchange::IComposition::Rectangle rectangle = Exchange::IComposition::Rectangle();

        _adminLock.Lock();

        const ClientData* clientdata = FindClientData(name);

        if(clientdata != nullptr) {
            rectangle = clientdata->currentRectangle;
        }

        _adminLock.Unlock();

        return rectangle;
    }

    IClient* FindClient(const string& name) const {
        IClient* client = nullptr;

        _adminLock.Lock();

        const ClientData* clientdata = FindClientData(name);

        if(clientdata != nullptr) {
            client = clientdata->clientInterface;
            ASSERT(client != nullptr);
            client->AddRef(); 
        }

        _adminLock.Unlock();

        return client;
    }

    const ClientData* FindClientData(const string& name) const {
        const ClientData* clientdata = nullptr;
        auto iterator =  GetClientIterator(name);
        if(iterator != _clients.end())
        {
            clientdata = &(*iterator);
        }
        return clientdata;
    }

    mutable Core::CriticalSection _adminLock;
    PluginHost::IShell* _service;
    std::unique_ptr<ExternalAccess> _externalAccess;
    std::list<Exchange::IComposition::INotification*> _observers;
    std::list<ClientData> _clients;
};

SERVICE_REGISTRATION(CompositorImplementation, 1, 0);

} // namespace Plugin
} // namespace WPEFramework
