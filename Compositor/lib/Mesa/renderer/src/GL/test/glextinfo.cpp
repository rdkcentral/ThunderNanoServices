/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological B.V.
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

#ifndef MODULE_NAME
#define MODULE_NAME CompositorRenderTest
#endif

#include <core/core.h>
#include <localtracer/localtracer.h>
#include <messaging/messaging.h>

#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "../EGL.h"
#include "../RenderAPI.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace Thunder;

namespace {
const std::vector<std::string> Parse(const std::string& input)
{
    std::istringstream iss(input);
    return std::vector<std::string> { std::istream_iterator<std::string> { iss }, std::istream_iterator<std::string> {} };
}

}

int main(int /*argc*/, const char* argv[])
{
    Messaging::LocalTracer& tracer = Messaging::LocalTracer::Open();

    const char* executableName(Core::FileNameOnly(argv[0]));

    {
        Messaging::ConsolePrinter printer(true);

        tracer.Callback(&printer);

        const std::vector<string> modules = {
            "Error",
            "Information",
            "EGL",
            "GL"
        };

        for (auto module : modules) {
            tracer.EnableMessage("CompositorRenderTest", module, true);
            tracer.EnableMessage("CompositorRenderer", module, true);
        }

        TRACE_GLOBAL(Trace::Information, ("%s - build: %s", executableName, __TIMESTAMP__));

        int drmFd = open("/dev/dri/renderD129", O_RDWR | O_CLOEXEC);

        Compositor::Renderer::EGL egl(drmFd);

        const std::string extensions(eglQueryString(egl.Display(), EGL_EXTENSIONS));

        std::stringstream msg;

        msg << std::endl
            << "Extensions:" << std::endl;

        for (auto& ext : Parse(extensions)) {
            msg << " - " << ext << std::endl;
        }
        TRACE_GLOBAL(Trace::Information, ("%s", msg.str().c_str()));

        const std::string apis(eglQueryString(egl.Display(), EGL_CLIENT_APIS));

        msg.str("");
        msg.clear();

        msg << std::endl
            << "API's:" << std::endl;
        for (auto& s : Parse(apis)) {
            msg << " - " << s << std::endl;
        }

        TRACE_GLOBAL(Trace::Information, ("%s", msg.str().c_str()));

        close(drmFd);

        TRACE_GLOBAL(Trace::Information, ("Exiting %s.... ", executableName));
    }

    tracer.Close();
    Core::Singleton::Dispose();

    return 0;
}
