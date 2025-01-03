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

#include <PixelBuffer.h>

#include "../../src/GL/Format.h"

#include <core/core.h>
#include <localtracer/localtracer.h>
#include <messaging/messaging.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace Thunder {
const Compositor::Color background = { 0.25f, 0.25f, 0.25f, 1.0f };

static Core::ProxyType<Compositor::PixelBuffer> textureRed(Core::ProxyType<Compositor::PixelBuffer>::Create(Texture::Red));
static Core::ProxyType<Compositor::PixelBuffer> textureGreen(Core::ProxyType<Compositor::PixelBuffer>::Create(Texture::Green));
static Core::ProxyType<Compositor::PixelBuffer> textureBlue(Core::ProxyType<Compositor::PixelBuffer>::Create(Texture::Blue));

static Core::ProxyType<Compositor::PixelBuffer> textureSimple(Core::ProxyType<Compositor::PixelBuffer>::Create(Texture::Simple));

static Core::ProxyType<Compositor::PixelBuffer> textureTv(Core::ProxyType<Compositor::PixelBuffer>::Create(Texture::TvTexture));

class RenderTest {
    class Sink : public Compositor::IOutput::ICallback {
    public:
        Sink(const Sink&) = delete;
        Sink& operator=(const Sink&) = delete;
        Sink() = delete;

        Sink(RenderTest& parent)
            : _parent(parent)
        {
        }

        virtual ~Sink() = default;

        virtual void Presented(const Compositor::IOutput* output, const uint32_t sequence, const uint64_t time) override
        {
            _parent.HandleVSync(output, sequence, time);
        }
    private:
        RenderTest& _parent;
    };
public:
    RenderTest() = delete;
    RenderTest(const RenderTest&) = delete;
    RenderTest& operator=(const RenderTest&) = delete;

    RenderTest(const std::string& connectorId, const std::string& renderId, const uint8_t framePerSecond, const uint8_t rotationsPerSecond)
        : _adminLock()
        , _format(DRM_FORMAT_ABGR8888, { DRM_FORMAT_MOD_LINEAR })
        , _connector()
        , _renderer()
        , _texture()
        , _period(std::chrono::microseconds(std::chrono::microseconds(std::chrono::seconds(1)) / framePerSecond))
        , _rotations(rotationsPerSecond)
        , _running(false)
        , _render()
        , _renderFd(::open(renderId.c_str(), O_RDWR))
        , _renderStart(std::chrono::high_resolution_clock::now())
        , _sink(*this)
        , _rendering()
        , _vsync()
        , _ppts(Core::Time::Now().Ticks())
        , _fps()
        , _sequence(0)
    {
        _connector = Compositor::CreateBuffer(
            connectorId,
            { 0, 0, 1080, 1920 },
            _format,
            &_sink);

        ASSERT(_connector.IsValid());

        _renderer = Compositor::IRenderer::Instance(_renderFd);

        _texture = _renderer->Texture(Core::ProxyType<Exchange::ICompositionBuffer>(textureTv));

        ASSERT(_renderer.IsValid());

        NewFrame();
    }

    ~RenderTest()
    {
        Stop();

        _renderer.Release();
        _connector.Release();

        ::close(_renderFd);
    }

    void Start()
    {
        TRACE(Trace::Information, ("Starting RenderTest"));

        _renderStart = std::chrono::high_resolution_clock::now();

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

        //const float runtime = std::chrono::duration<float>(start.time_since_epoch()).count();
        const float runtime = std::chrono::duration<float>(start - _renderStart).count();

        float alpha = 0.5f * (1 + sin((2.f * M_PI) * 0.25f * runtime));

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        const uint16_t width(_connector->Width());
        const uint16_t height(_connector->Height());

        const uint16_t renderWidth(512);
        const uint16_t renderHeight(512);

        _renderer->Bind(static_cast<Core::ProxyType<Exchange::ICompositionBuffer>>(_connector));

        _renderer->Begin(width, height);
        _renderer->Clear(background);

        const Exchange::IComposition::Rectangle renderBox = { (width / 2) - (renderWidth / 2), (height / 2) - (renderHeight / 2), renderWidth, renderHeight };
        Compositor::Matrix matrix;

        Compositor::Transformation::ProjectBox(matrix, renderBox, Compositor::Transformation::TRANSFORM_FLIPPED_180, rotation, _renderer->Projection());

        const Exchange::IComposition::Rectangle textureBox = { 0, 0, _texture->Width(), _texture->Height() };
        _renderer->Render(_texture, textureBox, matrix, alpha);

        _renderer->End(false);

        _connector->Commit();

        WaitForVSync(100);

        _renderer->Unbind();

        rotation += _period.count() * (2. * M_PI) / (_rotations * std::chrono::microseconds(std::chrono::seconds(1)).count());

        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
    }

private:
    void HandleVSync(const Compositor::IOutput* output VARIABLE_IS_NOT_USED, const uint32_t sequence, uint64_t pts /*usec from epoch*/)
    {
        _fps = 1 / ((pts - _ppts) / 1000000.0f);
        _sequence = sequence;
        _ppts = pts;
        _vsync.notify_all();
    }

    void WaitForVSync(uint32_t timeoutMs)
    {
        std::unique_lock<std::mutex> lock(_rendering);

        if (timeoutMs == Core::infinite) {
            _vsync.wait(lock);
        } else {
            _vsync.wait_for(lock, std::chrono::milliseconds(timeoutMs));
        }
        TRACE(Trace::Information, ("Connector running at %.2f fps", _fps));
    }

private:
    mutable Core::CriticalSection _adminLock;
    const Compositor::PixelFormat _format;
    Core::ProxyType<Compositor::IOutput> _connector;
    Core::ProxyType<Compositor::IRenderer> _renderer;
    Core::ProxyType<Compositor::IRenderer::ITexture> _texture;
    const std::chrono::microseconds _period;
    const uint8_t _rotations;
    bool _running;
    std::thread _render;
    int _renderFd;
    std::chrono::time_point<std::chrono::high_resolution_clock> _renderStart;
    Sink _sink;
    std::mutex _rendering;
    std::condition_variable _vsync;
    uint64_t _ppts;
    float _fps;
    uint32_t _sequence;
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
    Thunder::ConsoleOptions options(argc, argv);
    Thunder::Messaging::LocalTracer& tracer = Thunder::Messaging::LocalTracer::Open();

    const char* executableName(Thunder::Core::FileNameOnly(argv[0]));

    {
        Thunder::Messaging::ConsolePrinter printer(true);

        tracer.Callback(&printer);

        const std::vector<string> modules = {
            "CompositorRenderTest",
            "CompositorBuffer",
            "CompositorBackendOff",
            "CompositorRendererOff",
            "DRMCommon"
        };

        for (auto module : modules) {
            tracer.EnableMessage(module, "", true);
        }

        TRACE_GLOBAL(Thunder::Trace::Information, ("%s - build: %s", executableName, __TIMESTAMP__));

        Thunder::RenderTest test(options.Output, options.RenderNode, 60, 30);

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

        TRACE_GLOBAL(Thunder::Trace::Information, ("Exiting %s.... ", executableName));
    }

    tracer.Close();
    Thunder::Core::Singleton::Dispose();

    return 0;
}
