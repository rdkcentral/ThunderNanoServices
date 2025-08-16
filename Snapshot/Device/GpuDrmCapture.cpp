/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
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

#include "../Module.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <drm/drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <interfaces/ICapture.h>
#include <chrono>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>

namespace Thunder {
namespace Plugin {

    /**
     * Framebuffer format information including multi-plane support
     */
    struct FramebufferInfo {
        uint32_t format;
        uint32_t width;
        uint32_t height;
        uint32_t planeCount;
        struct {
            int fd;
            uint32_t offset;
            uint32_t pitch;
            uint64_t modifier;
        } planes[4]; // DRM supports up to 4 planes
        
        FramebufferInfo() : format(0), width(0), height(0), planeCount(0) {
            for (int i = 0; i < 4; ++i) {
                planes[i] = {-1, 0, 0, 0};
            }
        }
        
        ~FramebufferInfo() {
            // Cleanup any open file descriptors
            for (uint32_t i = 0; i < 4; ++i) {
                if (planes[i].fd >= 0) {
                    close(planes[i].fd);
                    planes[i].fd = -1;
                }
            }
        }
        
        // Disable copy to prevent fd double-close
        FramebufferInfo(const FramebufferInfo&) = delete;
        FramebufferInfo& operator=(const FramebufferInfo&) = delete;
        
        // Enable move semantics
        FramebufferInfo(FramebufferInfo&& other) noexcept 
            : format(other.format), width(other.width), height(other.height), planeCount(other.planeCount) {
            for (int i = 0; i < 4; ++i) {
                planes[i] = other.planes[i];
                other.planes[i].fd = -1; // Transfer ownership
            }
        }
        
        FramebufferInfo& operator=(FramebufferInfo&& other) noexcept {
            if (this != &other) {
                // Cleanup existing fds
                for (int i = 0; i < 4; ++i) {
                    if (planes[i].fd >= 0) {
                        close(planes[i].fd);
                    }
                }
                
                // Transfer from other
                format = other.format;
                width = other.width;
                height = other.height;
                planeCount = other.planeCount;
                for (int i = 0; i < 4; ++i) {
                    planes[i] = other.planes[i];
                    other.planes[i].fd = -1;
                }
            }
            return *this;
        }
    };

    /**
     * Format classification for handling different types
     */
    enum class FormatType {
        UNSUPPORTED,
        SINGLE_PLANE_RGB,
        SINGLE_PLANE_YUV,
        MULTI_PLANE_YUV
    };

    class GpuDrmCapture : public Exchange::ICapture {
    private:
        GpuDrmCapture(const GpuDrmCapture&) = delete;
        GpuDrmCapture& operator=(const GpuDrmCapture&) = delete;

        /**
         * Represents a single monitor/display output with GPU texture binding
         */
        struct MonitorInfo {
            uint32_t connectorId;
            uint32_t crtcId;
            uint32_t width;
            uint32_t height;
            int32_t x;
            int32_t y;
            bool active;
            uint32_t framebufferId;
            int drmFd;
            std::string devicePath;
            uint32_t pixelFormat; // Detected framebuffer format

            // GPU resources for this monitor
            GLuint texture;
            GLuint framebuffer;
            EGLImage eglImage;
            bool gpuResourcesReady;
            FormatType formatType; // Cached format classification

            // VSync support
            uint32_t crtcIndex; // Index for vblank events
            bool vsyncEnabled;

            MonitorInfo()
                : connectorId(0)
                , crtcId(0)
                , width(0)
                , height(0)
                , x(0)
                , y(0)
                , active(false)
                , framebufferId(0)
                , drmFd(-1)
                , pixelFormat(0)
                , texture(0)
                , framebuffer(0)
                , eglImage(EGL_NO_IMAGE_KHR)
                , gpuResourcesReady(false)
                , formatType(FormatType::UNSUPPORTED)
                , crtcIndex(0)
                , vsyncEnabled(false)
            {
            }
        };

        /**
         * GPU context for a single DRM device with OpenGL ES 2.0 resources
         */
        struct GpuContext {
            int drmFd;
            std::string devicePath;
            struct gbm_device* gbmDevice;
            EGLDisplay eglDisplay;
            EGLContext eglContext;
            EGLConfig eglConfig;
            bool initialized;
            std::vector<MonitorInfo> monitors;

            // EGL extension function pointers
            PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
            PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
            PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;

            // OpenGL ES 2.0 resources for blitting and composition
            GLuint blitVertexShader;
            GLuint blitFragmentShader;
            GLuint blitProgram;
            GLuint blitVertexBuffer;
            GLuint blitPositionAttrib;
            GLuint blitTexCoordAttrib;
            GLuint blitTextureUniform;

            // YUV conversion shader resources
            GLuint yuvVertexShader;
            GLuint yuvFragmentShader;
            GLuint yuvProgram;
            GLuint yuvPositionAttrib;
            GLuint yuvTexCoordAttrib;
            GLuint yuvTextureUniform;

            // Final composition resources
            GLuint compositionTexture;
            GLuint compositionFramebuffer;
            uint32_t compositionWidth;
            uint32_t compositionHeight;

            // Platform capabilities
            bool hasModifierSupport;

            GpuContext(int fd, const std::string& path)
                : drmFd(fd)
                , devicePath(path)
                , gbmDevice(nullptr)
                , eglDisplay(EGL_NO_DISPLAY)
                , eglContext(EGL_NO_CONTEXT)
                , eglConfig(nullptr)
                , initialized(false)
                , eglCreateImageKHR(nullptr)
                , eglDestroyImageKHR(nullptr)
                , glEGLImageTargetTexture2DOES(nullptr)
                , blitVertexShader(0)
                , blitFragmentShader(0)
                , blitProgram(0)
                , blitVertexBuffer(0)
                , blitPositionAttrib(0)
                , blitTexCoordAttrib(0)
                , blitTextureUniform(0)
                , yuvVertexShader(0)
                , yuvFragmentShader(0)
                , yuvProgram(0)
                , yuvPositionAttrib(0)
                , yuvTexCoordAttrib(0)
                , yuvTextureUniform(0)
                , compositionTexture(0)
                , compositionFramebuffer(0)
                , compositionWidth(0)
                , compositionHeight(0)
                , hasModifierSupport(false)
            {
            }

            ~GpuContext()
            {
                Cleanup();
            }

            void Cleanup()
            {
                if (initialized && eglDisplay != EGL_NO_DISPLAY) {
                    eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, eglContext);

                    // Cleanup monitor GPU resources
                    for (auto& monitor : monitors) {
                        CleanupMonitorGpuResources(monitor);
                    }

                    // Cleanup composition resources
                    if (compositionFramebuffer)
                        glDeleteFramebuffers(1, &compositionFramebuffer);
                    if (compositionTexture)
                        glDeleteTextures(1, &compositionTexture);

                    // Cleanup blit resources
                    if (blitVertexBuffer)
                        glDeleteBuffers(1, &blitVertexBuffer);
                    if (blitProgram)
                        glDeleteProgram(blitProgram);
                    if (blitVertexShader)
                        glDeleteShader(blitVertexShader);
                    if (blitFragmentShader)
                        glDeleteShader(blitFragmentShader);

                    // Cleanup YUV resources
                    if (yuvProgram)
                        glDeleteProgram(yuvProgram);
                    if (yuvVertexShader)
                        glDeleteShader(yuvVertexShader);
                    if (yuvFragmentShader)
                        glDeleteShader(yuvFragmentShader);

                    if (eglContext != EGL_NO_CONTEXT) {
                        eglDestroyContext(eglDisplay, eglContext);
                    }
                    eglTerminate(eglDisplay);
                }

                if (gbmDevice) {
                    gbm_device_destroy(gbmDevice);
                    gbmDevice = nullptr;
                }

                if (drmFd >= 0) {
                    close(drmFd);
                    drmFd = -1;
                }

                initialized = false;
            }

