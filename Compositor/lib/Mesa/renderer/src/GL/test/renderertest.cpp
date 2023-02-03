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

#include <IAllocator.h>
#include <IRenderer.h>
#include <Transformation.h>

#include <drm_fourcc.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace WPEFramework;

#define fourcc_mod_get_vendor(modifier) \
    (((modifier) >> 56) & 0xff)

#define fourcc_mod_get_code(modifier) \
    ((modifier)&0x00ffffffffffffffULL)

namespace {
const std::vector<std::string> Parse(const std::string& input)
{
    std::istringstream iss(input);
    return std::vector<std::string> { std::istream_iterator<std::string> { iss }, std::istream_iterator<std::string> {} };
}

#define CASE_STR(value) \
    case value:         \
        return #value;

static const char* DrmVendorString(uint64_t modifier)
{
    switch (fourcc_mod_get_vendor(modifier)) {
        CASE_STR(DRM_FORMAT_MOD_VENDOR_NONE)
        CASE_STR(DRM_FORMAT_MOD_VENDOR_INTEL)
        CASE_STR(DRM_FORMAT_MOD_VENDOR_AMD)
        CASE_STR(DRM_FORMAT_MOD_VENDOR_NVIDIA)
        CASE_STR(DRM_FORMAT_MOD_VENDOR_SAMSUNG)
        CASE_STR(DRM_FORMAT_MOD_VENDOR_QCOM)
        CASE_STR(DRM_FORMAT_MOD_VENDOR_VIVANTE)
        CASE_STR(DRM_FORMAT_MOD_VENDOR_BROADCOM)
        CASE_STR(DRM_FORMAT_MOD_VENDOR_ARM)
        CASE_STR(DRM_FORMAT_MOD_VENDOR_ALLWINNER)
        CASE_STR(DRM_FORMAT_MOD_VENDOR_AMLOGIC)
    default:
        return "Unknown";
    }
}
#undef CASE_STR

void PrintFormat(const string& preamble, const Compositor::PixelFormat& format)
{
    std::stringstream line;

    line << preamble << " fourcc \'"
         << char(format.Type() & 0xff) << char((format.Type() >> 8) & 0xff)
         << char((format.Type() >> 16) & 0xff) << char((format.Type() >> 24) & 0xff)
         << "\'"
         << " modifiers: [ " << std::endl;

    for (auto& modifier : format.Modifiers()) {
        line << "  " << DrmVendorString(modifier) << ": code 0x" << std::hex << std::setw(7) << std::setfill('0') << fourcc_mod_get_code(modifier) << std::endl;
    }

    line << "]";

    TRACE_GLOBAL(Trace::Information, ("%s", line.str().c_str()));
}
#define GBM_MAX_PLANES 4
void Swap(int gbmDevFD, WPEFramework::Core::ProxyType<Compositor::Interfaces::IBuffer>& buffer)
{
    ASSERT(gbmDevFD > 0);

    // TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG gbm_surface_lock_front_buffer"), __FILE__, __LINE__, __FUNCTION__));
    // Lock the new buffer so we can use it
    gbm_bo* fbo = gbm_surface_lock_front_buffer(buffer._surface);

    if (fbo != nullptr) {
        // TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG gbm_device_get_fd"), __FILE__, __LINE__, __FUNCTION__));
        int fb_fd = gbm_device_get_fd(_device);

        ASSERT(gbmDevFD == fb_fd);

        uint32_t width = gbm_bo_get_width(bo);
        uint32_t height = gbm_bo_get_height(bo);
        uint32_t format = gbm_bo_get_format(bo);
        uint32_t handle = gbm_bo_get_handle(bo).u32;
        uint32_t stride = gbm_bo_get_stride(bo);
        uint64_t modifier = gbm_bo_get_modifier(bo);

        const uint32_t handles[GBM_MAX_PLANES] = { handle, 0, 0, 0 };
        const uint32_t pitches[GBM_MAX_PLANES] = { stride, 0, 0, 0 };
        const uint32_t offsets[GBM_MAX_PLANES] = { 0, 0, 0, 0 };
        const uint64_t modifiers[GBM_MAX_PLANES] = { modifier, 0, 0, 0 };

        if (drmModeAddFB2WithModifiers(gbmDevFD, width, height, format, &handles[0], &pitches[0], &offsets[0], &modifiers[0], &(buffer._id), 0 /* flags */) != 0) {
            buffer._id = DRM::InvalidFb();

            TRACE(WPEFramework::Trace::Error, (_T ( "Unable to construct a frame buffer for scan out." )));
        } else {
            // TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG drmModeAddFB2WithModifiers"), __FILE__, __LINE__, __FUNCTION__));

            static data_t cdata(true);
            cdata = true;

            int err = drmModePageFlip(gbmDevFD, _crtc, buffer._id, DRM_MODE_PAGE_FLIP_EVENT, &cdata);
            // Many causes, but the most obvious is a busy resource or a missing drmModeSetCrtc
            // Probably a missing drmModeSetCrtc or an invalid _crtc
            // See ModeSet::Create, not recovering here
            ASSERT(err != -EINVAL);
            ASSERT(err != -EBUSY);

            if (err == 0) {
                // No error
                // Asynchronous, but never called more than once, waiting in scope
                // Use the magic constant here because the struct is versioned!

                drmEventContext context = {
                    .version = 2,
                    .vblank_handler = nullptr,
                    .page_flip_handler = PageFlip
#if DRM_EVENT_CONTEXT_VERSION > 2
                    ,
                    .page_flip_handler2 = nullptr
#endif
#if DRM_EVENT_CONTEXT_VERSION > 3
                    ,
                    .sequence_handler = nullptr
#endif
                };

                struct timespec timeout = { .tv_sec = 1, .tv_nsec = 0 };
                fd_set fds;

                // Going fast could trigger an (unrecoverable) EBUSY
                bool waiting = cdata;

                while (waiting != false) {
                    FD_ZERO(&fds);
                    FD_SET(gbmDevFD, &fds);

                    // Race free
                    if ((err = pselect(gbmDevFD + 1, &fds, nullptr, nullptr, &timeout, nullptr)) < 0) {
                        // Error; break the loop
                        TRACE(Trace::Error, (_T ( "Event processing for page flip failed." )));
                        break;
                    } else {
                        if (err == 0) {
                            // Timeout; retry
                            // TODO: add an additional condition to break the loop to limit the
                            // number of retries, but then deal with the asynchronous nature of
                            // the callback

                            TRACE(Trace::Information, (_T ( "Unable to execute a timely page flip. Trying again." )));
                        } else {
                            if (FD_ISSET(gbmDevFD, &fds) != 0) {
                                // Node is readable
                                if (drmHandleEvent(gbmDevFD, &context) != 0) {
                                    // Error; break the loop
                                    break;
                                }
                                // Flip probably occurred already otherwise it loops again
                            }
                        }

                        waiting = cdata;

                        if (waiting != true) {
                            // Do not prematurely remove the FB to prevent an EBUSY
                            static BufferInfo binfo(GBM::InvalidSurf(), nullptr, DRM::InvalidFb());
                            std::swap(binfo, buffer);
                        }
                    }
                }
            } else {
                TRACE(WPEFramework::Trace::Error, (_T ( "Unable to execute a page flip." )));
            }

            if (gbmDevFD != DRM::InvalidFd()
                && buffer._id != DRM::InvalidFb()
                && drmModeRmFB(gbmDevFD, buffer._id) != 0) {
                TRACE(WPEFramework::Trace::Error, (_T ( "Unable to remove (old) frame buffer." )));
            }
        }

        if (buffer._surface != GBM::InvalidSurf()
            && fbo != nullptr) {
            // TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG gbm_surface_release_buffer"), __FILE__, __LINE__, __FUNCTION__));
            gbm_surface_release_buffer(buffer._surface, fbo);
            fbo = nullptr;
        } else {
            TRACE(WPEFramework::Trace::Error, (_T ( "Unable to release (old) buffer." )));
        }
    } else {
        TRACE(WPEFramework::Trace::Error, (_T ( "Unable to obtain a buffer to support scan out." )));
    }
}
}

