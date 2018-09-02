#include "Implementation.h"
#include <virtualinput/virtualinput.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>

int g_pipefd[2];
struct Message {
    uint32_t code;
    actiontype type;
};

static const char * connectorName = "/tmp/keyhandler";
static void VirtualKeyboardCallback(actiontype type , unsigned int code) {
    if (type != COMPLETED) {
        Message message;
        message.code = code;
        message.type = type;
        write(g_pipefd[1], &message, sizeof(message));
    }
}

namespace WPEFramework {
namespace Rpi {

class AccessorCompositor : public Exchange::IComposition::INotification {
public:
    AccessorCompositor () = delete;
    AccessorCompositor (const AccessorCompositor&) = delete;
    AccessorCompositor& operator= (const AccessorCompositor&) = delete;

private:
    class RPCClient {
    private:
        RPCClient() = delete;
        RPCClient(const RPCClient&) = delete;
        RPCClient& operator=(const RPCClient&) = delete;

        typedef WPEFramework::RPC::InvokeServerType<4, 1> RPCService;

    public:
        RPCClient(const Core::NodeId& nodeId, const string& proxyStubPath)
            : _proxyStubs() 
            , _client(Core::ProxyType<RPC::CommunicatorClient>::Create(nodeId))
            , _service(Core::ProxyType<RPCService>::Create(Core::Thread::DefaultStackSize())) {

            _client->CreateFactory<RPC::InvokeMessage>(2);
            if (_client->Open(RPC::CommunicationTimeOut) == Core::ERROR_NONE) {

                // Time to load the ProxyStubs, that are used for InterProcess communication
                Core::Directory index(proxyStubPath.c_str(), _T("*.so"));

                while (index.Next() == true) {
                    Core::Library library(index.Current().c_str());

                    if (library.IsLoaded() == true) {
                        _proxyStubs.push_back(library);
                    }             
                } 
 
                SleepMs(100);

                _client->Register(_service);
            }
            else {
                _client.Release();
            }
        }
        ~RPCClient() {
            _proxyStubs.clear();
            if (_client.IsValid() == true) {
                _client->Unregister(_service);
                _client->Close(Core::infinite);
                _client.Release();
            }
        }

    public:
        inline bool IsOperational() const {
            return (_client.IsValid());
        }

        template <typename INTERFACE>
        INTERFACE* Create(const string& objectName, const uint32_t version = static_cast<uint32_t>(~0)) {
            INTERFACE* result = nullptr;

            if (_client.IsValid() == true) {
                // Oke we could open the channel, lets get the interface
                result = _client->Create<INTERFACE>(objectName, version);
            }

            return (result);
        }

    private:
        std::list<Core::Library> _proxyStubs;
        Core::ProxyType<RPC::CommunicatorClient> _client;
        Core::ProxyType<RPCService> _service;
    };

    private:

    AccessorCompositor (const TCHAR domainName[]) 
        : _refCount(1)
        , _adminLock() 
        , _client(Core::NodeId(domainName), _T("/usr/lib/wpeframework/proxystubs")) // todo: how to get the correct path?
        , _remote(nullptr) {

        if (_client.IsOperational() == true) { 

            _remote = _client.Create<Exchange::IComposition::INotification>(_T("CompositorImplementation"));
        }
    }

    public:

    virtual void AddRef() const override {
        Core::InterlockedIncrement(_refCount);
    }

    virtual uint32_t Release() const override {

        _adminLock.Lock();

        if (Core::InterlockedDecrement(_refCount) == 0) {
            delete this;
            return (Core::ERROR_DESTRUCTION_SUCCEEDED);
        }

        _adminLock.Unlock();

        return (Core::ERROR_NONE);
    }

    void Attached(Exchange::IComposition::IClient* client) override {
        if( _remote != nullptr ) {
            _remote->Attached(client);
        }
    }

    void Detached(Exchange::IComposition::IClient* client) override {
        if( _remote != nullptr ) {
            _remote->Detached(client);
        }
    }

public:
    static AccessorCompositor* Create () {

			string connector;
			if ((Core::SystemInfo::GetEnvironment(_T("RPI_COMPOSITOR"), connector) == false) || (connector.empty() == true)) {
				connector = _T("/tmp/rpicompositor");
			}

            AccessorCompositor* result = new AccessorCompositor (connector.c_str());

            if (result->_remote == nullptr) {
                TRACE_L1("Failed to creater AccessorCompositor");
                delete result;
            }
            else {
                TRACE_L1("Created the AccessorCompositor succesfully");
            }

        return (result);
    }

