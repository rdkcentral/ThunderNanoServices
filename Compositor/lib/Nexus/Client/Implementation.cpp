#include "Implementation.h"

#include <EGL/eglext.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string.h>

#define BACKEND_BCM_NEXUS_NXCLIENT 1

#ifdef BACKEND_BCM_NEXUS_NXCLIENT
#include <refsw/nxclient.h>
#endif

#include <virtualinput/virtualinput.h>

// pipe to relay the keys to the display...
int g_pipefd[2];

struct Message {
    uint32_t code;
    actiontype type;
};

static const char * connectorName = "/tmp/keyhandler";

    static void VirtualKeyboardCallback(actiontype type , unsigned int code)
    {
        if (type != COMPLETED)
        {
            Message message;
            message.code = code;
            message.type = type;
            write(g_pipefd[1], &message, sizeof(message));
        }
    }


namespace WPEFramework {

namespace Nexus {

    Display::SurfaceImplementation::SurfaceImplementation(Display& display, const std::string& name, const uint32_t width, const uint32_t height)
        : _parent(display)
        , _refcount(1)
        , _name(name)
        , _x(0)
        , _y(0)
        , _width(width)
        , _height(height)
        , _nativeWindow(nullptr)
        , _keyboard(nullptr) {

        uint32_t nexusClientId(0); // For now we only accept 0. See Mail David Montgomery
        const char* tmp (getenv("CLIENT_IDENTIFIER"));

        if ( (tmp != nullptr) && ((tmp = strchr(tmp, ',')) != nullptr) ) {
            nexusClientId = atoi(&(tmp[1]));
        }

        //NXPL_NativeWindowInfoEXT windowInfo;
        NXPL_NativeWindowInfo windowInfo;
        windowInfo.x = 0;
        windowInfo.y = 0;
        windowInfo.width = _width;
        windowInfo.height = _height;
        windowInfo.stretch = false;
#ifdef BACKEND_BCM_NEXUS_NXCLIENT
        windowInfo.zOrder = 0;
#endif
        windowInfo.clientID = nexusClientId;
        // _nativeWindow = NXPL_CreateNativeWindowEXT(&windowInfo);
        _nativeWindow = NXPL_CreateNativeWindow(&windowInfo);

        _parent.Register(this);
    }

    /* virtual */ Display::SurfaceImplementation::~SurfaceImplementation() {
        NEXUS_SurfaceClient_Release(reinterpret_cast<NEXUS_SurfaceClient*>(_nativeWindow));

       _parent.Unregister(this);
    }

    Display::Display(const std::string& name) 
        : _displayName(name)
        , _nxplHandle(nullptr)
        , _virtualkeyboard(nullptr)  {

        NEXUS_DisplayHandle displayHandle(nullptr);

#ifdef BACKEND_BCM_NEXUS_NXCLIENT
        NxClient_JoinSettings joinSettings;
        NxClient_GetDefaultJoinSettings(&joinSettings);

        strcpy(joinSettings.name, name.c_str());

        NEXUS_Error rc = NxClient_Join(&joinSettings);
        BDBG_ASSERT(!rc);
#else
        NEXUS_Error rc = NEXUS_Platform_Join();
        BDBG_ASSERT(!rc);
#endif

        NXPL_RegisterNexusDisplayPlatform(&_nxplHandle, displayHandle);

        if (pipe(g_pipefd) == -1) {
            g_pipefd[0] = -1;
            g_pipefd[1] = -1;
        }

        _virtualkeyboard = Construct(name.c_str(), connectorName, VirtualKeyboardCallback);
        if (_virtualkeyboard == nullptr) {
            fprintf(stderr, "[LibinputServer] Initialization of virtual keyboard failed!!!\n");
        }

        printf("Constructed the Display: %d - %s",this, _displayName.c_str());
    }

    Display::~Display()
    {
        NXPL_UnregisterNexusDisplayPlatform(_nxplHandle);
#ifdef BACKEND_BCM_NEXUS_NXCLIENT
        NxClient_Uninit();
#endif
        if (_virtualkeyboard != nullptr) {
           Destruct(_virtualkeyboard);
        }

        printf("Destructed the Display: %d - %s", this, _displayName.c_str());
    }

    /* virtual */ Compositor::IDisplay::ISurface* Display::Create(const std::string& name, const uint32_t width, const uint32_t height)
    {
        return (new SurfaceImplementation(*this, name, width, height));
    }

    /* static */ Compositor::IDisplay* Display::Instance(const std::string& displayName)
    {
        static Display myDisplay(displayName);

        return (&myDisplay);
    }

    /* virtual */ int Display::Process (const uint32_t data) {
        Message message;
        if ((data != 0) && (g_pipefd[0] != -1) && (read(g_pipefd[0], &message, sizeof(message)) > 0)) {

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

    /* virtual */ int Display::FileDescriptor() const
    {
        return (g_pipefd[0]);
    }
}

    /* static */ Compositor::IDisplay* Compositor::IDisplay::Instance(const std::string& displayName) {
        return (Nexus::Display::Instance(displayName));
    }
}
