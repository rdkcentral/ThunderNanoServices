/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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
 
#ifndef PROJECT_WAYLAND_H
#define PROJECT_WAYLAND_H

#include <wayland-egl.h>
// This define is used in mesa GL implementation to exlcude X11 headers
#define MESA_EGL_NO_X11_HEADERS 1

#include "Module.h"
#include <compositor/Client.h>
#include <compositorclient/Implementation.h>
#include <interfaces/IComposition.h>
#include <interfaces/IInputSwitch.h>

#include <virtualinput/virtualinput.h>

#ifdef ENABLE_NXSERVER
#include "NexusServer/NexusServer.h"
#endif

extern "C" {
namespace WPEFramework {
    namespace Implementation {

        uint32_t SetResolution(Exchange::IComposition::ScreenResolution);
        Exchange::IComposition::ScreenResolution GetResolution();
        
        struct IServer {
            virtual void SetInput(const string&) = 0;
            virtual void Get(const uint32_t, Wayland::Display::Surface&) = 0;
            virtual void Process(Wayland::Display::IProcess*) = 0;
            virtual bool CreateController(const string&, Wayland::Display::ICallback*)  = 0;
            virtual ~IServer(){};
        };

        IServer* Create(PluginHost::IShell* service);
    }
}
}
#endif //PROJECT_WAYLAND_H
