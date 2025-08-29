/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Metrological B.V.
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

#include <condition_variable>
#include <inttypes.h>
#include <iterator>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include <IBuffer.h>
#include <IOutput.h>
#include <IRenderer.h>
#include <Transformation.h>

#include <DRM.h>

#include <drm_fourcc.h>

#include <DmaBuffer.h>

#include "BaseTest.h"
#include "TerminalInput.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace Thunder;

namespace {
const Compositor::Color background = { 0.25f, 0.25f, 0.25f, 1.0f };

class RenderTest : public BaseTest {
public:
    RenderTest() = delete;
    RenderTest(const RenderTest&) = delete;
    RenderTest& operator=(const RenderTest&) = delete;

    RenderTest(const std::string& connectorId, const std::string& renderId, const uint16_t FPS, const uint16_t RPM)
        : BaseTest(connectorId, renderId, FPS)
        , _adminLock()
        , _format(DRM_FORMAT_ABGR8888, { DRM_FORMAT_MOD_LINEAR })
        , _textureBuffer()
        , _texture()
        , _radPerFrame(((RPM / 60.0f) * (2.0f * M_PI)) / float(FPS / 1000.0f)) // Convert mFPS to FPS
        , _renderStart(std::chrono::high_resolution_clock::now())
    {
        auto renderer = Renderer();
        int renderFd = ::open(renderId.c_str(), O_RDWR);
        ASSERT(renderFd >= 0);

        _textureBuffer = Core::ProxyType<Compositor::DmaBuffer>::Create(renderFd, Texture::TvTexture);
        _texture = renderer->Texture(Core::ProxyType<Exchange::IGraphicsBuffer>(_textureBuffer));
        ASSERT(_texture != nullptr);
        ASSERT(_texture->IsValid());
        TRACE_GLOBAL(Thunder::Trace::Information, ("created texture: %p", _texture));

        ::close(renderFd);
    }

    ~RenderTest()
    {
        _texture.Release();
        _textureBuffer.Release();
    }

protected:
    std::chrono::microseconds NewFrame() override
    {
        static float rotation = 0.f;

        const float runtime = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - _renderStart).count();
        float alpha = 0.5f * (1 + sin((2.f * M_PI) * 0.25f * runtime));

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        auto renderer = Renderer();
        auto connector = Connector();
        
        const uint16_t width(connector->Width());
        const uint16_t height(connector->Height());
        const uint16_t renderWidth(720);
        const uint16_t renderHeight(720);

        const float cosX = cos(M_PI * 2.0f * (rotation * (180.0f / M_PI)) / 360.0f);
        const float sinY = sin(M_PI * 2.0f * (rotation * (180.0f / M_PI)) / 360.0f);

        float x = float(renderWidth / 2.0f) * cosX;
        float y = float(renderHeight / 2.0f) * sinY;

        Core::ProxyType<Compositor::IRenderer::IFrameBuffer> frameBuffer = connector->FrameBuffer();

        renderer->Bind(frameBuffer);
        renderer->Begin(width, height);
        renderer->Clear(background);

        const Exchange::IComposition::Rectangle renderBox = { 
            static_cast<int32_t>(((width / 2) - (renderWidth / 2)) + x), 
            static_cast<int32_t>(((height / 2) - (renderHeight / 2)) + y), 
            renderWidth, renderHeight 
        };

        Compositor::Matrix matrix;
        Compositor::Transformation::ProjectBox(matrix, renderBox, Compositor::Transformation::TRANSFORM_FLIPPED_180, 0, renderer->Projection());

        const Exchange::IComposition::Rectangle textureBox = { 0, 0, _texture->Width(), _texture->Height() };
        renderer->Render(_texture, textureBox, matrix, alpha);

        renderer->End(false);
        renderer->Unbind(frameBuffer);

        uint32_t commit;
        if ((commit = connector->Commit()) == Core::ERROR_NONE) {
            WaitForVSync(100);
        } else {
            TRACE(Trace::Error, ("Commit failed: %d", commit));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        rotation += _radPerFrame;
        
        return std::chrono::microseconds(0); // Timing handled by BaseTest
    }

private:
    mutable Core::CriticalSection _adminLock;
    const Compositor::PixelFormat _format;
    Core::ProxyType<Compositor::DmaBuffer> _textureBuffer;
    Core::ProxyType<Compositor::IRenderer::ITexture> _texture;
    const float _radPerFrame;
    std::chrono::time_point<std::chrono::high_resolution_clock> _renderStart;
};

class ConsoleOptions : public Core::Options {
public:
    ConsoleOptions(int argumentCount, TCHAR* arguments[])
        : Core::Options(argumentCount, arguments, _T("o:r:f:s:h"))
        , RenderNode("/dev/dri/card0")
        , Output(RenderNode)
        , FPS(60)
        , RPM(1)
    {
        Parse();
    }
    ~ConsoleOptions()
    {
    }

public:
    const TCHAR* RenderNode;
    const TCHAR* Output;
    uint16_t FPS;
    uint16_t RPM;

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
        case 'f':
            FPS = std::stoi(argument);
            break;
        case 's':
            RPM = std::stoi(argument);
            break;
        case 'h':
        default:
            fprintf(stderr, "Usage: " EXPAND_AND_QUOTE(APPLICATION_NAME) " [-o <HDMI-A-1>] [-r </dev/dri/renderD128>] [-f <fps>] [-s <rpm>]\n");
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

        RenderTest test(options.Output, options.RenderNode, options.FPS * 1000, options.RPM);

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