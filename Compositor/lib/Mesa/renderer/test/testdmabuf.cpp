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

#include <iterator>
#include <sstream>
#include <string>
#include <vector>
#include <inttypes.h>

#include <IBackend.h>
#include <IBuffer.h>
#include <IRenderer.h>
#include <Transformation.h>

#include <DRM.h>

#include <drm_fourcc.h>

#include <DmaBuffer.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
const Compositor::Color background = { 0.25f, 0.25f, 0.25f, 1.0f };

class RenderTest {

public:
    RenderTest() = delete;
    RenderTest(const RenderTest&) = delete;
    RenderTest& operator=(const RenderTest&) = delete;

    RenderTest(const std::string& connectorId, const std::string& renderId, const uint8_t framePerSecond, const uint8_t rotationsPerSecond)
        : _adminLock()
        , _format(DRM_FORMAT_ABGR8888, { DRM_FORMAT_MOD_LINEAR })
        , _connector()
        , _renderer()
        , _textureBuffer()
        , _texture(nullptr)
        , _period(std::chrono::microseconds(std::chrono::microseconds(std::chrono::seconds(1)) / framePerSecond))
        , _rotations(rotationsPerSecond)
        , _running(false)
        , _render()
        , _renderFd(::open(renderId.c_str(), O_RDWR))
    {
        _connector = Compositor::Connector(
            connectorId,
            { 0, 0, 1080, 1920 },
            _format);

        ASSERT(_connector.IsValid());
        TRACE_GLOBAL(WPEFramework::Trace::Information, ("created connector: %p", _connector.operator->()));

        ASSERT(_renderFd >= 0);

        _renderer = Compositor::IRenderer::Instance(_renderFd);
        ASSERT(_renderer.IsValid());
        TRACE_GLOBAL(WPEFramework::Trace::Information, ("created renderer: %p", _renderer.operator->()));

        _textureBuffer = Core::ProxyType<Compositor::DmaBuffer>::Create(_renderFd, Texture::TvTexture);
        _texture = _renderer->Texture(_textureBuffer.operator->());
        ASSERT(_texture != nullptr);
        ASSERT(_texture->IsValid());
        TRACE_GLOBAL(WPEFramework::Trace::Information, ("created texture: %p", _texture));

        NewFrame();
    }

    ~RenderTest()
    {
        Stop();

        _renderer.Release();
        _connector.Release();
        _textureBuffer.Release();

        ::close(_renderFd);
    }

    void Start()
    {
        TRACE(Trace::Information, ("Starting RenderTest"));

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);
        _render = std::thread(&RenderTest::Render, this);
    }

    void Stop()
    {
        TRACE(Trace::Information, ("Stopping RenderTest"));

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        if (_running) {
            _running = false;
            _render.join();
        }
    }

    bool Running() const
    {
        return _running;
    }

private:
    void Render()
    {
        _running = true;

        while (_running) {
            const auto next = _period - NewFrame();
            std::this_thread::sleep_for((next.count() > 0) ? next : std::chrono::microseconds(0));
        }
    }

    std::chrono::microseconds NewFrame()
    {
        static float rotation = 0.f;

        const auto start = std::chrono::high_resolution_clock::now();

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        const uint16_t width(_connector->Width());
        const uint16_t height(_connector->Height());

        const uint16_t renderWidth(720);
        const uint16_t renderHeight(720);

        _renderer->Bind(_connector);

        _renderer->Begin(width, height);
        _renderer->Clear(background);

        const Compositor::Box renderBox = { ((width / 2) - (renderWidth / 2)), ((height / 2) - (renderHeight / 2)), renderWidth, renderHeight };
        Compositor::Matrix matrix;
        Compositor::Transformation::ProjectBox(matrix, renderBox, Compositor::Transformation::TRANSFORM_FLIPPED_180, rotation, _renderer->Projection());

        const Compositor::Box textureBox = { 0, 0, int(_texture->Width()), int(_texture->Height()) };
        _renderer->Render(_texture, textureBox, matrix, 1.0f);

        _renderer->End(false);

        _connector->Render();

        _renderer->Unbind();

        rotation += _period.count() * (2. * M_PI) / float(_rotations * std::chrono::microseconds(std::chrono::seconds(1)).count());

        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
    }

private:
    mutable Core::CriticalSection _adminLock;
    const Compositor::PixelFormat _format;
    Core::ProxyType<Exchange::ICompositionBuffer> _connector;
    Core::ProxyType<Compositor::IRenderer> _renderer;
    Core::ProxyType<Compositor::DmaBuffer> _textureBuffer;
    Compositor::IRenderer::ITexture* _texture;
    const std::chrono::microseconds _period;
    const float _rotations;
    bool _running;
    std::thread _render;
    int _renderFd;
}; // RenderTest

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
    WPEFramework::ConsoleOptions options(argc, argv);

    WPEFramework::Messaging::LocalTracer& tracer = WPEFramework::Messaging::LocalTracer::Open();

    const char* executableName(WPEFramework::Core::FileNameOnly(argv[0]));

    {
        WPEFramework::Messaging::ConsolePrinter printer(true);

        tracer.Callback(&printer);

        const std::vector<string> modules = {
            "CompositorRenderTest",
            "CompositorBuffer",
            "CompositorBackend",
            "CompositorRenderer",
            "DRMCommon"
        };

        for (auto module : modules) {
            tracer.EnableMessage(module, "", true);
        }

        TRACE_GLOBAL(WPEFramework::Trace::Information, ("%s - build: %s", executableName, __TIMESTAMP__));

        WPEFramework::RenderTest test(options.Output, options.RenderNode, 120, 10);

        test.Start();

        char keyPress;

        do {
            keyPress = toupper(getchar());

            switch (keyPress) {
            case 'S': {
                (test.Running() == false) ? test.Start() : test.Stop();
                break;
            }
            default:
                break;
            }

        } while (keyPress != 'Q');

        TRACE_GLOBAL(WPEFramework::Trace::Information, ("Exiting %s.... ", executableName));
    }

    tracer.Close();
    WPEFramework::Core::Singleton::Dispose();

    return 0;
}
