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

#include <IBuffer.h>
#include <IRenderer.h>
#include <Transformation.h>

#include <DRM.h>

#include <drm_fourcc.h>

#include <simpleworker/SimpleWorker.h>

#include <DmaBuffer.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
const Compositor::Color background = { 0.25f, 0.25f, 0.25f, 1.0f };

class RenderTest {

public:
    RenderTest() = delete;
    RenderTest(const RenderTest&) = delete;
    RenderTest& operator=(const RenderTest&) = delete;

    RenderTest(const std::string& connectorId, const uint8_t fps)
        : _adminLock()
        , _lastFrame(0)
        , _sink(*this)
        , _format(DRM_FORMAT_ARGB8888, { DRM_FORMAT_MOD_LINEAR })
        , _connector()
        , _renderer()
        , _textureBuffer()
        , _fps(fps)
        , _texture(nullptr)
    {
        _connector = Compositor::Connector(connectorId, Exchange::IComposition::ScreenResolution::ScreenResolution_1080p, _format, false);
        ASSERT(_connector.IsValid());
        TRACE_GLOBAL(WPEFramework::Trace::Information, ("created connector: %p", _connector.operator->()));

        _renderer = Compositor::IRenderer::Instance(_connector->Identifier());
        ASSERT(_renderer.IsValid());
        TRACE_GLOBAL(WPEFramework::Trace::Information, ("created renderer: %p", _renderer.operator->()));

        _textureBuffer = Core::ProxyType<Compositor::DmaBuffer>::Create(_connector->Identifier(), Texture::TvTexture);
        _texture = _renderer->Texture(_textureBuffer.operator->());
        TRACE_GLOBAL(WPEFramework::Trace::Information, ("created texture: %d", _texture));
    }

    ~RenderTest()
    {
        Stop();

        _renderer.Release();
        _connector.Release();
        _textureBuffer.Release();
    }

    void Start()
    {
        TRACE(Trace::Information, ("Starting RenderTest"));

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);
        _lastFrame = Core::Time::Now().Ticks();
        Core::SimpleWorker::Instance().Submit(&_sink);
    }

    void Stop()
    {
        TRACE(Trace::Information, ("Stopping RenderTest"));

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

        Sink(RenderTest& parent)
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
        RenderTest& _parent;
    };

    uint64_t NewFrame(const uint64_t scheduledTime)
    {
        static float rotation = 0.f;

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        if (Running() == false) {
            TRACE(Trace::Information, ("Canceling render attempt"));
            return 0;
        }

        // const long ms((scheduledTime - _lastFrame) / (Core::Time::TicksPerMillisecond));

        const uint16_t width(_connector->Width());
        const uint16_t height(_connector->Height());

        const uint16_t renderWidth(256);
        const uint16_t renderHeight(256);

        _renderer->Bind(_connector);

        _renderer->Begin(width, height);
        _renderer->Clear(background);

        const Compositor::Box renderBox = { ((width / 2) - (renderWidth / 2)), ((height / 2) - (renderHeight / 2)), renderWidth, renderHeight };
        Compositor::Matrix matrix;
        Compositor::Transformation::ProjectBox(matrix, renderBox, Compositor::Transformation::TRANSFORM_NORMAL, rotation, _renderer->Projection());

        const Compositor::Box textureBox = { 0, 0, int(_texture->Width()), int(_texture->Height()) };
        _renderer->Render(_texture, textureBox, matrix, 1.0f);

        _renderer->End(false);

        _connector->Render();

        _renderer->Unbind();

        // TODO rotate with a delta time
        rotation += 0.05;
        if (rotation > 2 * M_PI) {
            rotation = 0.f;
        }

        _lastFrame = scheduledTime;

        return scheduledTime + ((Core::Time::MilliSecondsPerSecond * Core::Time::TicksPerMillisecond) / _fps);
    }

private:
    mutable Core::CriticalSection _adminLock;
    uint64_t _lastFrame;
    Sink _sink;
    const Compositor::PixelFormat _format;
    Core::ProxyType<Exchange::ICompositionBuffer> _connector;
    Core::ProxyType<Compositor::IRenderer> _renderer;
    Core::ProxyType<Compositor::DmaBuffer> _textureBuffer;
    const uint8_t _fps;
    Compositor::IRenderer::ITexture* _texture;
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

        WPEFramework::RenderTest test(connectorId, 10);

        // test.Start();

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
