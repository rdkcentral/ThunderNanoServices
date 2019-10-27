#ifndef PROJECT_WAYLAND_H
#define PROJECT_WAYLAND_H

#include <wayland-egl.h>
// This define is used in mesa GL implementation to exlcude X11 headers
#define MESA_EGL_NO_X11_HEADERS 1

#include "Module.h"
#include <compositor/Client.h>
#include <compositorclient/Implementation.h>
#include <interfaces/IComposition.h>

#include <virtualinput/virtualinput.h>

#ifdef ENABLE_NXSERVER
#include "NexusServer/NexusServer.h"
#endif

extern "C" {
namespace WPEFramework {
    namespace Implementation {
        struct IServer {
            virtual void SetInput(const char name[]) = 0;

            virtual ~IServer(){};
        };

        IServer* Create(PluginHost::IShell* service);
    }
}
}
#endif //PROJECT_WAYLAND_H