            void CleanupMonitorGpuResources(MonitorInfo& monitor)
            {
                if (monitor.framebuffer) {
                    glDeleteFramebuffers(1, &monitor.framebuffer);
                    monitor.framebuffer = 0;
                }
                if (monitor.texture) {
                    glDeleteTextures(1, &monitor.texture);
                    monitor.texture = 0;
                }
                if (monitor.eglImage != EGL_NO_IMAGE_KHR && eglDestroyImageKHR) {
                    eglDestroyImageKHR(eglDisplay, monitor.eglImage);
                    monitor.eglImage = EGL_NO_IMAGE_KHR;
                }
                monitor.gpuResourcesReady = false;

                // Force GPU completion after cleanup
                glFlush();
            }
        };

        /**
         * Composition bounds for the final output
         */
        struct CompositionBounds {
            uint32_t totalWidth;
            uint32_t totalHeight;
            int32_t minX;
            int32_t minY;

            CompositionBounds()
                : totalWidth(0)
                , totalHeight(0)
                , minX(0)
                , minY(0)
            {
            }
        };

        /**
         * Format classification helper
         */
        static FormatType ClassifyFormat(uint32_t format)
        {
            switch (format) {
                // Single-plane RGB formats (easiest to handle)
                case DRM_FORMAT_XRGB8888:
                case DRM_FORMAT_ARGB8888:
                case DRM_FORMAT_XBGR8888:
                case DRM_FORMAT_ABGR8888:
                case DRM_FORMAT_RGBX8888:
                case DRM_FORMAT_RGBA8888:
                case DRM_FORMAT_BGRX8888:
                case DRM_FORMAT_BGRA8888:
                case DRM_FORMAT_RGB565:
                case DRM_FORMAT_BGR565:
                    return FormatType::SINGLE_PLANE_RGB;
                    
                // Single-plane YUV formats
                case DRM_FORMAT_YUYV:
                case DRM_FORMAT_YVYU:
                case DRM_FORMAT_UYVY:
                case DRM_FORMAT_VYUY:
                    return FormatType::SINGLE_PLANE_YUV;
                    
                // Multi-plane YUV formats (common on embedded)
                case DRM_FORMAT_YUV420:
                case DRM_FORMAT_YVU420:
                case DRM_FORMAT_YUV422:
                case DRM_FORMAT_YVU422:
                case DRM_FORMAT_YUV444:
                case DRM_FORMAT_YVU444:
                case DRM_FORMAT_NV12:
                case DRM_FORMAT_NV21:
                case DRM_FORMAT_NV16:
                case DRM_FORMAT_NV61:
                    return FormatType::MULTI_PLANE_YUV;
                    
                default:
                    TRACE_GLOBAL(Trace::Warning, (_T("Unknown format: 0x%x"), format));
                    return FormatType::UNSUPPORTED;
            }
        }

        /**
         * Format to string conversion for logging
         */
        static const char* FormatToString(uint32_t format)
        {
            switch (format) {
                case DRM_FORMAT_XRGB8888: return "XRGB8888";
                case DRM_FORMAT_ARGB8888: return "ARGB8888";
                case DRM_FORMAT_XBGR8888: return "XBGR8888";
                case DRM_FORMAT_ABGR8888: return "ABGR8888";
                case DRM_FORMAT_RGBX8888: return "RGBX8888";
                case DRM_FORMAT_RGBA8888: return "RGBA8888";
                case DRM_FORMAT_BGRX8888: return "BGRX8888";
                case DRM_FORMAT_BGRA8888: return "BGRA8888";
                case DRM_FORMAT_RGB565: return "RGB565";
                case DRM_FORMAT_BGR565: return "BGR565";
                case DRM_FORMAT_YUYV: return "YUYV";
                case DRM_FORMAT_YVYU: return "YVYU";
                case DRM_FORMAT_UYVY: return "UYVY";
                case DRM_FORMAT_VYUY: return "VYUY";
                case DRM_FORMAT_NV12: return "NV12";
                case DRM_FORMAT_NV21: return "NV21";
                case DRM_FORMAT_NV16: return "NV16";
                case DRM_FORMAT_NV61: return "NV61";
                case DRM_FORMAT_YUV420: return "YUV420";
                case DRM_FORMAT_YVU420: return "YVU420";
                case DRM_FORMAT_YUV422: return "YUV422";
                case DRM_FORMAT_YVU422: return "YVU422";
                case DRM_FORMAT_YUV444: return "YUV444";
                case DRM_FORMAT_YVU444: return "YVU444";
                default: return "Unknown";
            }
        }

        /**
         * Enhanced framebuffer information retrieval with multi-plane support
         */
        static bool GetFramebufferInfo(int drmFd, uint32_t fbId, FramebufferInfo& fbInfo)
        {
            // First try FB2 (modern API with multi-plane support)
            drmModeFB2* fb2 = drmModeGetFB2(drmFd, fbId);
            if (fb2) {
                fbInfo.format = fb2->pixel_format;
                fbInfo.width = fb2->width;
                fbInfo.height = fb2->height;
                
                // Get plane information
                uint32_t planeCount = 0;
                for (uint32_t i = 0; i < 4; ++i) {
                    if (fb2->handles[i] != 0) {
                        planeCount++;
                        
                        // Export each plane as DMA-BUF
                        int planeFd = -1;
                        if (drmPrimeHandleToFD(drmFd, fb2->handles[i], DRM_CLOEXEC, &planeFd) == 0) {
                            fbInfo.planes[i].fd = planeFd;
                            fbInfo.planes[i].offset = fb2->offsets[i];
                            fbInfo.planes[i].pitch = fb2->pitches[i];
                            fbInfo.planes[i].modifier = fb2->modifier;
                        } else {
                            TRACE_GLOBAL(Trace::Warning, (_T("Failed to export plane %d as DMA-BUF: %s"), i, strerror(errno)));
                        }
                    }
                }
                fbInfo.planeCount = planeCount;
                
                drmModeFreeFB2(fb2);
                
                TRACE_GLOBAL(Trace::Information, (_T("FB2 format: 0x%x (%s), %d planes"), 
                    fbInfo.format, FormatToString(fbInfo.format), fbInfo.planeCount));
                return true;
            }
            
            // Fallback to FB1 (legacy single-plane)
            drmModeFB* fb = drmModeGetFB(drmFd, fbId);
            if (fb) {
                fbInfo.format = DRM_FORMAT_XRGB8888; // Assume XRGB for legacy
                fbInfo.width = fb->width;
                fbInfo.height = fb->height;
                fbInfo.planeCount = 1;
                
                // Export single plane
                int planeFd = -1;
                if (drmPrimeHandleToFD(drmFd, fb->handle, DRM_CLOEXEC, &planeFd) == 0) {
                    fbInfo.planes[0].fd = planeFd;
                    fbInfo.planes[0].offset = 0;
                    fbInfo.planes[0].pitch = fb->pitch;
                    fbInfo.planes[0].modifier = DRM_FORMAT_MOD_LINEAR; // Assume linear
                }
                
                drmModeFreeFB(fb);
                TRACE_GLOBAL(Trace::Information, (_T("FB1 fallback: assuming XRGB8888, 1 plane")));
                return true;
            }
            
            TRACE_GLOBAL(Trace::Error, (_T("Failed to get framebuffer info for FB %d"), fbId));
            return false;
        }

        /**
         * Get appropriate fragment shader for format
         */
        static const char* GetFragmentShaderForFormat(FormatType formatType)
        {
            switch (formatType) {
                case FormatType::SINGLE_PLANE_RGB:
                    return R"(
                        precision mediump float;
                        uniform sampler2D u_texture;
                        varying vec2 v_texCoord;
                        void main() {
                            gl_FragColor = texture2D(u_texture, v_texCoord);
                        }
                    )";
                    
                case FormatType::SINGLE_PLANE_YUV:
                case FormatType::MULTI_PLANE_YUV:
                    return R"(
                        precision mediump float;
                        uniform sampler2D u_texture;
                        varying vec2 v_texCoord;
                        
                        // YUV to RGB conversion matrix (BT.709)
                        const mat3 yuv2rgb = mat3(
                            1.0, 1.0, 1.0,
                            0.0, -0.187, 1.856,
                            1.575, -0.468, 0.0
                        );
                        
                        void main() {
                            vec3 yuv = texture2D(u_texture, v_texCoord).rgb;
                            yuv.r = yuv.r - 0.0625; // Y adjustment
                            yuv.gb = yuv.gb - 0.5;  // UV adjustment
                            vec3 rgb = yuv2rgb * yuv;
                            gl_FragColor = vec4(rgb, 1.0);
                        }
                    )";
                    