    ~AccessorCompositor() {
        if (_remote != nullptr) {
            _remote->Release();
        }
        TRACE_L1("Destructed the AccessorCompositor");
    }

    BEGIN_INTERFACE_MAP(AccessorCompositor)
        INTERFACE_ENTRY(Exchange::IComposition::INotification)
    END_INTERFACE_MAP

private:
    mutable uint32_t _refCount;
    mutable Core::CriticalSection _adminLock;
    RPCClient _client;
    Exchange::IComposition::INotification* _remote;
};

Display::SurfaceImplementation::SurfaceImplementation(
        Display& display, const std::string& name,
        const uint32_t width, const uint32_t height)
: _parent(display)
, _refcount(1)
, _nativeWindow(nullptr)
, _keyboard(nullptr)
, _client(nullptr) 
{

    _client = Display::SurfaceImplementation::RaspberryPiClient::Create(name, 0, 0, width, height); //todo: where to get x and y?

    _parent.Register(this);
}

Display::SurfaceImplementation::~SurfaceImplementation() {

    dispman_update = vc_dispmanx_update_start(0);
    vc_dispmanx_element_remove(dispman_update, dispman_element);
    vc_dispmanx_update_submit_sync(dispman_update);
    vc_dispmanx_display_close(dispman_display);

    _parent.Unregister(this);

    if( _client != nullptr ) { 
        _client->Release();
        _client = nullptr;
    }
}

Display::Display(const std::string& name)
: _displayName(name)
, _virtualkeyboard(nullptr)
, _accessorCompositor(AccessorCompositor::Create())  {

    if (pipe(g_pipefd) == -1) {
        g_pipefd[0] = -1;
        g_pipefd[1] = -1;
    }

    _virtualkeyboard = Construct(
            name.c_str(), connectorName, VirtualKeyboardCallback);
    if (_virtualkeyboard == nullptr) {
        fprintf(stderr, "Initialization of virtual keyboard failed!!!\n");
    }
}

Display::~Display() {

    if (_virtualkeyboard != nullptr) {
        Destruct(_virtualkeyboard);
    }

    if ( _accessorCompositor != nullptr ) {
        _accessorCompositor->Release();
    }
}

int Display::Process(const uint32_t data) {
    Message message;
    if ((data != 0) && (g_pipefd[0] != -1) &&
            (read(g_pipefd[0], &message, sizeof(message)) > 0)) {

        std::list<SurfaceImplementation*>::iterator index(_surfaces.begin());
        while (index != _surfaces.end()) {
            (*index)->SendKey(
                    message.code, (message.type == 0 ?
                            IDisplay::IKeyboard::released :
                            IDisplay::IKeyboard::pressed), time(nullptr));
            index++;
        }
    }
    return (0);
}

int Display::FileDescriptor() const {
    return (g_pipefd[0]);
}

Compositor::IDisplay::ISurface* Display::Create(
        const std::string& name, const uint32_t width, const uint32_t height) {
    return (new SurfaceImplementation(*this, name, width, height));
}

Compositor::IDisplay* Display::Instance(const std::string& displayName) {
    static Display myDisplay(displayName);
    return (&myDisplay);
}


inline void Display::Register(Display::SurfaceImplementation* surface) {
    std::list<SurfaceImplementation*>::iterator index(
            std::find(_surfaces.begin(), _surfaces.end(), surface));
    if (index == _surfaces.end()) {
        _surfaces.push_back(surface);
        if ( _accessorCompositor != nullptr && surface->Client() != nullptr) {
        _accessorCompositor->Attached(surface->Client());
        }
    }
}
inline void Display::Unregister(Display::SurfaceImplementation* surface) {
    std::list<SurfaceImplementation*>::iterator index(
            std::find(_surfaces.begin(), _surfaces.end(), surface));
    if (index != _surfaces.end()) {
        if ( _accessorCompositor != nullptr && surface->Client() != nullptr) {
        _accessorCompositor->Detached(surface->Client());
        }
        _surfaces.erase(index);
    }
}  


}

Compositor::IDisplay* Compositor::IDisplay::Instance(
        const std::string& displayName) {
    bcm_host_init();
    return (Rpi::Display::Instance(displayName));
}

}

