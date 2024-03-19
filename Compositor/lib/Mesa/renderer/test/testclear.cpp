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

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <inttypes.h>

#include <IBackend.h>
#include <IBuffer.h>
#include <IRenderer.h>
#include <Transformation.h>

#include <drm_fourcc.h>

#include <simpleworker/SimpleWorker.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace WPEFramework;

namespace {
const Compositor::Color red = { 1.f, 0.f, 0.f, 1.f };

class ClearTest {
public:
    ClearTest() = delete;
    ClearTest(const ClearTest&) = delete;
    ClearTest& operator=(const ClearTest&) = delete;

    ClearTest(const std::string& connectorId, const uint8_t fps)
        : _adminLock()
        , _lastFrame(0)
        , _sink(*this)
        , _renderer()
        , _connector()
        , _color(red)
        , _fps(fps)
        , _previousIndex(0)

    {
        _connector = Compositor::Connector(
            connectorId, 
            { 0, 0, 1080, 1920 }, 
            Compositor::PixelFormat(DRM_FORMAT_XRGB8888, { DRM_FORMAT_MOD_LINEAR }));

        ASSERT(_connector.IsValid());

        _renderer = Compositor::IRenderer::Instance(_connector->Identifier());

        ASSERT(_renderer.IsValid());

        NewFrame(Core::Time::Now().Ticks());
    }

    ~ClearTest()
    {
        Stop();

        _renderer.Release();
        _connector.Release();
    }

    void Start()
    {
        TRACE(Trace::Information, ("Starting ClearTest"));

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);
        _lastFrame = Core::Time::Now().Ticks();
        Core::SimpleWorker::Instance().Submit(&_sink);
    }

    void Stop()
    {
        TRACE(Trace::Information, ("Stopping ClearTest"));

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);
        Core::SimpleWorker::Instance().Revoke(&_sink);
        _lastFrame = 0;
    }

    bool Running() const
    {
        return (_lastFrame != 0);
    }

private:
    class Sink : public Core::SimpleWorker::ICallback {
    public:
        Sink() = delete;
        Sink(const Sink&) = delete;
        Sink& operator=(const Sink&) = delete;

        Sink(ClearTest& parent)
            : _parent(parent)
        {
        }

        virtual ~Sink() = default;

    public:
        uint64_t Activity(const uint64_t time)
        {
            return _parent.NewFrame(time);
        }

    private:
        ClearTest& _parent;
    };

    uint64_t NewFrame(const uint64_t scheduledTime)
    {
        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        if (Running() == false) {
            TRACE(Trace::Information, ("Canceling render attempt"));
            return 0;
        }

        const long ms((scheduledTime - _lastFrame) / (Core::Time::TicksPerMillisecond));

        const uint8_t index((_previousIndex + 1) % 3);

        _color[index] += ms / 2000.0f;
        _color[_previousIndex] -= ms / 2000.0f;

        if (_color[_previousIndex] < 0.0f) {
            _color[index] = 1.0f;
            _color[_previousIndex] = 0.0f;
            _previousIndex = index;
        }

        // TRACE(Trace::Information, ("%f FPS, Colour[r=%f, g=%f, b=%f, a=%f]", float(Core::Time::MilliSecondsPerSecond / ms), _color[0], _color[1], _color[2], _color[3]));

        _renderer->Bind(_connector);

        _renderer->Begin(_connector->Width(), _connector->Height());
        _renderer->Clear(_color);
        _renderer->End();

        _connector->Render();

        _renderer->Unbind();

        _lastFrame = scheduledTime;

        return scheduledTime + ((Core::Time::MilliSecondsPerSecond * Core::Time::TicksPerMillisecond) / _fps);
    }

private:
    mutable Core::CriticalSection _adminLock;
    uint64_t _lastFrame;
    Sink _sink;
    Core::ProxyType<Compositor::IRenderer> _renderer;
    Core::ProxyType<Exchange::ICompositionBuffer> _connector;
    Compositor::Color _color;
    const uint8_t _fps;
    uint8_t _previousIndex;
}; // ClearTest

}

int main(int argc, const char* argv[])
{
    std::string connectorId;

    if (argc == 1) {
        connectorId = "card1-HDMI-A-1";
    } else {
        connectorId = argv[1];
    }

    Messaging::LocalTracer& tracer = Messaging::LocalTracer::Open();

    const char* executableName(Core::FileNameOnly(argv[0]));

    {
        Messaging::ConsolePrinter printer(true);

        tracer.Callback(&printer);

        const std::vector<string> modules = {
            "CompositorClearTest",
            "CompositorBuffer",
            "CompositorBackend",
            "CompositorRenderer",
            "DRMCommon"
        };

        for (auto module : modules) {
            tracer.EnableMessage(module, "", true);
        }

        TRACE_GLOBAL(Trace::Information, ("%s - build: %s", executableName, __TIMESTAMP__));

        ClearTest test(connectorId, 60);

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
