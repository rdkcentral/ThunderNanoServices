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

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <inttypes.h>

#include <IBackend.h>
#include <IBuffer.h>
#include <IRenderer.h>
#include <Transformation.h>

#include <drm_fourcc.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace WPEFramework;

namespace WPEFramework  {
const Compositor::Color background = { 0.25f, 0.25f, 0.25f, 1.0f };

class RenderTest {
public:
    RenderTest() = delete;
    RenderTest(const RenderTest&) = delete;
    RenderTest& operator=(const RenderTest&) = delete;

    RenderTest(const std::string& connectorId, const uint8_t framePerSecond, const uint8_t rotationsPerSecond)

        : _adminLock()
        , _renderer()
        , _connector()
        , _period(std::chrono::microseconds(std::chrono::microseconds(std::chrono::seconds(1)) / framePerSecond))
        , _rotations(rotationsPerSecond)
        , _running(false)
        , _render()

    {
        _connector = Compositor::Connector(
            connectorId,
            { 0, 0, 1080, 1920 },
            Compositor::PixelFormat(DRM_FORMAT_XRGB8888, { DRM_FORMAT_MOD_LINEAR }));

        ASSERT(_connector.IsValid());

        _renderer = Compositor::IRenderer::Instance(_connector->Identifier());

        ASSERT(_renderer.IsValid());

        NewFrame();
    }

    ~RenderTest()
    {
        Stop();

        _renderer.Release();
        _connector.Release();
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

        _renderer->Bind(_connector);

        _renderer->Begin(width, height);
        _renderer->Clear(background);

        constexpr int16_t squareSize(300);

        for (int y = -squareSize; y < height; y += squareSize) {
            for (int x = -squareSize; x < width; x += squareSize) {
                const Compositor::Box box = { x, y, squareSize, squareSize };

                const Compositor::Color color = { 255.f / x, 255.f / y, 255.f / (x + y), 1.f };

                Compositor::Matrix matrix;
                Compositor::Transformation::ProjectBox(matrix, box, Compositor::Transformation::TRANSFORM_FLIPPED_180, rotation, _renderer->Projection());

                _renderer->Quadrangle(color, matrix);
            }
        }

        _renderer->End();

        _connector->Render();

        _renderer->Unbind();

        rotation += _period.count() * (2. * M_PI) / float(_rotations * std::chrono::microseconds(std::chrono::seconds(1)).count());

        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
    }

private:
    mutable Core::CriticalSection _adminLock;
    Core::ProxyType<Compositor::IRenderer> _renderer;
    Core::ProxyType<Exchange::ICompositionBuffer> _connector;
    uint8_t _previousIndex;
    const std::chrono::microseconds _period;
    const float _rotations;
    bool _running;
    std::thread _render;
}; // RenderTest

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

        WPEFramework::RenderTest test(connectorId, 100, 5);

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
