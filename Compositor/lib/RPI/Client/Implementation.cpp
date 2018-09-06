#include "Implementation.h"
#include <virtualinput/virtualinput.h>

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
                // Time to load the ProxyStubs, that are used for
                // InterProcess communication
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
        INTERFACE* Create(const string& objectName,
                const uint32_t version = static_cast<uint32_t>(~0)) {
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
    , _client(Core::NodeId(domainName), _T("/usr/lib/wpeframework/proxystubs"))
    , _remote(nullptr) {

        if (_client.IsOperational() == true) { 
            _remote = _client.Create<Exchange::IComposition
                    ::INotification>(_T("CompositorImplementation"));
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
        if ((Core::SystemInfo::GetEnvironment(_T("RPI_COMPOSITOR"),
                connector) == false) || (connector.empty() == true)) {
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

int32_t Display::SurfaceImplementation::RaspberryPiClient::_glayerNum = 0;
Display::SurfaceImplementation::RaspberryPiClient::RaspberryPiClient(
        const std::string& name, const uint32_t x, const uint32_t y,
        const uint32_t width, const uint32_t height)
: Exchange::IComposition::IClient()
, _name(name)
, _x(x)
, _y(y)
, _width(width)
, _height(height) {

    TRACE_L1("Created client named: %s", _name.c_str());

    VC_DISPMANX_ALPHA_T alpha = {
            DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS,
            255,
            0
    };

    _layerNum = getLayerNum();
    vc_dispmanx_rect_set(&_dstRect, 0, 0, _width, _height);
    vc_dispmanx_rect_set(&_srcRect, 0, 0, (_width << 16), (_height << 16));

    _dispmanDisplay = vc_dispmanx_display_open(0);
    _dispmanUpdate = vc_dispmanx_update_start(0);
    _dispmanElement = vc_dispmanx_element_add(
            _dispmanUpdate,
            _dispmanDisplay,
            _layerNum /*layer*/,
            &_dstRect,
            0 /*src*/,
            &_srcRect,
            DISPMANX_PROTECTION_NONE,
            &alpha /*alpha*/,
            0 /*clamp*/,
            DISPMANX_NO_ROTATE);
    vc_dispmanx_update_submit_sync(_dispmanUpdate);

    _nativeWindow.element = _dispmanElement;
    _nativeWindow.width = _width;
    _nativeWindow.height = _height;
    _nativeSurface = static_cast<EGLSurface>(&_nativeWindow);
}

Display::SurfaceImplementation::RaspberryPiClient::~RaspberryPiClient() {

    TRACE_L1("Destructing client named: %s", _name.c_str());

    _dispmanUpdate = vc_dispmanx_update_start(0);
    vc_dispmanx_element_remove(_dispmanUpdate, _dispmanElement);
    vc_dispmanx_update_submit_sync(_dispmanUpdate);
    vc_dispmanx_display_close(_dispmanDisplay);
}

void Display::SurfaceImplementation::RaspberryPiClient::Opacity(
        const uint32_t value) {

    _opacity = (value > 255) ? 255 : value;
    _dispmanUpdate = vc_dispmanx_update_start(0);
    vc_dispmanx_element_change_attributes(_dispmanUpdate,
            _dispmanElement,
            ELEMENT_CHANGE_OPACITY,
            _layerNum,
            _opacity,
            &_dstRect,
            &_srcRect,
            0,
            DISPMANX_NO_ROTATE);
    vc_dispmanx_update_submit_sync(_dispmanUpdate);
}

void Display::SurfaceImplementation::RaspberryPiClient::Geometry(
        const uint32_t X, const uint32_t Y,
        const uint32_t width, const uint32_t height) {
    _x = X;
    _y = Y;
    _width = width;
    _height = height;
    vc_dispmanx_rect_set(&_dstRect, _x, _y, _width, _height);
    vc_dispmanx_rect_set(&_srcRect, 0, 0, (_width << 16), (_height << 16));

    _dispmanUpdate = vc_dispmanx_update_start(0);
    vc_dispmanx_element_change_attributes(_dispmanUpdate,
            _dispmanElement,
            ELEMENT_CHANGE_DEST_RECT,
            _layerNum,
            _opacity,
            &_dstRect,
            &_srcRect,
            0,
            DISPMANX_NO_ROTATE);
    vc_dispmanx_update_submit_sync(_dispmanUpdate);
}

void Display::SurfaceImplementation::RaspberryPiClient::SetTop()
{
    _layerNum = getLayerNum();
    _dispmanUpdate = vc_dispmanx_update_start(0);
    vc_dispmanx_element_change_layer(_dispmanUpdate, _dispmanElement, _layerNum);
    vc_dispmanx_update_submit_sync(_dispmanUpdate);
}

void Display::SurfaceImplementation::RaspberryPiClient::Visible(
        const bool visible) {
}

Display::SurfaceImplementation::SurfaceImplementation(
        Display& display, const std::string& name,
        const uint32_t width, const uint32_t height)
: _parent(display)
, _refcount(1)
, _keyboard(nullptr)
, _client(nullptr) {

    _client = Display::SurfaceImplementation::RaspberryPiClient::Create(
            name, 0, 0, width, height); //todo: where to get x and y?
    _parent.Register(this);
}

Display::SurfaceImplementation::~SurfaceImplementation() {

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
    if (_accessorCompositor != nullptr) {
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
