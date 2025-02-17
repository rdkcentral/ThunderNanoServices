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

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace Thunder {
const Compositor::Color background = { 0.25f, 0.25f, 0.25f, 1.0f };

class RenderTest {
private:
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

        // virtual void Display(const int fd, const std::string& node) override
        // {
        //     TRACE(Trace::Information, (_T("Connector fd %d opened on %s"), fd, node.c_str()));
        //     // _parent.HandleGPUNode(node);
        // }

    private:
        RenderTest& _parent;
    };

public:
    RenderTest() = delete;
    RenderTest(const RenderTest&) = delete;
    RenderTest& operator=(const RenderTest&) = delete;

    RenderTest(const std::string& connectorId, const std::string& renderId, const uint8_t FPS, const uint16_t RPM)
        : _adminLock()
        , _format(DRM_FORMAT_ABGR8888, { DRM_FORMAT_MOD_LINEAR })
        , _connector()
        , _renderer()
        , _textureBuffer()
        , _texture()
        , _radPerFrame(((RPM / 60.0f) * (2.0f * M_PI)) / float(FPS))
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
            _format, &_sink);

        ASSERT(_connector.IsValid());
        TRACE_GLOBAL(Thunder::Trace::Information, ("created connector: %p", _connector.operator->()));

        ASSERT(_renderFd >= 0);

        _renderer = Compositor::IRenderer::Instance(_renderFd);
        ASSERT(_renderer.IsValid());
        TRACE_GLOBAL(Thunder::Trace::Information, ("created renderer: %p", _renderer.operator->()));

        _textureBuffer = Core::ProxyType<Compositor::DmaBuffer>::Create(_renderFd, Texture::TvTexture);
        _texture = _renderer->Texture(Core::ProxyType<Exchange::ICompositionBuffer>(_textureBuffer));
        ASSERT(_texture != nullptr);
        ASSERT(_texture->IsValid());
        TRACE_GLOBAL(Thunder::Trace::Information, ("created texture: %p", _texture));

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
            NewFrame();
        }
    }

    void NewFrame()
    {
        static float rotation = 0.f;

        const float runtime = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - _renderStart).count();

        float alpha = 0.5f * (1 + sin((2.f * M_PI) * 0.25f * runtime));

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        const uint16_t width(_connector->Width());
        const uint16_t height(_connector->Height());

        const uint16_t renderWidth(720);
        const uint16_t renderHeight(720);

        const float cosX = cos(M_PI * 2.0f * (rotation * (180.0f / M_PI)) / 360.0f);
        const float sinY = sin(M_PI * 2.0f * (rotation * (180.0f / M_PI)) / 360.0f);

        float x = float(renderWidth / 2.0f) * cosX;
        float y = float(renderHeight / 2.0f) * sinY;

        _renderer->Bind(static_cast<Core::ProxyType<Exchange::ICompositionBuffer>>(_connector));

        _renderer->Begin(width, height);
        _renderer->Clear(background);

        // const Compositor::Box renderBox = { ((width / 2) - (renderWidth / 2)), ((height / 2) - (renderHeight / 2)), renderWidth, renderHeight };
        const Exchange::IComposition::Rectangle renderBox = { static_cast<int32_t>(((width / 2) - (renderWidth / 2)) + x), static_cast<int32_t>(((height / 2) - (renderHeight / 2)) + y), renderWidth, renderHeight };

        Compositor::Matrix matrix;
        Compositor::Transformation::ProjectBox(matrix, renderBox, Compositor::Transformation::TRANSFORM_FLIPPED_180, 0, _renderer->Projection());

        const Exchange::IComposition::Rectangle textureBox = { 0, 0, _texture->Width(), _texture->Height() };
        _renderer->Render(_texture, textureBox, matrix, alpha);

        _renderer->End(false);

        uint32_t commit;

        if ((commit = _connector->Commit()) == Core::ERROR_NONE) {
            WaitForVSync(100);
        } else {
            TRACE(Trace::Error, ("Commit failed: %d", commit));
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // just throttle the render thread a bit. 
        }

        _renderer->Unbind();

        rotation += _radPerFrame;
    }

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
    Core::ProxyType<Compositor::DmaBuffer> _textureBuffer;
    Core::ProxyType<Compositor::IRenderer::ITexture> _texture;
    const float _radPerFrame;
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
            "CompositorBackendOFF",
            "CompositorRendererOFF",
            "DRMCommon"
        };

        for (auto module : modules) {
            tracer.EnableMessage(module, "", true);
        }

        TRACE_GLOBAL(Thunder::Trace::Information, ("%s - build: %s", executableName, __TIMESTAMP__));

        Thunder::RenderTest test(options.Output, options.RenderNode, options.FPS, options.RPM);

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