                default:
                    return nullptr;
            }
        }

        /**
         * Create EGL image from framebuffer with multi-plane support
         */
        bool CreateEGLImageFromFramebuffer(GpuContext& context, const FramebufferInfo& fbInfo, 
                                           GLuint& texture, EGLImage& eglImage)
        {
            FormatType formatType = ClassifyFormat(fbInfo.format);
            
            switch (formatType) {
                case FormatType::SINGLE_PLANE_RGB:
                    return CreateSinglePlaneRGBImage(context, fbInfo, texture, eglImage);
                    
                case FormatType::SINGLE_PLANE_YUV:
                    return CreateSinglePlaneYUVImage(context, fbInfo, texture, eglImage);
                    
                case FormatType::MULTI_PLANE_YUV:
                    return CreateMultiPlaneYUVImage(context, fbInfo, texture, eglImage);
                    
                default:
                    TRACE_GLOBAL(Trace::Error, (_T("Unsupported format: 0x%x (%s)"), fbInfo.format, FormatToString(fbInfo.format)));
                    return false;
            }
        }

        bool CreateSinglePlaneRGBImage(GpuContext& context, const FramebufferInfo& fbInfo,
                                        GLuint& texture, EGLImage& eglImage)
        {
            if (fbInfo.planeCount != 1 || fbInfo.planes[0].fd < 0) {
                TRACE_GLOBAL(Trace::Error, (_T("Invalid single-plane RGB framebuffer: %d planes, fd=%d"), fbInfo.planeCount, fbInfo.planes[0].fd));
                return false;
            }
            
            std::vector<EGLint> attribs = {
                EGL_WIDTH, static_cast<EGLint>(fbInfo.width),
                EGL_HEIGHT, static_cast<EGLint>(fbInfo.height),
                EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLint>(fbInfo.format),
                EGL_DMA_BUF_PLANE0_FD_EXT, fbInfo.planes[0].fd,
                EGL_DMA_BUF_PLANE0_OFFSET_EXT, static_cast<EGLint>(fbInfo.planes[0].offset),
                EGL_DMA_BUF_PLANE0_PITCH_EXT, static_cast<EGLint>(fbInfo.planes[0].pitch)
            };
            
            // Add modifier if supported and valid
            if (context.hasModifierSupport && fbInfo.planes[0].modifier != DRM_FORMAT_MOD_INVALID) {
                attribs.insert(attribs.end(), {
                    EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, static_cast<EGLint>(fbInfo.planes[0].modifier & 0xFFFFFFFF),
                    EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, static_cast<EGLint>(fbInfo.planes[0].modifier >> 32)
                });
            }
            
            attribs.push_back(EGL_NONE);
            
            eglImage = context.eglCreateImageKHR(context.eglDisplay, EGL_NO_CONTEXT, 
                                                 EGL_LINUX_DMA_BUF_EXT, nullptr, attribs.data());
            if (eglImage == EGL_NO_IMAGE_KHR) {
                EGLint eglError = eglGetError();
                TRACE_GLOBAL(Trace::Error, (_T("Failed to create EGL image for single-plane RGB. EGL error: 0x%x"), eglError));
                return false;
            }
            
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            context.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, eglImage);
            
            // Set texture parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            
            return true;
        }

        bool CreateSinglePlaneYUVImage(GpuContext& context, const FramebufferInfo& fbInfo,
                                        GLuint& texture, EGLImage& eglImage)
        {
            // Similar to RGB but may need different handling
            return CreateSinglePlaneRGBImage(context, fbInfo, texture, eglImage);
        }

        bool CreateMultiPlaneYUVImage(GpuContext& context, const FramebufferInfo& fbInfo,
                                      GLuint& texture, EGLImage& eglImage)
        {
            // Multi-plane YUV requires more complex handling
            if (fbInfo.planeCount < 2) {
                TRACE_GLOBAL(Trace::Error, (_T("Multi-plane format but only %d planes"), fbInfo.planeCount));
                return false;
            }
            
            // Check if all required planes are available
            for (uint32_t i = 0; i < fbInfo.planeCount; ++i) {
                if (fbInfo.planes[i].fd < 0) {
                    TRACE_GLOBAL(Trace::Error, (_T("Missing plane %d for multi-plane format"), i));
                    return false;
                }
            }
            
            std::vector<EGLint> attribs = {
                EGL_WIDTH, static_cast<EGLint>(fbInfo.width),
                EGL_HEIGHT, static_cast<EGLint>(fbInfo.height),
                EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLint>(fbInfo.format),
            };
            
            // Add plane-specific attributes
            for (uint32_t i = 0; i < fbInfo.planeCount && i < 4; ++i) {
                attribs.insert(attribs.end(), {
                    static_cast<EGLint>(EGL_DMA_BUF_PLANE0_FD_EXT + i * 3), fbInfo.planes[i].fd,
                    static_cast<EGLint>(EGL_DMA_BUF_PLANE0_OFFSET_EXT + i * 3), static_cast<EGLint>(fbInfo.planes[i].offset),
                    static_cast<EGLint>(EGL_DMA_BUF_PLANE0_PITCH_EXT + i * 3), static_cast<EGLint>(fbInfo.planes[i].pitch)
                });
            }
            
            // Add modifier if supported
            if (context.hasModifierSupport && fbInfo.planes[0].modifier != DRM_FORMAT_MOD_INVALID) {
                attribs.insert(attribs.end(), {
                    EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, static_cast<EGLint>(fbInfo.planes[0].modifier & 0xFFFFFFFF),
                    EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, static_cast<EGLint>(fbInfo.planes[0].modifier >> 32)
                });
            }
            
            attribs.push_back(EGL_NONE);
            
            eglImage = context.eglCreateImageKHR(context.eglDisplay, EGL_NO_CONTEXT,
                                                 EGL_LINUX_DMA_BUF_EXT, nullptr, attribs.data());
            if (eglImage == EGL_NO_IMAGE_KHR) {
                EGLint eglError = eglGetError();
                TRACE_GLOBAL(Trace::Error, (_T("Failed to create EGL image for multi-plane YUV. EGL error: 0x%x"), eglError));
                return false;
            }
            
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            context.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, eglImage);
            
            // Set texture parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            
            return true;
        }

        /**
         * Wait for VSync on a specific monitor for stable frame capture
         */
        bool WaitForVSync(const MonitorInfo& monitor)
        {
            if (!monitor.vsyncEnabled) {
                return true; // Skip VSync if not supported
            }

            auto vsyncStart = std::chrono::high_resolution_clock::now();

            // Setup VBlank request
            drmVBlank vblank = {};
            vblank.request.type = (drmVBlankSeqType)(DRM_VBLANK_RELATIVE | (monitor.crtcIndex << DRM_VBLANK_HIGH_CRTC_SHIFT));
            vblank.request.sequence = 1; // Wait for next VBlank

            // Wait for VBlank
            int ret = drmWaitVBlank(monitor.drmFd, &vblank);
            if (ret != 0) {
                TRACE_GLOBAL(Trace::Warning, (_T("VBlank wait failed for monitor %d: %s"), monitor.connectorId, strerror(errno)));
                return false;
            }

            auto vsyncDuration = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - vsyncStart);
            TRACE_GLOBAL(Trace::Information, (_T("VSync wait completed in %lld μs for monitor %d"), vsyncDuration.count(), monitor.connectorId));

            return true;
        }

        /**
         * Log platform capabilities for diagnostics
         */
        void LogPlatformCapabilities(GpuContext& context)
        {
            TRACE_GLOBAL(Trace::Information, (_T("=== Platform Diagnostics for %s ==="), context.devicePath.c_str()));
            
            // EGL capabilities
            const char* vendor = eglQueryString(context.eglDisplay, EGL_VENDOR);
            const char* version = eglQueryString(context.eglDisplay, EGL_VERSION);
            const char* clientApis = eglQueryString(context.eglDisplay, EGL_CLIENT_APIS);
            const char* eglExtensions = eglQueryString(context.eglDisplay, EGL_EXTENSIONS);
            
            TRACE_GLOBAL(Trace::Information, (_T("EGL Vendor: %s"), vendor ? vendor : "Unknown"));
            TRACE_GLOBAL(Trace::Information, (_T("EGL Version: %s"), version ? version : "Unknown"));
            TRACE_GLOBAL(Trace::Information, (_T("EGL Client APIs: %s"), clientApis ? clientApis : "Unknown"));
            
            // Check for modifier support
            context.hasModifierSupport = eglExtensions && strstr(eglExtensions, "EGL_EXT_image_dma_buf_import_modifiers");
            TRACE_GLOBAL(Trace::Information, (_T("EGL Modifier Support: %s"), context.hasModifierSupport ? "Yes" : "No"));
            
            // OpenGL ES capabilities
            const char* glVersion = (const char*)glGetString(GL_VERSION);
            const char* glVendor = (const char*)glGetString(GL_VENDOR);
            const char* glRenderer = (const char*)glGetString(GL_RENDERER);
            const char* glExtensions = (const char*)glGetString(GL_EXTENSIONS);
            
            TRACE_GLOBAL(Trace::Information, (_T("OpenGL ES Version: %s"), glVersion ? glVersion : "Unknown"));
            TRACE_GLOBAL(Trace::Information, (_T("OpenGL ES Vendor: %s"), glVendor ? glVendor : "Unknown"));
            TRACE_GLOBAL(Trace::Information, (_T("OpenGL ES Renderer: %s"), glRenderer ? glRenderer : "Unknown"));
            
            // Check specific extensions for embedded platforms
            if (glExtensions) {
                bool hasOESTexture = strstr(glExtensions, "GL_OES_texture_npot") != nullptr;
                bool hasOESEGLImage = strstr(glExtensions, "GL_OES_EGL_image") != nullptr;
                bool hasOESEGLImageExternal = strstr(glExtensions, "GL_OES_EGL_image_external") != nullptr;
                
                TRACE_GLOBAL(Trace::Information, (_T("GL_OES_texture_npot: %s"), hasOESTexture ? "Yes" : "No"));
                TRACE_GLOBAL(Trace::Information, (_T("GL_OES_EGL_image: %s"), hasOESEGLImage ? "Yes" : "No"));
                TRACE_GLOBAL(Trace::Information, (_T("GL_OES_EGL_image_external: %s"), hasOESEGLImageExternal ? "Yes" : "No"));
            }
            
            // Get texture limits
            GLint maxTextureSize, maxViewportDims[2];
            glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
            glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxViewportDims);
            
            TRACE_GLOBAL(Trace::Information, (_T("Max texture size: %d"), maxTextureSize));
            TRACE_GLOBAL(Trace::Information, (_T("Max viewport: %dx%d"), maxViewportDims[0], maxViewportDims[1]));
            
            TRACE_GLOBAL(Trace::Information, (_T("=== End Platform Diagnostics ===")));
        }

    public:
        GpuDrmCapture()
        {
            TRACE_GLOBAL(Trace::Information, (_T("GpuDrmCapture: Initializing hardware-accelerated capture - Build: %s"), __TIMESTAMP__));

            if (!Initialize()) {
                TRACE_GLOBAL(Trace::Error, (_T("GpuDrmCapture: Failed to initialize")));
            }
        }

        ~GpuDrmCapture() override
        {
            Deinitialize();
        }

        BEGIN_INTERFACE_MAP(GpuDrmCapture)
        INTERFACE_ENTRY(Exchange::ICapture)
        END_INTERFACE_MAP

        const TCHAR* Name() const override
        {
            return (_T("GpuDrm v24.1"));
        }

        /**
         * Main capture method - performs GPU-accelerated screen capture with partial capture support
         * Target: <10ms per monitor for GPU framebuffer snap operation
         */
        bool Capture(ICapture::IStore& storer) override
        {
            auto captureStart = std::chrono::high_resolution_clock::now();

            if (_gpuContexts.empty()) {
                TRACE_GLOBAL(Trace::Error, (_T("No GPU contexts available for capture")));
                return false;
            }

            // Refresh display information to handle hotplug events
            if (!RefreshAllDisplays()) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to refresh displays for capture")));
                return false;
            }

            // Collect all active monitors across all GPU contexts
            std::vector<MonitorInfo*> allMonitors;
            for (auto& context : _gpuContexts) {
                for (auto& monitor : context->monitors) {
                    if (monitor.active) {
                        allMonitors.push_back(&monitor);
                    }
                }
            }

            if (allMonitors.empty()) {
                TRACE_GLOBAL(Trace::Error, (_T("No active monitors found for capture")));
                return false;
            }

            // Calculate composition bounds
            CompositionBounds bounds = CalculateCompositionBounds(allMonitors);
            TRACE_GLOBAL(Trace::Information, (_T("Composition bounds: %dx%d with offset (%d, %d)"), bounds.totalWidth, bounds.totalHeight, bounds.minX, bounds.minY));

            // Setup composition GPU buffer
            if (!SetupCompositionBuffer(bounds)) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to setup composition buffer")));
                return false;
            }

            // Capture each monitor using GPU acceleration with partial capture support
            std::vector<MonitorInfo*> successfulMonitors;
            std::vector<MonitorInfo*> failedMonitors;
            
            for (auto* monitor : allMonitors) {
                auto monitorStart = std::chrono::high_resolution_clock::now();

                // Wait for VSync before capture for frame stability
                if (!WaitForVSync(*monitor)) {
                    TRACE_GLOBAL(Trace::Warning, (_T("VSync wait failed for monitor %d, capturing anyway"), monitor->connectorId));
                }

                if (CaptureMonitorToGpu(*monitor)) {
                    successfulMonitors.push_back(monitor);
                    
                    auto monitorDuration = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::high_resolution_clock::now() - monitorStart);
                    TRACE_GLOBAL(Trace::Information, (_T("Monitor %d GPU snap completed in %lld μs"), monitor->connectorId, monitorDuration.count()));
                } else {
                    failedMonitors.push_back(monitor);
                    TRACE_GLOBAL(Trace::Warning, (_T("Failed to capture monitor %d on %s, continuing with others"), monitor->connectorId, monitor->devicePath.c_str()));
                }
            }

            if (successfulMonitors.empty()) {
                TRACE_GLOBAL(Trace::Error, (_T("All monitors failed to capture")));
                return false;
            }
            
            if (!failedMonitors.empty()) {
                TRACE_GLOBAL(Trace::Warning, (_T("%d of %d monitors failed capture"), static_cast<int>(failedMonitors.size()), static_cast<int>(allMonitors.size())));
            }

            // Compose successful monitors into final GPU buffer
            auto composeStart = std::chrono::high_resolution_clock::now();
            if (!ComposeMonitorsToGpu(successfulMonitors, bounds)) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to compose monitors on GPU")));
                return false;
            }
            auto composeDuration = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - composeStart);
            TRACE_GLOBAL(Trace::Information, (_T("GPU composition completed in %lld μs"), composeDuration.count()));

            // Read back final composed buffer from GPU to CPU
            std::vector<uint8_t> compositionBuffer(bounds.totalWidth * bounds.totalHeight * 4);
            auto readbackStart = std::chrono::high_resolution_clock::now();
            if (!ReadbackCompositionBuffer(bounds, compositionBuffer)) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to readback composition buffer")));
                return false;
            }
            auto readbackDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - readbackStart);
            TRACE_GLOBAL(Trace::Information, (_T("GPU readback completed in %lld ms"), readbackDuration.count()));

            auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - captureStart);
            TRACE_GLOBAL(Trace::Information, (_T("Total GPU capture completed in %lld ms for %d/%d monitors"), totalDuration.count(), static_cast<int>(successfulMonitors.size()), static_cast<int>(allMonitors.size())));

            // Cleanup GPU resources after capture to prevent accumulation
            for (auto& context : _gpuContexts) {
                if (context->initialized && !context->monitors.empty()) {
                    if (eglMakeCurrent(context->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, context->eglContext)) {
                        // Force GPU completion and release resources
                        glFlush();
                        glFinish();
                        // Release context
                        eglMakeCurrent(context->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                    }
                    break; // Only cleanup the active context
                }
            }

            // Pass composed buffer to store interface
            return storer.R8_G8_B8_A8(compositionBuffer.data(), bounds.totalWidth, bounds.totalHeight);
        }

    private:
        /**
         * Initialize all GPU contexts and display enumeration
         */
        bool Initialize()
        {
            // Enumerate DRM devices using modern API
            drmDevice** devices = nullptr;
            int deviceCount = drmGetDevices2(0, nullptr, 0);

            if (deviceCount <= 0) {
                TRACE_GLOBAL(Trace::Error, (_T("No DRM devices found during enumeration")));
                return false;
            }

            devices = new drmDevice*[deviceCount];
            deviceCount = drmGetDevices2(0, devices, deviceCount);

            if (deviceCount <= 0) {
                delete[] devices;
                TRACE_GLOBAL(Trace::Error, (_T("Failed to enumerate DRM devices")));
                return false;
            }

            TRACE_GLOBAL(Trace::Information, (_T("Found %d DRM devices"), deviceCount));

            // Initialize GPU context for each viable DRM device
            int successfulContexts = 0;
            for (int i = 0; i < deviceCount; ++i) {
                if (devices[i]->available_nodes & (1 << DRM_NODE_PRIMARY)) {
                    std::string devicePath = devices[i]->nodes[DRM_NODE_PRIMARY];
                    TRACE_GLOBAL(Trace::Information, (_T("Attempting to initialize GPU context for device: %s"), devicePath.c_str()));

                    int drmFd = open(devicePath.c_str(), O_RDWR);
                    if (drmFd >= 0) {
                        std::unique_ptr<GpuContext> context(new GpuContext(drmFd, devicePath));
                        if (InitializeGpuContext(*context)) {
                            _gpuContexts.push_back(std::move(context));
                            successfulContexts++;
                            TRACE_GLOBAL(Trace::Information, (_T("Successfully initialized GPU context for %s"), devicePath.c_str()));
                        } else {
                            TRACE_GLOBAL(Trace::Warning, (_T("Failed to initialize GPU context for %s"), devicePath.c_str()));
                            close(drmFd);
                        }
                    } else {
                        TRACE_GLOBAL(Trace::Warning, (_T("Failed to open DRM device %s: %s"), devicePath.c_str(), strerror(errno)));
                    }
                }
            }

            drmFreeDevices(devices, deviceCount);
            delete[] devices;

            if (successfulContexts == 0) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to initialize any GPU contexts")));
                return false;
            }

            TRACE_GLOBAL(Trace::Information, (_T("Initialized %d/%d GPU contexts successfully"), successfulContexts, deviceCount));
            return RefreshAllDisplays();
        }

        /**
         * Initialize GPU context for a single DRM device with EGL 1.4/1.5 and GLES2
         */
        bool InitializeGpuContext(GpuContext& context)
        {
            // Create GBM device
            context.gbmDevice = gbm_create_device(context.drmFd);
            if (!context.gbmDevice) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to create GBM device for %s"), context.devicePath.c_str()));
                return false;
            }

            // Get EGL display
            context.eglDisplay = eglGetDisplay((EGLNativeDisplayType)context.gbmDevice);
            if (context.eglDisplay == EGL_NO_DISPLAY) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to get EGL display for %s"), context.devicePath.c_str()));
                return false;
            }

            // Initialize EGL
            EGLint major, minor;
            if (!eglInitialize(context.eglDisplay, &major, &minor)) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to initialize EGL for %s: %d"), context.devicePath.c_str(), eglGetError()));
                return false;
            }

            TRACE_GLOBAL(Trace::Information, (_T("EGL %d.%d initialized for %s"), major, minor, context.devicePath.c_str()));

            // Verify EGL version compatibility (1.4 minimum)
            if (major < 1 || (major == 1 && minor < 4)) {
                TRACE_GLOBAL(Trace::Error, (_T("EGL version %d.%d is too old, need 1.4+"), major, minor));
                return false;
            }

            // Log EGL extensions for diagnostics
            const char* eglExtensions = eglQueryString(context.eglDisplay, EGL_EXTENSIONS);
            TRACE_GLOBAL(Trace::Information, (_T("EGL Extensions: %s"), eglExtensions ? eglExtensions : "None"));

            // Check for required DMA-BUF import extension
            if (!eglExtensions || !strstr(eglExtensions, "EGL_EXT_image_dma_buf_import")) {
                TRACE_GLOBAL(Trace::Error, (_T("Required EGL_EXT_image_dma_buf_import extension not available")));
                return false;
            }

            // Get EGL extension function pointers
            context.eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
            context.eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");

            if (!context.eglCreateImageKHR || !context.eglDestroyImageKHR) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to get EGL image extension functions")));
                return false;
            }

            // Choose EGL config for OpenGL ES 2.0 - try multiple configurations
            EGLint numConfigs = 0;

            // First try: Surfaceless context (since EGL_KHR_surfaceless_context is available)
            EGLint surfacelessConfigAttribs[] = {
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL_RED_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_BLUE_SIZE, 8,
                EGL_ALPHA_SIZE, 8,
                EGL_NONE
            };

            if (eglChooseConfig(context.eglDisplay, surfacelessConfigAttribs, &context.eglConfig, 1, &numConfigs) && numConfigs > 0) {
                TRACE_GLOBAL(Trace::Information, (_T("Using surfaceless EGL config for %s"), context.devicePath.c_str()));
            } else {
                // Fallback: Try with PBuffer support
                EGLint pbufferConfigAttribs[] = {
                    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
                    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                    EGL_RED_SIZE, 1, // More flexible color requirements
                    EGL_GREEN_SIZE, 1,
                    EGL_BLUE_SIZE, 1,
                    EGL_NONE
                };

                if (eglChooseConfig(context.eglDisplay, pbufferConfigAttribs, &context.eglConfig, 1, &numConfigs) && numConfigs > 0) {
                    TRACE_GLOBAL(Trace::Information, (_T("Using pbuffer EGL config for %s"), context.devicePath.c_str()));
                } else {
                    // Final fallback: Minimal requirements
                    EGLint minimalConfigAttribs[] = {
                        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                        EGL_NONE
                    };

                    if (eglChooseConfig(context.eglDisplay, minimalConfigAttribs, &context.eglConfig, 1, &numConfigs) && numConfigs > 0) {
                        TRACE_GLOBAL(Trace::Information, (_T("Using minimal EGL config for %s"), context.devicePath.c_str()));
                    } else {
                        TRACE_GLOBAL(Trace::Error, (_T("Failed to choose any EGL config for %s"), context.devicePath.c_str()));

                        // Debug: List available configs
                        EGLint totalConfigs = 0;
                        eglGetConfigs(context.eglDisplay, nullptr, 0, &totalConfigs);
                        TRACE_GLOBAL(Trace::Information, (_T("Available EGL configs: %d"), totalConfigs));

                        return false;
                    }
                }
            }

            // Bind OpenGL ES API
            if (!eglBindAPI(EGL_OPENGL_ES_API)) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to bind OpenGL ES API for %s"), context.devicePath.c_str()));
                return false;
            }

            // Create EGL context for OpenGL ES 2.0
            EGLint contextAttribs[] = {
                EGL_CONTEXT_CLIENT_VERSION, 2,
                EGL_NONE
            };

            context.eglContext = eglCreateContext(context.eglDisplay, context.eglConfig, EGL_NO_CONTEXT, contextAttribs);
            if (context.eglContext == EGL_NO_CONTEXT) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to create EGL context for %s: %d"), context.devicePath.c_str(), eglGetError()));
                return false;
            }

            // Make context current for setup
            if (!eglMakeCurrent(context.eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, context.eglContext)) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to make EGL context current for %s"), context.devicePath.c_str()));
                return false;
            }

            // Log platform capabilities for embedded platform diagnostics
            LogPlatformCapabilities(context);

            // Verify OpenGL ES 2.0
            const char* glVersion = (const char*)glGetString(GL_VERSION);
            if (!glVersion || !strstr(glVersion, "OpenGL ES 2.")) {
                TRACE_GLOBAL(Trace::Warning, (_T("OpenGL ES version may not be 2.0: %s"), glVersion ? glVersion : "Unknown"));
            }

            // Check for required OpenGL ES extension
            const char* glExtensions = (const char*)glGetString(GL_EXTENSIONS);
            if (!glExtensions || !strstr(glExtensions, "GL_OES_EGL_image")) {
                TRACE_GLOBAL(Trace::Error, (_T("Required GL_OES_EGL_image extension not available")));
                return false;
            }

            // Get OpenGL ES extension function pointer
            context.glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
            if (!context.glEGLImageTargetTexture2DOES) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to get glEGLImageTargetTexture2DOES function")));
                return false;
            }

            // Initialize OpenGL ES resources for blitting and composition
            if (!InitializeGlResources(context)) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to initialize OpenGL resources for %s"), context.devicePath.c_str()));
                return false;
            }

            // Release context after initialization to avoid conflicts
            eglMakeCurrent(context.eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

            context.initialized = true;
            return true;
        }

        /**
         * Initialize OpenGL ES 2.0 resources for blitting and composition with YUV support
         */
        bool InitializeGlResources(GpuContext& context)
        {
            // Vertex shader for fullscreen quad blitting
            const char* vertexShaderSource = R"(
                attribute vec2 a_position;
                attribute vec2 a_texCoord;
                varying vec2 v_texCoord;
                void main() {
                    gl_Position = vec4(a_position, 0.0, 1.0);
                    v_texCoord = a_texCoord;
                }
            )";

            // Standard RGB fragment shader
            const char* rgbFragmentShaderSource = GetFragmentShaderForFormat(FormatType::SINGLE_PLANE_RGB);

            // YUV fragment shader
            const char* yuvFragmentShaderSource = GetFragmentShaderForFormat(FormatType::SINGLE_PLANE_YUV);

            // Compile RGB shaders
            context.blitVertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
            if (context.blitVertexShader == 0) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to compile RGB vertex shader")));
                return false;
            }

            context.blitFragmentShader = CompileShader(GL_FRAGMENT_SHADER, rgbFragmentShaderSource);
            if (context.blitFragmentShader == 0) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to compile RGB fragment shader")));
                return false;
            }

            // Create RGB shader program
            context.blitProgram = glCreateProgram();
            glAttachShader(context.blitProgram, context.blitVertexShader);
            glAttachShader(context.blitProgram, context.blitFragmentShader);
            glLinkProgram(context.blitProgram);

            GLint linkStatus;
            glGetProgramiv(context.blitProgram, GL_LINK_STATUS, &linkStatus);
            if (linkStatus != GL_TRUE) {
                GLchar infoLog[512];
                glGetProgramInfoLog(context.blitProgram, sizeof(infoLog), nullptr, infoLog);
                TRACE_GLOBAL(Trace::Error, (_T("Failed to link RGB shader program: %s"), infoLog));
                return false;
            }

            // Get RGB attribute and uniform locations
            context.blitPositionAttrib = glGetAttribLocation(context.blitProgram, "a_position");
            context.blitTexCoordAttrib = glGetAttribLocation(context.blitProgram, "a_texCoord");
            context.blitTextureUniform = glGetUniformLocation(context.blitProgram, "u_texture");

            // Compile YUV shaders
            context.yuvVertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
            if (context.yuvVertexShader == 0) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to compile YUV vertex shader")));
                return false;
            }

            context.yuvFragmentShader = CompileShader(GL_FRAGMENT_SHADER, yuvFragmentShaderSource);
            if (context.yuvFragmentShader == 0) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to compile YUV fragment shader")));
                return false;
            }

            // Create YUV shader program
            context.yuvProgram = glCreateProgram();
            glAttachShader(context.yuvProgram, context.yuvVertexShader);
            glAttachShader(context.yuvProgram, context.yuvFragmentShader);
            glLinkProgram(context.yuvProgram);

            glGetProgramiv(context.yuvProgram, GL_LINK_STATUS, &linkStatus);
            if (linkStatus != GL_TRUE) {
                GLchar infoLog[512];
                glGetProgramInfoLog(context.yuvProgram, sizeof(infoLog), nullptr, infoLog);
                TRACE_GLOBAL(Trace::Error, (_T("Failed to link YUV shader program: %s"), infoLog));
                return false;
            }

            // Get YUV attribute and uniform locations
            context.yuvPositionAttrib = glGetAttribLocation(context.yuvProgram, "a_position");
            context.yuvTexCoordAttrib = glGetAttribLocation(context.yuvProgram, "a_texCoord");
            context.yuvTextureUniform = glGetUniformLocation(context.yuvProgram, "u_texture");

            // Create vertex buffer for fullscreen quad with flipped texture coordinates
            float vertices[] = {
                -1.0f, -1.0f, 0.0f, 0.0f, // Bottom-left: position + flipped texcoord
                1.0f, -1.0f, 1.0f, 0.0f, // Bottom-right
                -1.0f, 1.0f, 0.0f, 1.0f, // Top-left
                1.0f, 1.0f, 1.0f, 1.0f // Top-right
            };

            glGenBuffers(1, &context.blitVertexBuffer);
            glBindBuffer(GL_ARRAY_BUFFER, context.blitVertexBuffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            // Check for OpenGL errors
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                TRACE_GLOBAL(Trace::Error, (_T("OpenGL error during resource initialization: %d"), error));
                return false;
            }

            return true;
        }

        /**
         * Compile OpenGL ES shader
         */
        GLuint CompileShader(GLenum type, const char* source)
        {
            GLuint shader = glCreateShader(type);
            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);

            GLint compileStatus;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
            if (compileStatus != GL_TRUE) {
                GLchar infoLog[512];
                glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
                TRACE_GLOBAL(Trace::Error, (_T("Failed to compile shader: %s"), infoLog));
                glDeleteShader(shader);
                return 0;
            }

            return shader;
        }

        /**
         * Refresh displays for all GPU contexts (handles hotplug)
         */
        bool RefreshAllDisplays()
        {
            bool foundAnyDisplays = false;

            for (auto& context : _gpuContexts) {
                if (RefreshDisplaysForContext(*context)) {
                    foundAnyDisplays = true;
                }
            }

            return foundAnyDisplays;
        }

        /**
         * Refresh displays for a single GPU context with enhanced format detection
         */
        bool RefreshDisplaysForContext(GpuContext& context)
        {
            // Cleanup existing monitor GPU resources
            for (auto& monitor : context.monitors) {
                context.CleanupMonitorGpuResources(monitor);
            }
            context.monitors.clear();

            drmModeRes* resources = drmModeGetResources(context.drmFd);
            if (!resources) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to get DRM resources for %s"), context.devicePath.c_str()));
                return false;
            }

            bool foundDisplays = false;

            for (int i = 0; i < resources->count_connectors; ++i) {
                drmModeConnector* connector = drmModeGetConnector(context.drmFd, resources->connectors[i]);
                if (!connector)
                    continue;

                if (connector->connection == DRM_MODE_CONNECTED && connector->encoder_id) {
                    drmModeEncoder* encoder = drmModeGetEncoder(context.drmFd, connector->encoder_id);
                    if (encoder && encoder->crtc_id) {
                        drmModeCrtc* crtc = drmModeGetCrtc(context.drmFd, encoder->crtc_id);
                        if (crtc && crtc->mode_valid && crtc->buffer_id) {
                            MonitorInfo monitor;
                            monitor.connectorId = connector->connector_id;
                            monitor.crtcId = encoder->crtc_id;
                            monitor.width = crtc->width;
                            monitor.height = crtc->height;
                            monitor.x = crtc->x;
                            monitor.y = crtc->y;
                            monitor.active = true;
                            monitor.framebufferId = crtc->buffer_id;
                            monitor.drmFd = context.drmFd;
                            monitor.devicePath = context.devicePath;

                            // Detect framebuffer format
                            FramebufferInfo fbInfo;
                            if (GetFramebufferInfo(context.drmFd, crtc->buffer_id, fbInfo)) {
                                monitor.pixelFormat = fbInfo.format;
                                monitor.formatType = ClassifyFormat(fbInfo.format);
                                TRACE_GLOBAL(Trace::Information, (_T("Monitor %d format: 0x%x (%s) - %s"), 
                                    monitor.connectorId, monitor.pixelFormat, FormatToString(monitor.pixelFormat),
                                    monitor.formatType == FormatType::SINGLE_PLANE_RGB ? "RGB" :
                                    monitor.formatType == FormatType::SINGLE_PLANE_YUV ? "YUV-Single" :
                                    monitor.formatType == FormatType::MULTI_PLANE_YUV ? "YUV-Multi" : "Unsupported"));
                            } else {
                                TRACE_GLOBAL(Trace::Warning, (_T("Failed to detect format for monitor %d, assuming XRGB8888"), monitor.connectorId));
                                monitor.pixelFormat = DRM_FORMAT_XRGB8888;
                                monitor.formatType = FormatType::SINGLE_PLANE_RGB;
                            }

                            // Setup VSync support - find CRTC index
                            monitor.crtcIndex = 0;
                            for (int crtcIdx = 0; crtcIdx < resources->count_crtcs; ++crtcIdx) {
                                if (resources->crtcs[crtcIdx] == encoder->crtc_id) {
                                    monitor.crtcIndex = crtcIdx;
                                    monitor.vsyncEnabled = true;
                                    break;
                                }
                            }

                            context.monitors.push_back(monitor);
                            foundDisplays = true;

                            TRACE_GLOBAL(Trace::Information, (_T("Found monitor %d on %s: %dx%d at (%d, %d) with VSync %s"), 
                                monitor.connectorId, context.devicePath.c_str(), monitor.width, monitor.height, 
                                monitor.x, monitor.y, monitor.vsyncEnabled ? "enabled" : "disabled"));
                        }
                        drmModeFreeCrtc(crtc);
                    }
                    drmModeFreeEncoder(encoder);
                }
                drmModeFreeConnector(connector);
            }

            drmModeFreeResources(resources);
            return foundDisplays;
        }

        /**
         * Calculate composition bounds for all monitors
         */
        CompositionBounds CalculateCompositionBounds(const std::vector<MonitorInfo*>& monitors)
        {
            CompositionBounds bounds;

            if (monitors.empty()) {
                return bounds;
            }

            int32_t maxX = monitors[0]->x + static_cast<int32_t>(monitors[0]->width);
            int32_t maxY = monitors[0]->y + static_cast<int32_t>(monitors[0]->height);
            bounds.minX = monitors[0]->x;
            bounds.minY = monitors[0]->y;

            for (const auto* monitor : monitors) {
                bounds.minX = std::min(bounds.minX, monitor->x);
                bounds.minY = std::min(bounds.minY, monitor->y);
                maxX = std::max(maxX, monitor->x + static_cast<int32_t>(monitor->width));
                maxY = std::max(maxY, monitor->y + static_cast<int32_t>(monitor->height));
            }

            bounds.totalWidth = static_cast<uint32_t>(maxX - bounds.minX);
            bounds.totalHeight = static_cast<uint32_t>(maxY - bounds.minY);

            return bounds;
        }

        /**
         * Setup composition buffer for final GPU composition
         */
        bool SetupCompositionBuffer(const CompositionBounds& bounds)
        {
            // Find a GPU context that actually has displays (not the first one blindly)
            GpuContext* context = nullptr;
            for (auto& ctx : _gpuContexts) {
                if (ctx->initialized && !ctx->monitors.empty()) {
                    context = ctx.get();
                    break;
                }
            }

            if (!context) {
                TRACE_GLOBAL(Trace::Error, (_T("No GPU context with displays found for composition")));
                return false;
            }

            TRACE_GLOBAL(Trace::Information, (_T("Using GPU context %s for composition"), context->devicePath.c_str()));

            // Verify context is still valid
            if (context->eglContext == EGL_NO_CONTEXT || context->eglDisplay == EGL_NO_DISPLAY) {
                TRACE_GLOBAL(Trace::Error, (_T("Invalid EGL context or display for %s"), context->devicePath.c_str()));
                return false;
            }

            // Ensure no other context is current first
            eglMakeCurrent(context->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

            if (!eglMakeCurrent(context->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, context->eglContext)) {
                EGLint eglError = eglGetError();
                TRACE_GLOBAL(Trace::Error, (_T("Failed to make EGL context current for composition setup. EGL error: 0x%x"), eglError));

                // Additional debugging
                TRACE_GLOBAL(Trace::Information, (_T("Context: %p, Display: %p"), context->eglContext, context->eglDisplay));
                return false;
            }

            // Check if composition buffer needs resize
            if (context->compositionWidth != bounds.totalWidth || context->compositionHeight != bounds.totalHeight) {
                // Cleanup existing composition resources
                if (context->compositionFramebuffer) {
                    glDeleteFramebuffers(1, &context->compositionFramebuffer);
                    context->compositionFramebuffer = 0;
                }
                if (context->compositionTexture) {
                    glDeleteTextures(1, &context->compositionTexture);
                    context->compositionTexture = 0;
                }

                // Create composition texture
                glGenTextures(1, &context->compositionTexture);
                glBindTexture(GL_TEXTURE_2D, context->compositionTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bounds.totalWidth, bounds.totalHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                // Create composition framebuffer
                glGenFramebuffers(1, &context->compositionFramebuffer);
                glBindFramebuffer(GL_FRAMEBUFFER, context->compositionFramebuffer);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, context->compositionTexture, 0);

                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                    TRACE_GLOBAL(Trace::Error, (_T("Composition framebuffer is not complete")));
                    return false;
                }

                context->compositionWidth = bounds.totalWidth;
                context->compositionHeight = bounds.totalHeight;

                TRACE_GLOBAL(Trace::Information, (_T("Setup composition buffer: %dx%d"), bounds.totalWidth, bounds.totalHeight));
            }

            // Clear composition buffer
            glBindFramebuffer(GL_FRAMEBUFFER, context->compositionFramebuffer);
            glViewport(0, 0, bounds.totalWidth, bounds.totalHeight);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            return true;
        }

        /**
         * Enhanced capture with multi-plane framebuffer support
         * This is the critical <10ms performance path
         */
        bool CaptureMonitorToGpu(MonitorInfo& monitor)
        {
            // Find the GPU context for this monitor
            GpuContext* context = nullptr;
            for (auto& ctx : _gpuContexts) {
                if (ctx->drmFd == monitor.drmFd) {
                    context = ctx.get();
                    break;
                }
            }

            if (!context || !context->initialized) {
                TRACE_GLOBAL(Trace::Error, (_T("No valid GPU context found for monitor %d"), monitor.connectorId));
                return false;
            }

            // Verify context is still valid
            if (context->eglContext == EGL_NO_CONTEXT || context->eglDisplay == EGL_NO_DISPLAY) {
                TRACE_GLOBAL(Trace::Error, (_T("Invalid EGL context or display for monitor %d"), monitor.connectorId));
                return false;
            }

            // Ensure no other context is current first
            eglMakeCurrent(context->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

            if (!eglMakeCurrent(context->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, context->eglContext)) {
                EGLint eglError = eglGetError();
                TRACE_GLOBAL(Trace::Error, (_T("Failed to make EGL context current for monitor capture. EGL error: 0x%x"), eglError));
                return false;
            }

            TRACE_GLOBAL(Trace::Information, (_T("Made EGL context current for monitor %d on %s"), monitor.connectorId, context->devicePath.c_str()));

            // Get comprehensive framebuffer info with multi-plane support
            FramebufferInfo fbInfo;
            if (!GetFramebufferInfo(monitor.drmFd, monitor.framebufferId, fbInfo)) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to get framebuffer info for monitor %d"), monitor.connectorId));
                return false;
            }

            // Cleanup previous GPU resources for this monitor BEFORE creating new ones
            context->CleanupMonitorGpuResources(monitor);

            // Force OpenGL state cleanup
            glBindTexture(GL_TEXTURE_2D, 0);
            glFlush();

            // Create EGL image based on detected format and plane configuration
            if (!CreateEGLImageFromFramebuffer(*context, fbInfo, monitor.texture, monitor.eglImage)) {
                TRACE_GLOBAL(Trace::Error, (_T("Failed to create EGL image for monitor %d with format %s"), 
                    monitor.connectorId, FormatToString(fbInfo.format)));
                return false;
            }

            monitor.gpuResourcesReady = true;

            // Force GPU state completion and check for errors
            glFlush();
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                TRACE_GLOBAL(Trace::Error, (_T("OpenGL error during monitor capture: %d"), error));
                context->CleanupMonitorGpuResources(monitor);
                return false;
            }

            return true;
        }

        /**
         * Compose all monitor textures into final GPU buffer with format-aware rendering
         */
        bool ComposeMonitorsToGpu(const std::vector<MonitorInfo*>& monitors, const CompositionBounds& bounds)
        {
            // Find a GPU context that actually has displays (same as composition setup)
            GpuContext* context = nullptr;
            for (auto& ctx : _gpuContexts) {
                if (ctx->initialized && !ctx->monitors.empty()) {
                    context = ctx.get();
                    break;
                }
            }

            if (!context) {
                TRACE_GLOBAL(Trace::Error, (_T("No GPU context with displays found for composition")));
                return false;
            }

            // Verify context is still valid
            if (context->eglContext == EGL_NO_CONTEXT || context->eglDisplay == EGL_NO_DISPLAY) {
                TRACE_GLOBAL(Trace::Error, (_T("Invalid EGL context or display for composition")));
                return false;
            }

            // Ensure no other context is current first
            eglMakeCurrent(context->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

            if (!eglMakeCurrent(context->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, context->eglContext)) {
                EGLint eglError = eglGetError();
                TRACE_GLOBAL(Trace::Error, (_T("Failed to make EGL context current for composition. EGL error: 0x%x"), eglError));
                return false;
            }

            // Bind composition framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, context->compositionFramebuffer);
            glViewport(0, 0, bounds.totalWidth, bounds.totalHeight);

            glBindBuffer(GL_ARRAY_BUFFER, context->blitVertexBuffer);

            // Render each monitor texture to its position in composition
            for (const auto* monitor : monitors) {
                if (!monitor->gpuResourcesReady || monitor->texture == 0) {
                    continue;
                }

                // Choose appropriate shader program based on format
                GLuint program;
                GLuint positionAttrib, texCoordAttrib, textureUniform;
                
                if (monitor->formatType == FormatType::SINGLE_PLANE_YUV || monitor->formatType == FormatType::MULTI_PLANE_YUV) {
                    program = context->yuvProgram;
                    positionAttrib = context->yuvPositionAttrib;
                    texCoordAttrib = context->yuvTexCoordAttrib;
                    textureUniform = context->yuvTextureUniform;
                } else {
                    program = context->blitProgram;
                    positionAttrib = context->blitPositionAttrib;
                    texCoordAttrib = context->blitTexCoordAttrib;
                    textureUniform = context->blitTextureUniform;
                }

                glUseProgram(program);

                // Setup vertex attributes
                glEnableVertexAttribArray(positionAttrib);
                glVertexAttribPointer(positionAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(texCoordAttrib);
                glVertexAttribPointer(texCoordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

                // Calculate normalized coordinates for this monitor's position
                float normalizedX = static_cast<float>(monitor->x - bounds.minX) / static_cast<float>(bounds.totalWidth);
                float normalizedY = static_cast<float>(monitor->y - bounds.minY) / static_cast<float>(bounds.totalHeight);
                float normalizedWidth = static_cast<float>(monitor->width) / static_cast<float>(bounds.totalWidth);
                float normalizedHeight = static_cast<float>(monitor->height) / static_cast<float>(bounds.totalHeight);

                // Convert to OpenGL NDC coordinates (-1 to 1)
                float left = normalizedX * 2.0f - 1.0f;
                float right = (normalizedX + normalizedWidth) * 2.0f - 1.0f;
                float bottom = (1.0f - normalizedY - normalizedHeight) * 2.0f - 1.0f;
                float top = (1.0f - normalizedY) * 2.0f - 1.0f;

                // Update vertex buffer with monitor position (with corrected texture coordinates)
                float positionVertices[] = {
                    left, bottom, 0.0f, 0.0f, // Bottom-left: corrected texcoord
                    right, bottom, 1.0f, 0.0f, // Bottom-right
                    left, top, 0.0f, 1.0f, // Top-left
                    right, top, 1.0f, 1.0f // Top-right
                };

                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(positionVertices), positionVertices);

                // Bind monitor texture
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, monitor->texture);
                glUniform1i(textureUniform, 0);

                // Render monitor
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                // Cleanup
                glDisableVertexAttribArray(positionAttrib);
                glDisableVertexAttribArray(texCoordAttrib);
            }

            return true;
        }

        /**
         * Read back final composition buffer from GPU to CPU as RGBA8888
         */
        bool ReadbackCompositionBuffer(const CompositionBounds& bounds, std::vector<uint8_t>& buffer)
        {
            // Find a GPU context that actually has displays (same as composition setup)
            GpuContext* context = nullptr;
            for (auto& ctx : _gpuContexts) {
                if (ctx->initialized && !ctx->monitors.empty()) {
                    context = ctx.get();
                    break;
                }
            }

            if (!context) {
                TRACE_GLOBAL(Trace::Error, (_T("No GPU context with displays found for readback")));
                return false;
            }

            // Verify context is still valid
            if (context->eglContext == EGL_NO_CONTEXT || context->eglDisplay == EGL_NO_DISPLAY) {
                TRACE_GLOBAL(Trace::Error, (_T("Invalid EGL context or display for readback")));
                return false;
            }

            // Ensure no other context is current first
            eglMakeCurrent(context->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

            if (!eglMakeCurrent(context->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, context->eglContext)) {
                EGLint eglError = eglGetError();
                TRACE_GLOBAL(Trace::Error, (_T("Failed to make EGL context current for readback. EGL error: 0x%x"), eglError));
                return false;
            }

            // Bind composition framebuffer for reading
            glBindFramebuffer(GL_FRAMEBUFFER, context->compositionFramebuffer);

            // Read pixels from GPU to CPU
            glReadPixels(0, 0, bounds.totalWidth, bounds.totalHeight, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

            // Check for OpenGL errors
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                TRACE_GLOBAL(Trace::Error, (_T("OpenGL error during composition readback: %d"), error));
                return false;
            }

            return true;
        }

        /**
         * Cleanup all GPU contexts and resources
         */
        void Deinitialize()
        {
            _gpuContexts.clear(); // Unique_ptr cleanup will call GpuContext destructors
            TRACE_GLOBAL(Trace::Information, (_T("GpuDrmCapture: Deinitialized")));
        }

    private:
        std::vector<std::unique_ptr<GpuContext>> _gpuContexts;
    };

} // namespace Plugin

/* static */ Exchange::ICapture* Exchange::ICapture::Instance()
{
    return (Core::ServiceType<Plugin::GpuDrmCapture>::Create<Exchange::ICapture>());
}

} // namespace Thunder