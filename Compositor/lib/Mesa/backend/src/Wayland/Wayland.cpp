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

#include "../Module.h"

#include <CompositorTypes.h>
#include <DRM.h>
#include <IBuffer.h>
#include <interfaces/IComposition.h>
#include <interfaces/ICompositionBuffer.h>

#include "IOutput.h"
#include "Input.h"
#include "Output.h"

#include <sys/mman.h>

#include <drm_fourcc.h>
#include <wayland-client.h>
#include <xf86drm.h>

#include "generated/drm-client-protocol.h"
#include "generated/linux-dmabuf-unstable-v1-client-protocol.h"
#include "generated/pointer-gestures-unstable-v1-client-protocol.h"
#include "generated/presentation-time-client-protocol.h"
#include "generated/relative-pointer-unstable-v1-client-protocol.h"
#include "generated/xdg-activation-v1-client-protocol.h"
#include "generated/xdg-decoration-unstable-v1-client-protocol.h"
#include "generated/xdg-shell-client-protocol.h"

MODULE_NAME_ARCHIVE_DECLARATION

namespace Thunder {
namespace Compositor {
    namespace Backend {

        class WaylandImplementation : public Wayland::IBackend {
        public:
            typedef struct __attribute__((packed, aligned(4))) {
                uint32_t format;
                uint32_t padding;
                uint64_t modifier;
            } WaylandFormat;

            using FormatRegister = std::unordered_map<uint32_t, std::vector<uint64_t> >;

            class ServerMonitor : public Core::IResource {
            public:
                ServerMonitor() = delete;
                ServerMonitor(ServerMonitor&&) = delete;
                ServerMonitor(const ServerMonitor&) = delete;
                ServerMonitor& operator=(const ServerMonitor&) = delete;

                ServerMonitor(const WaylandImplementation& parent, const int fd)
                    : _parent(parent)
                    , _fd(fd)
                {
                }

                virtual ~ServerMonitor()
                {
                }

                handle Descriptor() const
                {
                    return _fd;
                }

                uint16_t Events()
                {
                    return POLLIN | POLLOUT | POLLERR | POLLHUP;
                }

                void Handle(const uint16_t events)
                {
                    _parent.Dispatch(events);
                }

            private:
                const WaylandImplementation& _parent;
                const int _fd;

            }; // ServerMonitor

            WaylandImplementation(WaylandImplementation&&) = delete;
            WaylandImplementation(const WaylandImplementation&) = delete;
            WaylandImplementation& operator=(const WaylandImplementation&) = delete;
            WaylandImplementation();

            virtual ~WaylandImplementation();

            uint32_t AddRef() const override;
            uint32_t Release() const override;

            wl_surface* Surface() const override;
            xdg_surface* WindowSurface(wl_surface* surface) const override;
            int RoundTrip() const override;
            int Flush() const override;
            void Format(const Compositor::PixelFormat& input, uint32_t& format, uint64_t& modifier) const override;
            int RenderNode() const override;

            wl_buffer* CreateBuffer(Exchange::ICompositionBuffer* buffer) const override;

            struct zxdg_toplevel_decoration_v1* GetWindowDecorationInterface(xdg_toplevel* topLevelSurface) const override;
            struct wp_presentation_feedback* GetFeedbackInterface(wl_surface* surface) const override;

            void RegisterInterface(struct wl_registry* registry, uint32_t name, const char* iface, uint32_t version);

            void PresentationClock(const uint32_t clockType);
            void HandleDmaFormatTable(int32_t fd, uint32_t size);

            void OpenDrmRender(drmDevice* device);
            void OpenDrmRender(const std::string& name);

            int Dispatch(const uint32_t events) const;

            Core::ProxyType<IOutput> Output(const string& name, const Exchange::IComposition::Rectangle& rectangle, const Compositor::PixelFormat& format, Compositor::IOutput::ICallback* feedback);

        private:
            mutable uint32_t _refCount;
            int _drmRenderFd;
            std::string _activationToken;

            wl_display* _wlDisplay;
            wl_registry* _wlRegistry;
            wl_compositor* _wlCompositor;
            wl_drm* _wlDrm;
            clockid_t _presentationClock;

            xdg_wm_base* _xdgWmBase;
            zxdg_decoration_manager_v1* _wlZxdgDecorationManagerV1;
            zwp_pointer_gestures_v1* _wlZwpPointerGesturesV1;
            wp_presentation* _wlPresentation;
            zwp_linux_dmabuf_v1* _wlZwpLinuxDmabufV1;
            wl_shm* _wlShm;
            zwp_relative_pointer_manager_v1* _wlZwpRelativePointerManagerV1;
            xdg_activation_v1* _wlXdgActivationV1;
            zwp_linux_dmabuf_feedback_v1* _wlZwpLinuxDmabufFeedbackV1;
            FormatRegister _dmaFormats;

