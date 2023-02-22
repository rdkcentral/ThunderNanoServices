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
#define MODULE_NAME CompositorDrmDevTest
#endif

#include <core/core.h>
#include <localtracer/localtracer.h>
#include <messaging/messaging.h>

#include <IBackend.h>

#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <libudev.h>

using namespace WPEFramework;

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

int main(int /* argc*/, const char* argv[])
{
    Messaging::LocalTracer& tracer = Messaging::LocalTracer::Open();
    Messaging::ConsolePrinter printer(false);

    tracer.Callback(&printer);

    const std::vector<string> modules = {
        "CompositorDrmDevTest",
        "CompositorBackend",
        "CompositorBuffer"
    };

    for (auto module : modules) {
        tracer.EnableMessage(module, "", true);
    }

    {
        TRACE_GLOBAL(Trace::Information, ("Starting %s build %s", argv[0], __TIMESTAMP__));

        uint64_t mods[1] = { DRM_FORMAT_MOD_LINEAR };
        Compositor::PixelFormat format(DRM_FORMAT_ARGB8888, (sizeof(mods) / sizeof(mods[0])), mods);

        WPEFramework::Core::ProxyType<Compositor::Interfaces::IBuffer> framebuffer;

        char keyPress;

        do {
            keyPress = toupper(getchar());

            switch (keyPress) {
            case 'Q': {
                break;
            }

            case 'S': {
                if (framebuffer.IsValid() == true) {
                    framebuffer->Render();
                    TRACE_GLOBAL(Trace::Information, ("Back buffer swapped to id=%u", framebuffer->Identifier()));
                    break;
                }
            }

            case 'A': {
                if (framebuffer.IsValid() == false) {
                    framebuffer = Compositor::Interfaces::IBackend::Connector("card1-HDMI-A-1", Exchange::IComposition::ScreenResolution::ScreenResolution_1080p, format, false);
                    TRACE_GLOBAL(Trace::Information, ("Allocated framebuffer %u %ux%u", framebuffer->Identifier(), framebuffer->Height(), framebuffer->Width()));
                } else {
                    framebuffer->AddRef();
                    TRACE_GLOBAL(Trace::Information, ("Add reffed framebuffer %u", framebuffer->Identifier()));
                }
                break;
            }

            case 'R': {
                if (framebuffer.IsValid() == true) {
                    TRACE_GLOBAL(Trace::Information, ("Releasing framebuffer", framebuffer->Identifier()));
                    framebuffer.Release();
                }
                break;
            }

            case '\n': {
                break;
            }

            default: {
                printf("Not known. Press '?' for help.\n");
                break;
            }
            }
        } while (keyPress != 'Q');

        if (framebuffer.IsValid() == true) {
            framebuffer.Release();
            TRACE_GLOBAL(Trace::Information, ("framebuffer1 released..."));
        }
    }

    TRACE_GLOBAL(Trace::Information, ("Testing Done..."));
    tracer.Close();
    WPEFramework::Core::Singleton::Dispose();

    return 0;
}