const Compositor::Color gray = { 0.2, 0.2, 0.2, 1.0 };
const Compositor::Color red = { 1.0, 0.0, 0.0, 1.0 };
}

int main(int /*argc*/, const char* argv[])
{
    Messaging::MessageUnit::flush flushMode;
    flushMode = Messaging::MessageUnit::flush::FLUSH;

    Messaging::LocalTracer& tracer = Messaging::LocalTracer::Open();

    const char* executableName(Core::FileNameOnly(argv[0]));

    {
        Messaging::ConsolePrinter printer(true);

        tracer.Callback(&printer);

        const std::vector<string> modules = {
            "Error",
            "Information",
            "EGL",
            "GL",
            "Buffer"
        };

        for (auto module : modules) {
            tracer.EnableMessage("CompositorRenderTest", module, true);
            tracer.EnableMessage("CompositorRendererEGL", module, true);
            tracer.EnableMessage("CompositorRendererGLES2", module, true);
            tracer.EnableMessage("CompositorBuffer", module, true);
        }

        TRACE_GLOBAL(Trace::Information, ("%s - build: %s", executableName, __TIMESTAMP__));

        int drmFd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC);

        Compositor::PixelFormat format(DRM_FORMAT_XRGB8888);

        WPEFramework::Core::ProxyType<Compositor::Interfaces::IAllocator> allocator = Compositor::Interfaces::IAllocator::Instance(drmFd);

        WPEFramework::Core::ProxyType<Compositor::Interfaces::IRenderer> renderer = Compositor::Interfaces::IRenderer::Instance(drmFd);

        // const std::vector<Compositor::PixelFormat>& renderFormats(renderer->RenderFormats());
        // for (const auto& format : renderFormats) {
        //     PrintFormat("Render", format);
        // }

        // const std::vector<Compositor::PixelFormat>& textureFormats(renderer->TextureFormats());
        // for (const auto& format : textureFormats) {
        //     PrintFormat("Texture", format);
        // }

        WPEFramework::Core::ProxyType<Compositor::Interfaces::IBuffer> frameBuffer = allocator->Create(1920, 1080, format);

        // Add a buffer to render on
        renderer->Bind(frameBuffer);

        // wlr_output_attach_render(wlr_output, NULL);

        renderer->Begin(1920, 1080);

        renderer->Clear(gray);

        // TODO make one object of this...
        Compositor::Matrix quad1;
        Compositor::Transformation::Projection(quad1, 60, 120, Compositor::Transformation::TRANSFORM_NORMAL);
        Compositor::Transformation::Translate(quad1, 20, 20);

        renderer->Quadrangle(red, quad1);

        renderer->End();

        // wlr_output_commit(wlr_output);
        // crtc_commit

        renderer->Unbind();

        frameBuffer.Release();
        renderer.Release();
        allocator.Release();

        close(drmFd);
    }
    
    TRACE_GLOBAL(Trace::Information, ("Exiting %s.... ", executableName));

    tracer.Close();
    Core::Singleton::Dispose();

    return 0;
}