            Input _input;
            ServerMonitor _serverMonitor;

            Core::ProxyMapType<string, Exchange::ICompositionBuffer> _windows;

        }; // class WaylandImplementation

        /**
         * @brief Callback for the clock_id event on the presentation object
         *
         * @param data User data passed to the callback
         * @param presentation The wp_presentation object
         * @param clock The clock id of the presentation object
         *
         */
        static void onPresentationClockId(void* data, struct wp_presentation* presentation, uint32_t clockType)
        {
            WaylandImplementation* implementation = static_cast<WaylandImplementation*>(data);
            implementation->PresentationClock(clockType);
        }

        static const struct wp_presentation_listener presentationListener = {
            .clock_id = onPresentationClockId,
        };

        /**
         * @brief Callback for the ping event on the xdg_wm_base object
         *
         * @param data User data passed to the callback
         * @param base The xdg_wm_base object
         * @param serial The serial of the ping event
         *
         */
        static void onXdgWmBasePing(void* data, struct xdg_wm_base* base, uint32_t serial)
        {
            xdg_wm_base_pong(base, serial);
        }

        static const struct xdg_wm_base_listener xdgWmBaseListener = {
            .ping = onXdgWmBasePing,
        };

        /**
         * @brief Callback for the configure event on the surface
         *
         * @param data User data passed to the callback
         * @param xdg_surface The xdg_surface object
         * @param serial The serial of the configure event
         *
         */
        static void onDrmHandleDevice(void* data, struct wl_drm* drm, const char* name)
        {
            WaylandImplementation* implementation = static_cast<WaylandImplementation*>(data);
            implementation->OpenDrmRender(name);
        }

        /**
         * @brief Callback for the format event on the drm object
         *
         * @param data User data passed to the callback
         * @param drm The wl_drm object
         * @param format The format of the drm object
         *
         */
        static void onDrmHandleFormat(void* data, struct wl_drm* drm, uint32_t format)
        {
            /* ignore this event */
        }

        /**
         * @brief Callback for the authenticated event on the drm object
         *
         * @param data User data passed to the callback
         * @param drm The wl_drm object
         *
         */
        static void onDrmHandleAuthenticated(void* data, struct wl_drm* drm)
        {
            /* ignore this event */
        }

        /**
         * @brief Callback for the device event on the drm object
         *
         * @param data User data passed to the callback
         * @param drm The wl_drm object
         * @param name The name of the device
         *
         */
        static void onDrmHandleCapabilities(void* data, struct wl_drm* drm, uint32_t caps)
        {
            /* ignore this event */
        }

        static const struct wl_drm_listener drmListener = {
            .device = onDrmHandleDevice,
            .format = onDrmHandleFormat,
            .authenticated = onDrmHandleAuthenticated,
            .capabilities = onDrmHandleCapabilities,
        };

        /**
         * @brief Callback for the format event on the shm object
         *
         * @param data User data passed to the callback
         * @param shm The wl_shm object
         * @param format The format of the shm object
         *
         */
        static void onShmFormat(void* data, struct wl_shm* shm, uint32_t format)
        {
            if (format != DRM_FORMAT_INVALID) {
                TRACE_GLOBAL(Trace::Backend, ("Found SHM format: %s", DRM::FormatToString(format)));
            }
        }

        static const struct wl_shm_listener shmListener = {
            .format = onShmFormat,
        };

#ifdef USE_LEGACY_WAYLAND
#warning "Note: Using legacy Linux dmabuf Wayland events"
        static void linuxDmabufV1HandleFormat(void* data, struct zwp_linux_dmabuf_v1* linux_dmabuf_v1, uint32_t format)
        {
            if (format != DRM_FORMAT_INVALID) {
                TRACE_GLOBAL(Trace::Backend, ("Found DMA format: %s", DRM::FormatString(format)));
            }
        }

        static void linuxDmabufV1HandleModifier(void* data, struct zwp_linux_dmabuf_v1* linux_dmabuf_v1, uint32_t format, uint32_t modifier_hi, uint32_t modifier_lo)
        {
            uint64_t modifier = ((uint64_t)modifier_hi << 32) | modifier_lo;

            if (format != DRM_FORMAT_INVALID) {
                TRACE_GLOBAL(Trace::Backend, ("Found DMA format: %s: 0x%" PRIX64, DRM::FormatString(format), modifier));
            }
        }

        static const struct zwp_linux_dmabuf_v1_listener linuxDmabufV1Listener = {
            .format = linuxDmabufV1HandleFormat,
            .modifier = linuxDmabufV1HandleModifier,
        };
#else

