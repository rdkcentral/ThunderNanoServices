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

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace Thunder;

namespace Thunder {
const Compositor::Color background = { 0.f, 0.f, 0.f, 1.0f };

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

    RenderTest(const std::string& connectorId, const std::string& renderId, const uint8_t framePerSecond, const uint8_t rotationsPerSecond)
        : _adminLock()
        , _renderer()
        , _connector()
        , _period(std::chrono::microseconds(std::chrono::microseconds(std::chrono::seconds(1)) / framePerSecond))
        , _rotations(rotationsPerSecond)
        , _running(false)
        , _render()
        , _renderFd(::open(renderId.c_str(), O_RDWR))
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
            Compositor::PixelFormat(DRM_FORMAT_XRGB8888, { DRM_FORMAT_MOD_LINEAR }), &_sink);

        ASSERT(_connector.IsValid());

        _renderer = Compositor::IRenderer::Instance(_renderFd);

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

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        _render = std::thread(&RenderTest::Render, this);
    }

    void Stop()
    {
        TRACE(Trace::Information, ("Stopping RenderTest"));

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);
        if (_running) {
            _running = false;

            _vsync.notify_all();

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

        const float runtime = std::chrono::duration<float>(start.time_since_epoch()).count();

        float alpha = 0.5f * (1 + sin((2.f * M_PI) * 0.06f * runtime));

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        const uint16_t width(_connector->Width());
        const uint16_t height(_connector->Height());

        _renderer->Bind(static_cast<Core::ProxyType<Exchange::ICompositionBuffer>>(_connector));

        _renderer->Begin(width, height);
        _renderer->Clear(background);

        constexpr int16_t squareSize(300);

        for (int y = -squareSize; y < height; y += squareSize) {
            for (int x = -squareSize; x < width; x += squareSize) {
                const Exchange::IComposition::Rectangle box = { x, y, squareSize, squareSize };

                float R = (float(width) - float(x)) / (float(width) - float(-squareSize));
                float G = (float(x) - float(-squareSize)) / (float(width) - float(-squareSize));
                float B = (float(y) - float(-squareSize)) / (float(height) - float(-squareSize));

                const Compositor::Color color = { R, G, B, alpha };

                Compositor::Matrix matrix;
                Compositor::Transformation::ProjectBox(matrix, box, Compositor::Transformation::TRANSFORM_FLIPPED_180, rotation, _renderer->Projection());

                _renderer->Quadrangle(color, matrix);
            }
        }

        _renderer->End();

        _connector->Commit();

        WaitForVSync(100);
        
        _renderer->Unbind();

        rotation += _period.count() * (2. * M_PI) / float(_rotations * std::chrono::microseconds(std::chrono::seconds(1)).count());

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
    Core::ProxyType<Compositor::IRenderer> _renderer;
    Core::ProxyType<Compositor::IOutput> _connector;
    uint8_t _previousIndex;
    const std::chrono::microseconds _period;
    const float _rotations;
    bool _running;
    std::thread _render;
    int _renderFd;
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

    Messaging::LocalTracer& tracer = Messaging::LocalTracer::Open();

    const char* executableName(Core::FileNameOnly(argv[0]));

    {
        Messaging::ConsolePrinter printer(true);

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

        TRACE_GLOBAL(Trace::Information, ("%s - build: %s", executableName, __TIMESTAMP__));

        Thunder::RenderTest test(options.Output, options.RenderNode, 100, 5);

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

        TRACE_GLOBAL(Trace::Information, ("Exiting %s.... ", executableName));
    }

    tracer.Close();
    Core::Singleton::Dispose();

    return 0;
}
