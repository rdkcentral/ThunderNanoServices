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
        RPCClient(const Core::NodeId& nodeId)
        : _client(Core::ProxyType<RPC::CommunicatorClient>::Create(nodeId))
        , _service(Core::ProxyType<RPCService>::Create(Core::Thread::DefaultStackSize())) {

            if (_client->Open(RPC::CommunicationTimeOut, _T("CompositorImplementation"), Exchange::IComposition::INotification::ID, ~0) == Core::ERROR_NONE) {
                _client->CreateFactory<RPC::InvokeMessage>(2);
                _client->Register(_service);
            }
            else {
                _client.Release();
            }
        }
        ~RPCClient() {
            if (_client.IsValid() == true) {
                _client->Unregister(_service);
                _client->DestroyFactory<RPC::InvokeMessage>();
                _client->Close(Core::infinite);
                _client.Release();
            }
        }

    public:
        inline bool IsValid() const {
            return (_client.IsValid());
        }
        template <typename INTERFACE>
		INTERFACE* WaitForCompletion(const uint32_t waitTime) {
			return (_client->WaitForCompletion<INTERFACE>(waitTime));
		}

    private:
        Core::ProxyType<RPC::CommunicatorClient> _client;
        Core::ProxyType<RPCService> _service;
    };

private:
    AccessorCompositor (const TCHAR domainName[]) 
    : _refCount(1)
    , _adminLock()
    , _client(Core::NodeId(domainName))
    , _remote(nullptr) {
        if (_client.IsValid() == true) {
           _remote = _client.WaitForCompletion<Exchange::IComposition::INotification>(6000);
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
        if ((Core::SystemInfo::GetEnvironment(_T("COMPOSITOR"), connector) == false) || (connector.empty() == true)) {
            connector = _T("/tmp/compositor");
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

Display::SurfaceImplementation::RaspberryPiClient::RaspberryPiClient(
        SurfaceImplementation& surface,
        const std::string& name, const uint32_t x, const uint32_t y,
        const uint32_t width, const uint32_t height)
: Exchange::IComposition::IClient()
, _parent(surface)
, _refcount(1)
, _name(name)
, _x(x)
, _y(y)
, _width(width)
, _height(height)
, _opacity(255)
, _visible(true) {

    TRACE_L1("Created client named: %s", _name.c_str());

    graphics_get_display_size(0, &_screenWidth, &_screenHeight);
    VC_DISPMANX_ALPHA_T alpha = {
            DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS,
            255,
            0
    };
    _layerNum = _parent.getLayerNum();
    vc_dispmanx_rect_set(&_dstRect, 0, 0, _width, _height);
    vc_dispmanx_rect_set(&_srcRect,
            0, 0, (_screenWidth << 16), (_screenHeight << 16));

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
    _nativeWindow.width = _screenWidth;
    _nativeWindow.height = _screenHeight;
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
    uint32_t opacity = (_visible == true) ? _opacity : 0;

    _dispmanUpdate = vc_dispmanx_update_start(0);
    vc_dispmanx_element_change_attributes(_dispmanUpdate,
            _dispmanElement,
            ELEMENT_CHANGE_OPACITY,
            _layerNum,
            opacity,
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
    vc_dispmanx_rect_set(&_srcRect,
            0, 0, (_screenWidth << 16), (_screenHeight << 16));
    uint32_t opacity = (_visible == true) ? _opacity : 0;

    _dispmanUpdate = vc_dispmanx_update_start(0);
    vc_dispmanx_element_change_attributes(_dispmanUpdate,
            _dispmanElement,
            ELEMENT_CHANGE_DEST_RECT,
            _layerNum,
            opacity,
            &_dstRect,
            &_srcRect,
            0,
            DISPMANX_NO_ROTATE);
    vc_dispmanx_update_submit_sync(_dispmanUpdate);
}

void Display::SurfaceImplementation::RaspberryPiClient::SetTop() {
    _layerNum = _parent.getLayerNum();
    _dispmanUpdate = vc_dispmanx_update_start(0);
    vc_dispmanx_element_change_layer(_dispmanUpdate, _dispmanElement, _layerNum);
    vc_dispmanx_update_submit_sync(_dispmanUpdate);
}

void Display::SurfaceImplementation::RaspberryPiClient::SetInput() {
    _parent.SetInput();
}

void Display::SurfaceImplementation::RaspberryPiClient::Visible(
        const bool visible) {

    _visible = visible;
    uint32_t opacity = (_visible == true) ? _opacity : 0;

    _dispmanUpdate = vc_dispmanx_update_start(0);
    vc_dispmanx_element_change_attributes(_dispmanUpdate,
            _dispmanElement,
            ELEMENT_CHANGE_OPACITY,
            _layerNum,
            opacity,
            &_dstRect,
            &_srcRect,
            0,
            DISPMANX_NO_ROTATE);
    vc_dispmanx_update_submit_sync(_dispmanUpdate);
}

Display::SurfaceImplementation::SurfaceImplementation(
        Display& display, const std::string& name,
        const uint32_t width, const uint32_t height)
: _parent(display)
, _refcount(1)
, _keyboard(nullptr)
, _client(nullptr) {

    _client = new Display::SurfaceImplementation::RaspberryPiClient(
                *this, name, 0, 0, width, height); //todo: where to get x and y?
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
, _glayerNum(0)
, _virtualkeyboard(nullptr)
, _accessorCompositor(AccessorCompositor::Create())  {
 bcm_host_init();
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
                // RELEASED  = 0,
                // PRESSED   = 1,
                // REPEAT    = 2,

                (*index)->SendKey (message.code, (message.type == 0 ? IDisplay::IKeyboard::released : IDisplay::IKeyboard::pressed), time(nullptr));
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
   
    return (Rpi::Display::Instance(displayName));
}
}