        /**
         * @brief Callback for the feedback done event on the feedback object
         *
         * @param data User data passed to the callback
         * @param zwp_linux_dmabuf_feedback_v1 The zwp_linux_dmabuf_feedback_v1 object
         *
         */
        static void onLinuxDmabufFeedbackDone(void* data, struct zwp_linux_dmabuf_feedback_v1* zwp_linux_dmabuf_feedback_v1)
        {
            /* ignore this event */
            TRACE_GLOBAL(Trace::Backend, ("DMA Buffer feedback complete"));
        }

        /**
         * @brief Callback for the format table event on the feedback object
         *
         * @param data User data passed to the callback
         * @param zwp_linux_dmabuf_feedback_v1 The zwp_linux_dmabuf_feedback_v1 object
         * @param fd The file descriptor of the format table
         * @param size The size of the format table
         *
         */

        static void onLinuxDmabufFeedbackFormatTable(void* data, struct zwp_linux_dmabuf_feedback_v1* zwp_linux_dmabuf_feedback_v1, int32_t fd, uint32_t size)
        {
            WaylandImplementation* implementation = static_cast<WaylandImplementation*>(data);
            implementation->HandleDmaFormatTable(fd, size);
        }

        /**
         * @brief Callback for the main device event on the feedback object
         *
         * @param data User data passed to the callback
         * @param zwp_linux_dmabuf_feedback_v1 The zwp_linux_dmabuf_feedback_v1 object
         * @param mainDevice The main device of the feedback object
         *
         */
        static void onLinuxDmabufFeedbackMainDevice(void* data, struct zwp_linux_dmabuf_feedback_v1* zwp_linux_dmabuf_feedback_v1, struct wl_array* mainDevice)
        {
            WaylandImplementation* implementation = static_cast<WaylandImplementation*>(data);

            if (implementation != nullptr) {
                dev_t device;
                assert(mainDevice->size == sizeof(device));
                memcpy(&device, mainDevice->data, sizeof(device));

                drmDevicePtr drmDev(nullptr);

                if (drmGetDeviceFromDevId(device, /* flags */ 0, &drmDev) != 0) {
                    TRACE_GLOBAL(Trace::Backend, ("Failed to open main DRM device %p", drmDev));
                    return;
                }

                implementation->OpenDrmRender(drmDev);
            }
        }

        /**
         * @brief Callback for the tranche done event on the feedback object
         *
         * @param data User data passed to the callback
         * @param zwp_linux_dmabuf_feedback_v1 The zwp_linux_dmabuf_feedback_v1 object
         *
         */
        static void onLinuxDmabufFeedbackTranceDone(void* data, struct zwp_linux_dmabuf_feedback_v1* zwp_linux_dmabuf_feedback_v1)
        {
            /* ignore this event */
        }

        /**
         * @brief Callback for the tranche target device event on the feedback object
         *
         * @param data User data passed to the callback
         * @param zwp_linux_dmabuf_feedback_v1 The zwp_linux_dmabuf_feedback_v1 object
         * @param device The target device of the tranche
         *
         */
        static void onLinuxDmabufFeedbackTrancheTargetDevice(void* data, struct zwp_linux_dmabuf_feedback_v1* zwp_linux_dmabuf_feedback_v1, struct wl_array* device)
        {
            /* ignore this event */
        }

        /**
         * @brief Callback for the tranche formats event on the feedback object
         *
         * @param data User data passed to the callback
         * @param zwp_linux_dmabuf_feedback_v1 The zwp_linux_dmabuf_feedback_v1 object
         * @param indices The indices of the tranche
         *
         */
        static void onLinuxDmabufFeedbackTrancheFormats(void* data, struct zwp_linux_dmabuf_feedback_v1* zwp_linux_dmabuf_feedback_v1, struct wl_array* indices)
        {
            /* ignore this event */
        }

        /**
         * @brief Callback for the tranche flags event on the feedback object
         *
         * @param data User data passed to the callback
         * @param zwp_linux_dmabuf_feedback_v1 The zwp_linux_dmabuf_feedback_v1 object
         * @param flags The flags of the tranche
         *
         */

        static void onLinuxDmabufFeedbackTrancheFlags(void* data, struct zwp_linux_dmabuf_feedback_v1* zwp_linux_dmabuf_feedback_v1, uint32_t flags)
        {
            /* ignore this event */
        }

        static const struct zwp_linux_dmabuf_feedback_v1_listener linuxDmabufFeedbackV1Listener = {
            .done = onLinuxDmabufFeedbackDone,
            .format_table = onLinuxDmabufFeedbackFormatTable,
            .main_device = onLinuxDmabufFeedbackMainDevice,
            .tranche_done = onLinuxDmabufFeedbackTranceDone,
            .tranche_target_device = onLinuxDmabufFeedbackTrancheTargetDevice,
            .tranche_formats = onLinuxDmabufFeedbackTrancheFormats,
            .tranche_flags = onLinuxDmabufFeedbackTrancheFlags,
        };
#endif // USE_LEGACY_WAYLAND

