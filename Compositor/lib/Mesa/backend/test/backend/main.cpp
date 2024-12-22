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

#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <IOutput.h>

using namespace Thunder;

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

int main(int argc, const char* argv[])
{

    std::string ConnectorId;

    if (argc == 1) {
        ConnectorId = "card1-HDMI-A-1";
    } else {
        ConnectorId = argv[1];
    }

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

        Core::ProxyType<Exchange::ICompositionBuffer> framebuffer;

        const Exchange::IComposition::Rectangle rectangle = { 0, 0, 1080, 1920 };

        char keyPress;

        do {
            keyPress = toupper(getchar());

            switch (keyPress) {
            case 'Q': {
                break;
            }

            case 'S': {
                if (framebuffer.IsValid() == true) {
                    // TODO: Discuss with Bram what the intention here is??
                    // framebuffer->Render();
                    TRACE_GLOBAL(Trace::Information, ("Back buffer swapped of framebuffer!"));
                }
                break;
            }

            case 'A': {
                if (framebuffer.IsValid() == false) {
                    framebuffer = Compositor::CreateBuffer(ConnectorId, rectangle, format, nullptr);
                    TRACE_GLOBAL(Trace::Information, ("Allocated framebuffer %ux%u", framebuffer->Height(), framebuffer->Width()));
                }
                break;
            }

            case 'R': {
                if (framebuffer.IsValid() == true) {
                    uint32_t res = framebuffer.Release();
                    TRACE_GLOBAL(Trace::Information, ("Released framebuffer (%d)", res));
                }
                break;
            }

            case '\n': {
                break;
            }

            case '?': {
                printf("%s <card1-HDMI-A-2>\n\n", argv[0]);
                printf("[S]wap buffers\n");
                printf("[A]llocate a frame buffer\n");
                printf("[R]elease a frame buffer\n");
                printf("[Q]uit application \n");
                break;
            }

            default: {
                printf("Not known. Press '?' for help.\n");
                break;
            }
            }
        } while (keyPress != 'Q');
    }

    TRACE_GLOBAL(Trace::Information, ("Testing Done..."));
    tracer.Close();
    Core::Singleton::Dispose();

    return 0;
}
