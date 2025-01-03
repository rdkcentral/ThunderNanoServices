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
#define MODULE_NAME CompositorClearTest
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

namespace {
const Compositor::Color red = { 1.f, 0.f, 0.f, 1.f };

class ClearTest {

    class Sink : public Compositor::IOutput::ICallback {
    public:
        Sink(const Sink&) = delete;
        Sink& operator=(const Sink&) = delete;
        Sink() = delete;

        Sink(ClearTest& parent)
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
        ClearTest& _parent;
    };

public:
    ClearTest() = delete;
    ClearTest(const ClearTest&) = delete;
    ClearTest& operator=(const ClearTest&) = delete;

    ClearTest(const std::string& connectorId, const std::string& renderId, const uint8_t framePerSecond, const uint8_t rotationsPerSecond)
        : _adminLock()
        , _renderer()
        , _connector()
        , _color(red)
        , _previousIndex(0)
        , _period(std::chrono::microseconds(std::chrono::seconds(1)) / framePerSecond)
        , _rotationPeriod(std::chrono::seconds(rotationsPerSecond))
        , _rotation(0)
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

    ~ClearTest()
    {
        Stop();

        _renderer.Release();
        _connector.Release();

        ::close(_renderFd);
    }

    void Start()
    {
        TRACE(Trace::Information, ("Starting ClearTest"));

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        _render = std::thread(&ClearTest::Render, this);
    }

    void Stop()
    {
        TRACE(Trace::Information, ("Stopping ClearTest"));

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

    Compositor::Color hsv2rgb(float H, float S, float V)
    {
        Compositor::Color RGB;

        float P, Q, T, fract;

        (H == 360.) ? (H = 0.) : (H /= 60.);
        fract = H - floor(H);

        P = V * (1. - S);
        Q = V * (1. - S * fract);
        T = V * (1. - S * (1. - fract));

        if (0. <= H && H < 1.)
            RGB = { V, T, P, 1. };
        else if (1. <= H && H < 2.)
            RGB = { Q, V, P, 1. };
        else if (2. <= H && H < 3.)
            RGB = { P, V, T, 1. };
        else if (3. <= H && H < 4.)
            RGB = { P, Q, V, 1. };
        else if (4. <= H && H < 5.)
            RGB = { T, P, V, 1. };
        else if (5. <= H && H < 6.)
            RGB = { V, P, Q, 1. };
        else
            RGB = { 0., 0., 0., 1. };

        return RGB;
    }

    std::chrono::microseconds NewFrame()
    {
        const auto start = std::chrono::high_resolution_clock::now();

        _renderer->Bind(static_cast<Core::ProxyType<Exchange::ICompositionBuffer>>(_connector));

        _renderer->Begin(_connector->Width(), _connector->Height());
        _renderer->Clear(_color);
        _renderer->End();

        _connector->Commit();

        _renderer->Unbind();

        _rotation += 360. / (_rotationPeriod.count() * (std::chrono::microseconds(std::chrono::seconds(1)).count() / _period.count()));

        if (_rotation >= 360)
            _rotation = 0;

        _color = hsv2rgb(_rotation, .9, .7);

        WaitForVSync(100);

        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
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
    Core::ProxyType<Compositor::IRenderer> _renderer;
    Core::ProxyType<Compositor::IOutput> _connector;
    Compositor::Color _color;
    uint8_t _previousIndex;
    const std::chrono::microseconds _period;
    const std::chrono::seconds _rotationPeriod;
    float _rotation;
    bool _running;
    std::thread _render;
    int _renderFd;
    Sink _sink;
    std::mutex _rendering;
    std::condition_variable _vsync;
    uint64_t _ppts;
    float _fps;
    uint32_t _sequence;

}; // ClearTest
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
    ConsoleOptions options(argc, argv);

    Messaging::LocalTracer& tracer = Messaging::LocalTracer::Open();

    const char* executableName(Core::FileNameOnly(argv[0]));

    {
        Messaging::ConsolePrinter printer(true);

        tracer.Callback(&printer);

        const std::vector<string> modules = {
            "CompositorClearTest",
            "CompositorBuffer",
            "CompositorBackendOff",
            "CompositorRendererOff",
            "DRMCommon"
        };

        for (auto module : modules) {
            tracer.EnableMessage(module, "", true);
        }

        TRACE_GLOBAL(Trace::Information, ("%s - build: %s", executableName, __TIMESTAMP__));

        ClearTest test(options.Output, options.RenderNode, 100, 10);

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

        test.Stop();
        TRACE_GLOBAL(Trace::Information, ("Exiting %s.... ", executableName));
    }

    tracer.Close();
    Core::Singleton::Dispose();

    return 0;
}