        /**
         * @brief Callback for the global event on the registry
         *
         * @param data User data passed to the callback
         * @param registry The wl_registry object
         * @param name The name of the global object
         * @param iface The interface of the global object
         * @param version The version of the global object
         *
         */
        static void onRegistryGlobal(void* data, struct wl_registry* registry, uint32_t name, const char* iface, uint32_t version)
        {
            WaylandImplementation* implementation = static_cast<WaylandImplementation*>(data);

            if (implementation != nullptr) {
                implementation->RegisterInterface(registry, name, iface, version);
            }
        }

        /**
         * @brief Callback for the global_remove event on the registry
         *
         * @param data User data passed to the callback
         * @param registry The wl_registry object
         * @param name The name of the global object
         *
         */
        static void onRegistryGlobalRemove(void* data, struct wl_registry* registry, uint32_t name)
        {
            /* ignore this event */
        }

        static const struct wl_registry_listener globalRegistryListener = {
            .global = onRegistryGlobal,
            .global_remove = onRegistryGlobalRemove
        };

        static const TCHAR* DisplayName() {
            return ("");
        }

        WaylandImplementation::WaylandImplementation()
            : _refCount(0)
            , _drmRenderFd(InvalidFileDescriptor)
            , _activationToken()
            , _wlDisplay(wl_display_connect(DisplayName()[0] != '\0' ? DisplayName() : nullptr))
            , _wlRegistry(wl_display_get_registry(_wlDisplay))
            , _wlCompositor(nullptr)
            , _wlDrm(nullptr)
            , _presentationClock(CLOCK_MONOTONIC)
            , _wlZxdgDecorationManagerV1(nullptr)
            , _wlZwpPointerGesturesV1(nullptr)
            , _wlPresentation(nullptr)
            , _wlZwpLinuxDmabufV1(nullptr)
            , _wlShm(nullptr)
            , _wlZwpRelativePointerManagerV1(nullptr)
            , _wlXdgActivationV1(nullptr)
            , _wlZwpLinuxDmabufFeedbackV1(nullptr)
            , _dmaFormats()
            , _input()
            , _serverMonitor(*this, wl_display_get_fd(_wlDisplay))
            , _windows()
        {
            TRACE(Trace::Backend, ("Starting WaylandImplementation backend"));
            ASSERT(_wlDisplay != nullptr);
            ASSERT(_wlRegistry != nullptr);

            wl_registry_add_listener(_wlRegistry, &globalRegistryListener, this);

            // Query to bind the global registry
            RoundTrip();

            ASSERT(_wlCompositor != nullptr);
            ASSERT(_xdgWmBase != nullptr);

            // Query available drm formats
            RoundTrip();

            ASSERT(_dmaFormats.size() > 0);

            if (const char* token = getenv("XDG_ACTIVATION_TOKEN")) {
                _activationToken = token;
                TRACE(Trace::Backend, ("Got an activation token"));

                unsetenv("XDG_ACTIVATION_TOKEN");
            }

            wl_display_flush(_wlDisplay);
            Core::ResourceMonitor::Instance().Register(_serverMonitor);
        }

        WaylandImplementation::~WaylandImplementation()
        {
            Core::ResourceMonitor::Instance().Unregister(_serverMonitor);

            if (_drmRenderFd != InvalidFileDescriptor) {
                close(_drmRenderFd);
            }

            if (_wlZxdgDecorationManagerV1 != nullptr) {
                zxdg_decoration_manager_v1_destroy(_wlZxdgDecorationManagerV1);
            }

            if (_wlZwpPointerGesturesV1 != nullptr) {
                zwp_pointer_gestures_v1_destroy(_wlZwpPointerGesturesV1);
            }

            if (_wlPresentation != nullptr) {
                wp_presentation_destroy(_wlPresentation);
            }

            if (_wlZwpLinuxDmabufV1 != nullptr) {
                zwp_linux_dmabuf_v1_destroy(_wlZwpLinuxDmabufV1);
            }

            if (_wlShm != nullptr) {
                wl_shm_destroy(_wlShm);
            }

            if (_wlZwpRelativePointerManagerV1 != nullptr) {
                zwp_relative_pointer_manager_v1_destroy(_wlZwpRelativePointerManagerV1);
            }

            if (_wlDrm != nullptr) {
                wl_drm_destroy(_wlDrm);
            }

            if (_wlXdgActivationV1 != nullptr) {
                xdg_activation_v1_destroy(_wlXdgActivationV1);
            }

            if (_wlZwpLinuxDmabufFeedbackV1 != nullptr) {
                zwp_linux_dmabuf_feedback_v1_destroy(_wlZwpLinuxDmabufFeedbackV1);
            }

            xdg_wm_base_destroy(_xdgWmBase);
            wl_compositor_destroy(_wlCompositor);
            wl_registry_destroy(_wlRegistry);
            wl_display_flush(_wlDisplay);
            wl_display_disconnect(_wlDisplay);
        }

