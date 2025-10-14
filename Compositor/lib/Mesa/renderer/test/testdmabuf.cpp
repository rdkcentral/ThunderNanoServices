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

    RenderTest(const std::string& connectorId, const std::string& renderId, const uint16_t FPS, const uint16_t width, const uint16_t height)
        : BaseTest(connectorId, renderId, FPS, width, height)
        , _adminLock()
        , _textureBuffer()
        , _texture()
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

    virtual ~RenderTest()
    {
        _texture.Release();
        _textureBuffer.Release();
    }

protected:
    std::chrono::microseconds NewFrame() override
    {
        static constexpr float RADIANS_PER_REVOLUTION = 2.0f * M_PI;
        static constexpr float BASE_SPEED = 150.0f; // Base speed in pixels per second
        static constexpr float MIN_SPEED = 50.0f; // Minimum speed
        static constexpr float MAX_SPEED = 800.0f; // Maximum speed
        static constexpr float BOUNCE_BOOST = 1.3f; // Speed multiplier on bounce
        static constexpr float FRICTION = 0.98f; // Speed decay factor per second
        static constexpr uint16_t TEXTURE_SIZE = 400; // Size of the texture in pixels

        // Static variables to maintain state between frames
        static float posX = 0.0f;
        static float posY = 0.0f;
        static float velocityX = BASE_SPEED;
        static float velocityY = BASE_SPEED * 0.7f;
        static bool initialized = false;

        const float runtime = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - _renderStart).count();

        // Simple time-based rotation (1 revolution per 3 seconds)
        float rotation = runtime * RADIANS_PER_REVOLUTION / 3.0f;

        // Alpha oscillation for visual effect
        float alpha = 0.5f * (1 + sin(RADIANS_PER_REVOLUTION * 0.25f * runtime));

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        auto renderer = Renderer();
        auto connector = Connector();

        const uint16_t width(connector->Width());
        const uint16_t height(connector->Height());
        const uint16_t renderWidth(TEXTURE_SIZE);
        const uint16_t renderHeight(TEXTURE_SIZE);

        // Initialize position to center if first frame
        if (!initialized) {
            posX = (width - renderWidth) / 2.0f;
            posY = (height - renderHeight) / 2.0f;
            initialized = true;
        }

        // Calculate frame time for physics
        static auto lastFrameTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;

        // Apply friction (slow down over time)
        float currentSpeed = sqrt(velocityX * velocityX + velocityY * velocityY);
        if (currentSpeed > MIN_SPEED) {
            float frictionFactor = pow(FRICTION, deltaTime);
            velocityX *= frictionFactor;
            velocityY *= frictionFactor;
        }

        // Update position based on velocity
        posX += velocityX * deltaTime;
        posY += velocityY * deltaTime;

        // Bounce off edges with speed boost
        if (posX <= 0) {
            posX = 0;
            velocityX = -velocityX * BOUNCE_BOOST;
            if (abs(velocityX) > MAX_SPEED) {
                velocityX = (velocityX > 0 ? MAX_SPEED : -MAX_SPEED);
            }
        } else if (posX >= (width - renderWidth)) {
            posX = width - renderWidth;
            velocityX = -velocityX * BOUNCE_BOOST;
            if (abs(velocityX) > MAX_SPEED) {
                velocityX = (velocityX > 0 ? MAX_SPEED : -MAX_SPEED);
            }
        }

        if (posY <= 0) {
            posY = 0;
            velocityY = -velocityY * BOUNCE_BOOST;
            if (abs(velocityY) > MAX_SPEED) {
                velocityY = (velocityY > 0 ? MAX_SPEED : -MAX_SPEED);
            }
        } else if (posY >= (height - renderHeight)) {
            posY = height - renderHeight;
            velocityY = -velocityY * BOUNCE_BOOST;
            if (abs(velocityY) > MAX_SPEED) {
                velocityY = (velocityY > 0 ? MAX_SPEED : -MAX_SPEED);
            }
        }

        Core::ProxyType<Compositor::IRenderer::IFrameBuffer> frameBuffer = connector->FrameBuffer();

        if (frameBuffer.IsValid() == true) {
            renderer->Bind(frameBuffer);
            renderer->Begin(width, height);
            renderer->Clear(background);

            const Exchange::IComposition::Rectangle renderBox = {
                static_cast<int32_t>(posX),
                static_cast<int32_t>(posY),
                renderWidth,
                renderHeight
            };

            Compositor::Matrix matrix;
            Compositor::Transformation::ProjectBox(matrix, renderBox, Compositor::Transformation::TRANSFORM_FLIPPED_180, rotation, renderer->Projection());

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
        } else {
            TRACE(Trace::Error, ("No valid framebuffer to render to"));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        return std::chrono::microseconds(0);
    }

private:
    mutable Core::CriticalSection _adminLock;
    Core::ProxyType<Compositor::DmaBuffer> _textureBuffer;
    Core::ProxyType<Compositor::IRenderer::ITexture> _texture;
    std::chrono::time_point<std::chrono::high_resolution_clock> _renderStart;
};


}

int main(int argc, char* argv[])
{
    bool quitApp(false);

    BaseTest::ConsoleOptions options(argc, argv);

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
            { "CompositorBackend", { "Error" } },
            { "CompositorRenderer", { "Error", "Warning", "Information" } },
            { "DRMCommon", { "Error", "Warning", "Information" } }
        };

        for (const auto& module_entry : modules) {
            for (const auto& category : module_entry.second) {
                tracer.EnableMessage(module_entry.first, category, true);
            }
        }

        TRACE_GLOBAL(Trace::Information, ("%s - build: %s", executableName, __TIMESTAMP__));

        RenderTest test(options.Output, options.RenderNode, options.FPS, options.Width, options.Height);

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