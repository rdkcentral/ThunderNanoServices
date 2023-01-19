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

#include <IRenderer.h>

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
            "GL"
        };

        for (auto module : modules) {
            tracer.EnableMessage("CompositorRenderTest", module, true);
            tracer.EnableMessage("CompositorRendererEGL", module, true);
            tracer.EnableMessage("CompositorRendererGLES2", module, true);
        }

        TRACE_GLOBAL(Trace::Information, ("%s - build: %s", executableName, __TIMESTAMP__));

        int drmFd = open("/dev/dri/renderD129", O_RDWR | O_CLOEXEC);

        WPEFramework::Core::ProxyType<Compositor::Interfaces::IRenderer> renderer = Compositor::Interfaces::IRenderer::Instance(drmFd);

        const std::vector<Compositor::PixelFormat>& renderFormats(renderer->RenderFormats());
        for (const auto& format : renderFormats) {
            PrintFormat("Render", format);
        }

        const std::vector<Compositor::PixelFormat>& textureFormats(renderer->TextureFormats());
        for (const auto& format : textureFormats) {
            PrintFormat("Texture", format);
        }

        

        close(drmFd);

        TRACE_GLOBAL(Trace::Information, ("Exiting %s.... ", executableName));
    }

    tracer.Close();
    Core::Singleton::Dispose();

    return 0;
}