        uint32_t WaylandImplementation::AddRef() const
        {
            Core::InterlockedIncrement(_refCount);
            return Core::ERROR_NONE;
        }

        uint32_t WaylandImplementation::Release() const
        {
            uint32_t result = Core::ERROR_NONE;

            if (Core::InterlockedDecrement(_refCount) == 0) {
                delete this;
                result = Core::ERROR_DESTRUCTION_SUCCEEDED;
            }

            return (result);
        }

        /**
         * This function registers various Wayland interfaces and binds them to the corresponding objects.
         *
         * @param registry A pointer to a Wayland registry object, which is used to track global objects and
         * their interfaces.
         * @param name The identifier for the interface being registered.
         * @param iface A string representing the name of the interface being registered.
         * @param version The version number of the interface being registered.
         */
        void WaylandImplementation::RegisterInterface(struct wl_registry* registry, uint32_t name, const char* iface, uint32_t version)
        {
            TRACE(Trace::Backend, ("Received interface: %s v%d", iface, version));

            if (::strcmp(iface, wl_compositor_interface.name) == 0) {
                _wlCompositor = static_cast<wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 4));
            } else if (::strcmp(iface, wl_seat_interface.name) == 0) {
                _input.onRegister(registry, name, iface, version);
            } else if (::strcmp(iface, xdg_wm_base_interface.name) == 0) {
                _xdgWmBase = static_cast<xdg_wm_base*>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
                xdg_wm_base_add_listener(_xdgWmBase, &xdgWmBaseListener, NULL);
            } else if (::strcmp(iface, zxdg_decoration_manager_v1_interface.name) == 0) {
                _wlZxdgDecorationManagerV1 = static_cast<zxdg_decoration_manager_v1*>(wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, 1));
            } else if (::strcmp(iface, zwp_pointer_gestures_v1_interface.name) == 0) {
                _wlZwpPointerGesturesV1 = static_cast<zwp_pointer_gestures_v1*>(wl_registry_bind(registry, name, &zwp_pointer_gestures_v1_interface, version < 3 ? version : 3));
            } else if (::strcmp(iface, wp_presentation_interface.name) == 0) {
                _wlPresentation = static_cast<wp_presentation*>(wl_registry_bind(registry, name, &wp_presentation_interface, 1));
                wp_presentation_add_listener(_wlPresentation, &presentationListener, this);
            } else if (::strcmp(iface, zwp_linux_dmabuf_v1_interface.name) == 0 && version >= 4) {
                _wlZwpLinuxDmabufV1 = static_cast<zwp_linux_dmabuf_v1*>(wl_registry_bind(registry, name, &zwp_linux_dmabuf_v1_interface, 4));
#ifdef USE_LEGACY_WAYLAND
                zwp_linux_dmabuf_v1_add_listener(_wlZwpLinuxDmabufV1, &linuxDmabufV1Listener, this);
#else
                if (_wlZwpLinuxDmabufV1) {
                    _wlZwpLinuxDmabufFeedbackV1 = zwp_linux_dmabuf_v1_get_default_feedback(_wlZwpLinuxDmabufV1);
                    zwp_linux_dmabuf_feedback_v1_add_listener(_wlZwpLinuxDmabufFeedbackV1, &linuxDmabufFeedbackV1Listener, this);
                }
#endif // USE_LEGACY_WAYLAND

            } else if (::strcmp(iface, zwp_relative_pointer_manager_v1_interface.name) == 0) {
                _wlZwpRelativePointerManagerV1 = static_cast<zwp_relative_pointer_manager_v1*>(wl_registry_bind(registry, name, &zwp_relative_pointer_manager_v1_interface, 1));
            } else if (::strcmp(iface, wl_drm_interface.name) == 0) {
                _wlDrm = static_cast<wl_drm*>(wl_registry_bind(registry, name, &wl_drm_interface, 1));
                wl_drm_add_listener(_wlDrm, &drmListener, this);
            } else if (::strcmp(iface, wl_shm_interface.name) == 0) {
                _wlShm = static_cast<wl_shm*>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
                wl_shm_add_listener(_wlShm, &shmListener, this);
            } else if (::strcmp(iface, xdg_activation_v1_interface.name) == 0) {
                _wlXdgActivationV1 = static_cast<xdg_activation_v1*>(wl_registry_bind(registry, name, &xdg_activation_v1_interface, 1));
            }
        }

        /**
         * This function sets the presentation clock value in a Wayland implementation.
         *
         * @param clock The `clock` parameter is an unsigned 32-bit integer that represents the presentation
         * clock value. This function sets the `_presentationClock` member variable to the value of `clock`.
         */
        void WaylandImplementation::PresentationClock(const uint32_t clockType)
        {
            _presentationClock = clockType;

            if (clockType == CLOCK_MONOTONIC || clockType == CLOCK_MONOTONIC_RAW) {
                TRACE(Trace::Information, ("Using monotonic clock for presentation"));
            } else {
                TRACE(Trace::Information, ("Using realtime clock for presentation"));
            }
        }

        /**
         * This function handles the DMA format table by parsing the data and storing the formats and modifiers
         * in a map.
         *
         * @param fd fd is a file descriptor, which is an integer value that identifies an open file in the
         * operating system. In this function, it is used to open and read a file containing DMA format table
         * data.
         * @param size The size parameter is an unsigned 32-bit integer that represents the size of the data to
         * be mapped in bytes. It is used in the mmap() function call to specify the size of the memory region
         * to be mapped.
         */
        void WaylandImplementation::HandleDmaFormatTable(int fd, uint32_t size)
        {
            void* map = ::mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);

            close(fd);

            if (map != MAP_FAILED) {
                const WaylandFormat* formats = reinterpret_cast<WaylandFormat*>(map);
                const uint16_t nFormats(size / sizeof(WaylandFormat));

                // std::string hexData;
                // Core::ToHexString(static_cast<uint8_t*>(map), size, hexData);
                // TRACE(Trace::Backend, ("RAW Data: %s", hexData.c_str()));

                _dmaFormats.clear();

                for (int i = 0; i < nFormats; i++) {
                    const uint32_t format(formats[i].format);

                    // somehow Ubuntu is returning a format of 0, which is invalid
                    // but linear modifier is always valid, so we will just ignore the format
                    const uint64_t modifier((formats[i].modifier != DRM_FORMAT_MOD_INVALID) ? (formats[i].modifier) : DRM_FORMAT_MOD_LINEAR);

                    TRACE(Trace::Backend, ("%d Found DMA format: %s modifier: 0x%" PRIX64, i, DRM::FormatToString(format), modifier));

                    FormatRegister::iterator index = _dmaFormats.find(format);

                    if (index == _dmaFormats.end()) {
                        _dmaFormats.emplace(format, std::vector<uint64_t>({ modifier }));
                    } else {
                        std::vector<uint64_t>& modifiers = index->second;

                        if (std::find(modifiers.begin(), modifiers.end(), modifier) == modifiers.end()) {
                            modifiers.push_back(modifier);
                        }
                    }
                }

                TRACE(Trace::Backend, ("Found %d formats", _dmaFormats.size()));

                ::munmap(map, size);
            }
        }

        /**
         * This function opens the DRM render node for a given DRM device.
         *
         * @param device A pointer to a structure representing a DRM device.
         */
        void WaylandImplementation::OpenDrmRender(drmDevice* device)
        {
            string renderNode = DRM::GetNode(DRM_NODE_RENDER, device);

            if (renderNode.empty()) {
                renderNode = DRM::GetNode(DRM_NODE_PRIMARY, device);
                TRACE(Trace::Information, ("DRM device %p has no render node, trying primary instead", device));
            }

            if (renderNode.empty() == false) {
                OpenDrmRender(renderNode);
            } else {
                TRACE(Trace::Error, ("Could not find render node for drm device %p", device));
            }
        }

        /**
         * The function opens a DRM node for rendering if it exists in a list of available nodes.
         *
         * @param name A string representing the name of the DRM node to be opened.
         */
        void WaylandImplementation::OpenDrmRender(const string& name)
        {
            if (_drmRenderFd == InvalidFileDescriptor) {
                std::vector<std::string> nodes;
                Compositor::DRM::GetNodes(DRM_NODE_RENDER | DRM_NODE_PRIMARY, nodes);

                if (std::find(nodes.begin(), nodes.end(), name) != nodes.end()) {
                    _drmRenderFd = open(name.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
                    ASSERT(_drmRenderFd > 0);
                    TRACE(Trace::Backend, ("Opened DRM node: %s fd: %d", name.c_str(), _drmRenderFd));
                } else {
                    TRACE(Trace::Error, ("Unknown DRM node: %s", name.c_str()));
                }
            } else {
                TRACE(Trace::Information, ("DRM node %s already opened", name.c_str()));
            }
        }

        /**
         * This function dispatches events for a Wayland display and handles errors.
         *
         * @param events A bitfield representing the events that have occurred on the Wayland display
         * file descriptor. It can contain the following flags:
         *
         * @return an integer value, which represents the number of events that were dispatched.
         */
        int WaylandImplementation::Dispatch(const uint32_t events) const
        {

            if ((events & POLLHUP) || (events & POLLERR)) {
                if (events & POLLERR) {
                    TRACE(Trace::Error, ("Failed to read from Wayland display"));
                }
                return 0;
            }

            int count(0);

            if (events & POLLIN) {
                count = wl_display_dispatch(_wlDisplay);
                // TRACE(Trace::Backend, ("Dispatch"));
            }
            if (events & POLLOUT) {
                Flush();
                // TRACE(Trace::Backend, ("Flush display"));
            }
            if (events == 0) {
                count = wl_display_dispatch_pending(_wlDisplay);
                Flush();
            }

            if (count < 0) {
                TRACE(Trace::Error, ("Failed to dispatch Wayland display"));
                return 0;
            }

            return count;
        }

        /**
         * This function creates a Wayland surface and activates it if an activation token is provided.
         *
         * @return a Wayland surface (`wl_surface*`).
         */
        wl_surface* WaylandImplementation::Surface() const
        {
            wl_surface* surface = wl_compositor_create_surface(_wlCompositor);

            if ((_wlXdgActivationV1 != nullptr) && (_activationToken.empty() == false)) {
                xdg_activation_v1_activate(_wlXdgActivationV1, _activationToken.c_str(), surface);
            }

            return surface;
        };

        /**
         * This function returns an xdg_surface and a decoration manager for a given wl_surface in a
         * Wayland implementation.
         *
         * @param surface A Wayland surface that represents a window or a part of a window.
         * @param decoration The `decoration` parameter is a reference to a pointer of type
         * `zxdg_decoration_manager_v1`. It is an output parameter that will be set to either a valid
         * pointer to a `zxdg_decoration_manager_v1` object or `nullptr` depending on whether the `
         *
         * @return an xdg_surface pointer.
         */
        xdg_surface* WaylandImplementation::WindowSurface(wl_surface* surface) const
        {
            xdg_surface* xdgSurface = xdg_wm_base_get_xdg_surface(_xdgWmBase, surface);

            return xdgSurface;
        };

        int WaylandImplementation::RoundTrip() const
        {
            return wl_display_roundtrip(_wlDisplay);
        }

        int WaylandImplementation::Flush() const
        {
            return wl_display_flush(_wlDisplay);
        }

        void WaylandImplementation::Format(const Compositor::PixelFormat& requested, uint32_t& format, uint64_t& modifier) const
        {
            format = DRM_FORMAT_INVALID;
            modifier = DRM_FORMAT_MOD_INVALID;

            for (const auto& localFormat : _dmaFormats) {
                if (localFormat.first == requested.Type()) {
                    format = requested.Type();
                    for (const auto& localModifier : localFormat.second) {
                        for (const auto& requestedModifier : requested.Modifiers()) {
                            if (requestedModifier == localModifier) {
                                modifier = requestedModifier;
                                break;
                            }
                        }
                    }
                    break;
                }
            }
        }

        int WaylandImplementation::RenderNode() const
        {
            return (_drmRenderFd); // this will always be the render node. If not, we have a problem :-)
        }

        Core::ProxyType<IOutput> WaylandImplementation::Output(const string& name, const Exchange::IComposition::Rectangle& rectangle, const Compositor::PixelFormat& format, Compositor::IOutput::ICallback* feedback)
        {
            return (Core::ProxyType<IOutput> (_windows.Instance<Backend::WaylandOutput>(name, *this, name, rectangle, format)));
        }

        struct zxdg_toplevel_decoration_v1* WaylandImplementation::GetWindowDecorationInterface(xdg_toplevel* topLevelSurface) const
        {
            ASSERT(topLevelSurface != nullptr);
            return (_wlZxdgDecorationManagerV1 != nullptr) ? zxdg_decoration_manager_v1_get_toplevel_decoration(_wlZxdgDecorationManagerV1, topLevelSurface) : nullptr;
        }
        struct wp_presentation_feedback* WaylandImplementation::GetFeedbackInterface(wl_surface* surface) const
        {
            ASSERT(surface != nullptr);
            return (_wlPresentation != NULL) ? wp_presentation_feedback(_wlPresentation, surface) : nullptr;
        }

        /**
         * The function imports a DMA buffer into a Wayland buffer.
         *
         * @param api a pointer to the zwp_linux_dmabuf_v1 object, which is used to create the buffer
         * parameters and the buffer itself.
         * @param buffer A pointer to an object of type Exchange::ICompositionBuffer, which represents a buffer
         * used for compositing.
         *
         * @return A `wl_buffer*` is being returned.
         */
        wl_buffer* ImportDmabuf(zwp_linux_dmabuf_v1* api, Exchange::ICompositionBuffer* buffer)
        {
            ASSERT((buffer != nullptr) && (api != nullptr));

            uint32_t modifierHigh = buffer->Modifier() >> 32;
            uint32_t modifierLow = buffer->Modifier() & 0xffffffff;

            struct zwp_linux_buffer_params_v1* params = zwp_linux_dmabuf_v1_create_params(api);

            Exchange::ICompositionBuffer::IIterator* planes = buffer->Acquire(Compositor::DefaultTimeoutMs);
            ASSERT(planes != nullptr);

            uint8_t i(0);

            while (planes->Next() == true) {
                zwp_linux_buffer_params_v1_add(params, planes->Descriptor(), i++, planes->Offset(), planes->Stride(), modifierHigh, modifierLow);
            }

            wl_buffer* wlBuffer = zwp_linux_buffer_params_v1_create_immed(params, buffer->Width(), buffer->Height(), buffer->Format(), 0);

            buffer->Relinquish();

            return wlBuffer;
        }

        static wl_shm_format ConvertDrmFormat(uint32_t drmFormat)
        {
            switch (drmFormat) {
            case DRM_FORMAT_XRGB8888: {
                return WL_SHM_FORMAT_XRGB8888;
            }
            case DRM_FORMAT_ARGB8888: {
                return WL_SHM_FORMAT_ARGB8888;
            }
            default: {
                return static_cast<wl_shm_format>(drmFormat);
            }
            }
        }

        /**
         * Prepare for a SHM allocator, only 1 plane is supported
         * */

        /**
         * The function imports a shared memory buffer using the Wayland shared memory API.
         *
         * @param api A pointer to a Wayland shared memory (wl_shm) object, which is used to create a shared
         * memory pool for the buffer.
         * @param buffer A pointer to an object of type Exchange::ICompositionBuffer, which represents a buffer
         * used for composing images or video frames.
         *
         * @return a pointer to a `wl_buffer` object.
         */
        wl_buffer* ImportShm(wl_shm* api, Exchange::ICompositionBuffer* buffer)
        {
            ASSERT((buffer != nullptr) && (api != nullptr));

            enum wl_shm_format wl_shm_format = ConvertDrmFormat(buffer->Format());

            Exchange::ICompositionBuffer::IIterator* planes = buffer->Acquire(Compositor::DefaultTimeoutMs);
            ASSERT(planes != nullptr);

            uint32_t size(0);

            planes->Next();
            ASSERT(planes->IsValid());

            wl_shm_pool* pool = wl_shm_create_pool(api, planes->Descriptor(), (planes->Stride() * buffer->Height()));

            if (pool == NULL) {
                return NULL;
            }
            wl_buffer* wlBuffer = wl_shm_pool_create_buffer(pool, planes->Offset(), buffer->Width(), buffer->Height(), planes->Stride(), wl_shm_format);

            buffer->Relinquish();

            wl_shm_pool_destroy(pool);

            return wlBuffer;
        }

        /**
         * This function creates a Wayland buffer based on the type of composition buffer provided.
         *
         * @param buffer A pointer to an object that implements the Exchange::ICompositionBuffer
         * interface. This object represents a buffer that can be used for displaying graphics or video
         * content. The function checks the type of the buffer (raw or DMA) and imports it into the
         * Wayland compositor using the appropriate protocol (wl_sh
         *
         * @return a pointer to a `wl_buffer` object.
         */
        wl_buffer* WaylandImplementation::CreateBuffer(Exchange::ICompositionBuffer* buffer) const
        {
            ASSERT(buffer != nullptr);

            wl_buffer* result(nullptr);

            if (buffer->Type() == Exchange::ICompositionBuffer::TYPE_RAW) {
                result = ImportShm(_wlShm, buffer);
            } else if (buffer->Type() == Exchange::ICompositionBuffer::TYPE_DMA) {
                result = ImportDmabuf(_wlZwpLinuxDmabufV1, buffer);
            }

            return result;
        }

    } // namespace Backend

    /**
     * The function "Connector" returns a proxy object for a composition buffer using the Wayland
     * implementation backend.
     *
     * @param name A string representing the name of the output device to connect to.
     * @param rectangle  the area that this connector covers in the composition
     * @param format The format parameter is of type Compositor::PixelFormat and represents the pixel
     * format of the composition buffer. It specifies how the color information of each pixel is stored in
     * memory. Examples of pixel formats include RGBA, BGRA, and ARGB.
     * @param force The "force" parameter is a boolean value that determines whether the output should be
     * forced or not. If set to true, the output will be forced even if it is not available or already in
     * use. If set to false, the function will return an error if the output is not available or already
     *
     * @return A `Core::ProxyType` object that wraps an instance of `IOutput`.
     */

    /* static */ Core::ProxyType<IOutput> CreateBuffer(const string& name, const Exchange::IComposition::Rectangle& rectangle, const Compositor::PixelFormat& format, IOutput::ICallback* feedback)
    {
        static Backend::WaylandImplementation& backend = Core::SingletonType<Backend::WaylandImplementation>::Instance();

        return backend.Output(name, rectangle, format, feedback);
    }
} // namespace Compositor
} // namespace Thunder
