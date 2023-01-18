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

#include <IRenderer.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace WPEFramework;

namespace {
const std::vector<std::string> Parse(const std::string& input)
{
    std::istringstream iss(input);
    return std::vector<std::string> { std::istream_iterator<std::string> { iss }, std::istream_iterator<std::string> {} };
}
}

int main(int /*argc*/, const char* argv[])
{

    Messaging::MessageUnit::flush flushMode;
    flushMode = Messaging::MessageUnit::flush::FLUSH;

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
            tracer.EnableMessage("CompositorRendererEGL", module, true);
            tracer.EnableMessage("CompositorRendererGLES2", module, true);
        }

        TRACE_GLOBAL(Trace::Information, ("%s - build: %s", executableName, __TIMESTAMP__));

        int drmFd = open("/dev/dri/renderD129", O_RDWR | O_CLOEXEC);

        Compositor::Interfaces::IRenderer::Instance(drmFd);

        close(drmFd);

        TRACE_GLOBAL(Trace::Information, ("Exiting %s.... ", executableName));
    }

    tracer.Close();
    Core::Singleton::Dispose();

    return 0;
}
