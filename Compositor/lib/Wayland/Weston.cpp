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

#include "Wayland.h"

namespace WPEFramework {
namespace Weston {

    class Compositor : public Implementation::IServer {
    private:
        Compositor(const string& renderModule, const string& display, const uint32_t width, const uint32_t height)
        {
            TRACE(Trace::Information, (_T("Starting Compositor renderModule=%s display=%s"), renderModule.c_str(), display.c_str()));

            ASSERT(_instance == nullptr);
            _instance = this;

            string runtimeDir;
            Core::SystemInfo::GetEnvironment(_T("XDG_RUNTIME_DIR"), runtimeDir);

            Core::File path(runtimeDir);

            if (path.IsDirectory() == false) {
                Core::Directory(path.Name().c_str()).CreatePath();
                TRACE(Trace::Information, (_T("Created XDG_RUNTIME_DIR: %s\n"), path.Name().c_str()));
            }

        }

    public:
        Compositor() = delete;
        Compositor(const Compositor&) = delete;
        Compositor& operator=(const Compositor&) = delete;

        static Compositor* Create(const string& renderer, const string& display, uint32_t width, uint32_t height)
        {
            return _instance == nullptr ? new Weston::Compositor(renderer, display, width, height) : _instance;
        }
        static Compositor* Instance() {
            return (_instance);
        }

        virtual ~Compositor()
        {
            TRACE(Trace::Information, (_T("Destructing the compositor")));

            _instance = nullptr;
        }

    public:

        /*virtual*/ void SetInput(const char name[])
        {
        }

    private:
        static void callback(bool ready)
        {
        }

    private:
        static Weston::Compositor* _instance;
    };

    /*static*/ Weston::Compositor* Weston::Compositor::_instance = nullptr;
} // namespace Weston

extern "C" {
namespace Implementation {

    class Config : public Core::JSON::Container {
    private:
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

    public:
        Config()
            : Core::JSON::Container()
            , Renderer(_T("/usr/lib/libwesteros_render_gl.so"))
            , GLName(_T("libwesteros_gl.so.0.0.0"))
            , Display(_T("wayland-0"))
            , Cursor(_T(""))
        {
            Add(_T("renderer"), &Renderer);
            Add(_T("glname"), &GLName);
            Add(_T("display"), &Display);
            Add(_T("cursor"), &Cursor);
            Add(_T("width"), &Width);
            Add(_T("height"), &Height);
        }
        ~Config()
        {
        }

    public:
        Core::JSON::String Renderer;
        Core::JSON::String GLName;
        Core::JSON::String Display;
        Core::JSON::String Cursor;
        Core::JSON::DecUInt32 Width;
        Core::JSON::DecUInt32 Height;
    };
 
    IServer* Create(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        Config config;
        config.FromString(service->ConfigLine());

        Weston::Compositor* instance = Weston::Compositor::Create(config.Renderer.Value(), config.Display.Value(), config.Width.Value(), config.Height.Value());

        return instance;
    }
} // namespace Implementation
}
} // namespace WPEFramework
