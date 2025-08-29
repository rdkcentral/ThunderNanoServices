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

#include <condition_variable>
#include <mutex>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <inttypes.h>

#include <IOutput.h>
#include <IBuffer.h>
#include <IRenderer.h>
#include <Transformation.h>

#include <drm_fourcc.h>

#include "TerminalInput.h"
#include "BaseTest.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace Thunder;

namespace {
const Compositor::Color background = { 0.f, 0.f, 0.f, 1.0f };

class RenderTest : public BaseTest {
public:
    RenderTest() = delete;
    RenderTest(const RenderTest&) = delete;
    RenderTest& operator=(const RenderTest&) = delete;

    RenderTest(const std::string& connectorId, const std::string& renderId, const uint16_t framePerSecond, const uint8_t rotationsPerSecond)
        : BaseTest(connectorId, renderId, framePerSecond)
        , _adminLock()
        , _rotations(rotationsPerSecond)
        , _rotation(0.0f)
    {
    }

    ~RenderTest() = default;

protected:
    std::chrono::microseconds NewFrame() override
    {
        const auto start = std::chrono::high_resolution_clock::now();

        const float runtime = std::chrono::duration<float>(start.time_since_epoch()).count();
        float alpha = 0.5f * (1 + sin((2.f * M_PI) * 0.06f * runtime));

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        auto renderer = Renderer();
        auto connector = Connector();

        const uint16_t width(connector->Width());
        const uint16_t height(connector->Height());

        Core::ProxyType<Compositor::IRenderer::IFrameBuffer> frameBuffer = connector->FrameBuffer();

        renderer->Bind(frameBuffer);
        renderer->Begin(width, height);
        renderer->Clear(background);

        constexpr int16_t squareSize(300);

        for (int y = -squareSize; y < height; y += squareSize) {
            for (int x = -squareSize; x < width; x += squareSize) {
                const Exchange::IComposition::Rectangle box = { x, y, squareSize, squareSize };

                float R = (float(width) - float(x)) / (float(width) - float(-squareSize));
                float G = (float(x) - float(-squareSize)) / (float(width) - float(-squareSize));
                float B = (float(y) - float(-squareSize)) / (float(height) - float(-squareSize));

                const Compositor::Color color = { R, G, B, alpha };

                Compositor::Matrix matrix;
                Compositor::Transformation::ProjectBox(matrix, box, Compositor::Transformation::TRANSFORM_FLIPPED_180, _rotation, renderer->Projection());

                renderer->Quadrangle(color, matrix);
            }
        }

        renderer->End();
        renderer->Unbind(frameBuffer);

        connector->Commit();

        WaitForVSync(100);

        auto periodCount = Period().count();
        if (periodCount > 0) {
            _rotation += periodCount * (2. * M_PI) / float(_rotations * std::chrono::microseconds(std::chrono::seconds(1)).count());
        }

        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
    }

private:
    mutable Core::CriticalSection _adminLock;
    const uint8_t _rotations;
    float _rotation;
};

class ConsoleOptions : public Core::Options {
public:
    ConsoleOptions(int argumentCount, TCHAR* arguments[])
        : Core::Options(argumentCount, arguments, _T("o:r:h"))
        , RenderNode("/dev/dri/card0")
        , Output(RenderNode)
    {
        Parse();
    }
    ~ConsoleOptions()
    {
    }

public:
    const TCHAR* RenderNode;
    const TCHAR* Output;

private:
    virtual void Option(const TCHAR option, const TCHAR* argument)
    {
        switch (option) {
        case 'o':
            Output = argument;
            break;
        case 'r':
            RenderNode = argument;
            break;
        case 'h':
        default:
            fprintf(stderr, "Usage: " EXPAND_AND_QUOTE(APPLICATION_NAME) " [-o <HDMI-A-1>] [-r </dev/dri/renderD128>]\n");
            exit(EXIT_FAILURE);
            break;
        }
    }
};
}

int main(int argc, char* argv[])
{
    bool quitApp(false);

    ConsoleOptions options(argc, argv);

    Messaging::LocalTracer& tracer = Messaging::LocalTracer::Open();

    const char* executableName(Core::FileNameOnly(argv[0]));

    {
        TerminalInput keyboard;
        ASSERT(keyboard.IsValid() == true);

        Messaging::ConsolePrinter printer(true);

        tracer.Callback(&printer);

        const std::map<std::string, std::vector<std::string>> modules = {
            { "CompositorRenderTest", { "" } },
            { "CompositorBuffer", { "Error", "Information" } },
            { "CompositorBackend", { "Error", "Information" } },
            { "CompositorRenderer", { "Error", "Warning", "Information" } },
            { "DRMCommon", { "Error", "Information" } }
        };

        for (const auto& module_entry : modules) {
            for (const auto& category : module_entry.second) {
                tracer.EnableMessage(module_entry.first, category, true);
            }
        }

        TRACE_GLOBAL(Trace::Information, ("%s - build: %s", executableName, __TIMESTAMP__));

        RenderTest test(options.Output, options.RenderNode, 60000, 10);

        test.Start();

        if (keyboard.IsValid() == true) {
            while (!test.ShouldExit() && !quitApp) {
                switch (toupper(keyboard.Read())) {
                case 'S':
                    if (test.ShouldExit() == false) {
                        (test.IsRunning() == false) ? test.Start() : test.Stop();
                    }
                    break;
                case 'F':
                    TRACE_GLOBAL(Trace::Information, ("Current FPS: %.2f", test.GetFPS()));
                    break;
                case 'Q':
                    quitApp = true;
                    break;
                case 'H':
                    TRACE_GLOBAL(Trace::Information, ("Available commands:"));
                    TRACE_GLOBAL(Trace::Information, ("  S - Start/Stop the rendering"));
                    TRACE_GLOBAL(Trace::Information, ("  F - Show current FPS"));
                    TRACE_GLOBAL(Trace::Information, ("  Q - Quit the application"));
                    TRACE_GLOBAL(Trace::Information, ("  H - Show this help message"));
                    break;
                default:
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } else {
            TRACE_GLOBAL(Thunder::Trace::Error, ("Failed to initialize keyboard input"));
        }

        test.Stop();
        TRACE_GLOBAL(Thunder::Trace::Information, ("Exiting %s.... ", executableName));
    }

    tracer.Close();
    Core::Singleton::Dispose();

    return 0;
}