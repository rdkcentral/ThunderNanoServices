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

#include "../Trace.h"

#include <IBackend.h>

#include <CompositorTypes.h>
#include <DrmCommon.h>

#include <IAllocator.h>
#include <IOutput.h>
#include <interfaces/IComposition.h>

#include <ResolutionConversion.h>
// #define __GBM__

#ifdef __cplusplus
extern "C" {
#endif
#include <drm_fourcc.h>
#include <gbm.h>
#include <libudev.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#ifdef __cplusplus
}
#endif

MODULE_NAME_ARCHIVE_DECLARATION

// clang-format off
namespace WPEFramework {
    ENUM_CONVERSION_BEGIN(Exchange::IComposition::ScreenResolution)
    { Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown,   _TXT("Unknown") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_480i,      _TXT("480i") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_480p,      _TXT("480p") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_576i,      _TXT("576i") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_576p,      _TXT("576p") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_576p50Hz,  _TXT("576p50") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_720p,      _TXT("720p") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_720p50Hz,  _TXT("720p50") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_1080i,     _TXT("1080i") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_1080i25Hz, _TXT("1080i25") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_1080i50Hz, _TXT("1080i50") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_1080p,     _TXT("1080p") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_1080p24Hz, _TXT("1080p24") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_1080p25Hz, _TXT("1080p25") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_1080p30Hz, _TXT("1080p30") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_1080p50Hz, _TXT("1080p50") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_1080p60Hz, _TXT("1080p60") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_2160p30Hz, _TXT("2160p30") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_2160p50Hz, _TXT("2160p50") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_2160p60Hz, _TXT("2160p60") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_4320p30Hz, _TXT("4320p30") },
    { Exchange::IComposition::ScreenResolution::ScreenResolution_4320p60Hz, _TXT("4320p60") },
    ENUM_CONVERSION_END(Exchange::IComposition::ScreenResolution)
}
// clang-format on

using namespace WPEFramework;

namespace Compositor {
namespace Backend {
    constexpr char drmDevicesRootPath[] = "/dev/dri/";
    constexpr int InvalidFd = -1;

    class DRM : virtual public Interfaces::IBackend {
    public:
        class ConnectorImplementation : virtual public Interfaces::IBuffer {
        public:
            ConnectorImplementation() = delete;
            ConnectorImplementation(WPEFramework::Core::ProxyType<DRM> backend, const std::string& connector, const WPEFramework::Exchange::IComposition::ScreenResolution resolution, const Compositor::PixelFormat format, const bool forceResolution)
                : _referenceCount(0)
                , _backend(backend)
                , _connectorId(_backend->FindConnectorId(connector))
                , _buffer()
                , _refreshRate(RefreshRateFromResolution(resolution))
                , _forceOutputResolution(forceResolution)
            {
                TRACE(Trace::Backend, ("Connector %p for Id=%d", this, _connectorId));

                ASSERT(_connectorId != 0);

                WPEFramework::Core::ProxyType<Compositor::Interfaces::IAllocator> allocator = _backend->Allocator();

                _buffer = allocator->Create(WidthFromResolution(resolution), HeightFromResolution(resolution), format);

                ASSERT(_buffer.IsValid() == true);

                allocator.Release();
                TRACE(Trace::Backend, ("Constructed connector %p for id=%d buffer=%p", this, _connectorId, _buffer.operator->()));

                AddRef();
            }

            ~ConnectorImplementation()
            {
                // _backend->DestroyFrameBuffer(_frameBufferId);

                _buffer.Release();
                _backend.Release();

                TRACE(Trace::Backend, ("Connector %p Destroyed", this));
            }

            /**
             * @brief Interfaces::IBuffer implementation
             *
             */
            void AddRef() const override
            {
                WPEFramework::Core::InterlockedIncrement(_referenceCount);
            }

            uint32_t Release() const override
            {
                ASSERT(_referenceCount > 0);
                uint32_t result(WPEFramework::Core::ERROR_NONE);

                if (WPEFramework::Core::InterlockedDecrement(_referenceCount) == 0) {
                    delete this;
                    result = WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
                }

                return result;
            }

            uint32_t Identifier() const override
            {
                return _connectorId;
            }

            IIterator* Planes(const uint32_t timeoutMs) override
            {
                ASSERT(_buffer.IsValid() == true);
                return _buffer->Planes(timeoutMs);
            }

            uint32_t Completed(const bool dirty) override
            {
                ASSERT(_buffer.IsValid() == true);
                return _buffer->Completed(dirty);
            }

            void Render() override
            {
                ASSERT(_buffer.IsValid() == true);

                if (IsConnected() == true) {
                    _backend->PageFlip(*this);
                }
            }

            uint32_t Width() const override
            {
                ASSERT(_buffer.IsValid() == true);
                return _buffer->Width();
            }
            uint32_t Height() const override
            {
                ASSERT(_buffer.IsValid() == true);
                return _buffer->Height();
            }
            uint32_t Format() const
            {
                ASSERT(_buffer.IsValid() == true);
                return _buffer->Format();
            }
            uint64_t Modifier() const override
            {
                ASSERT(_buffer.IsValid() == true);
                return _buffer->Modifier();
            }

            uint32_t ConnectorId() const
            {
                return _connectorId;
            };

            uint32_t CrtcId() const
            {
                return _crtcId;
            };

        private:
            void SetPreferredMode()
            {
            }

            bool IsConnected() const
            {
                return (_crtcId > 0);
            }

        private:
            WPEFramework::Core::ProxyType<DRM> _backend;
            mutable uint32_t _referenceCount;

            const uint32_t _connectorId;
            uint32_t _crtcId;
            uint32_t _encoderId;

            WPEFramework::Core::ProxyType<Compositor::Interfaces::IBuffer> _buffer;

            std::shared_ptr<IOutput> _output;
            const uint16_t _refreshRate;
            const bool _forceOutputResolution;
        };

    private:
        friend class ConnectorImplementation;

        class Monitor : virtual public WPEFramework::Core::IResource {
        public:
            Monitor() = delete;
            Monitor(const DRM& parent)
                : _parent(parent)
            {
            }
            ~Monitor() = default;

            handle Descriptor() const override
            {
                return (_parent.Descriptor());
            }
            uint16_t Events() override
            {
                return (POLLIN | POLLPRI);
            }
            void Handle(const uint16_t events) override
            {
                if (((events & POLLIN) != 0) || ((events & POLLPRI) != 0)) {
                    _parent.DrmEventHandler();
                }
            }

        private:
            const DRM& _parent;
        };

        using CommitRegister = std::unordered_map<uint32_t, ConnectorImplementation*>;

    public:
        DRM(const DRM&) = delete;
        DRM& operator=(const DRM&) = delete;

        DRM() = delete;

        DRM(const std::string& gpuNode)
            : _adminLock()
            , _cardFd(OpenGPU(gpuNode))
            , _monitor(*this)
            , _output(IOutput::IOutputFactory::Instance()->Create())
            , _allocator(Compositor::Interfaces::IAllocator::Instance(_cardFd))
        {
            ASSERT(_cardFd > 0);

            int setUniversalPlanes(drmSetClientCap(_cardFd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1));
            ASSERT(setUniversalPlanes == 0);

            WPEFramework::Core::ResourceMonitor::Instance().Register(_monitor);

            char* name = drmGetDeviceNameFromFd2(_cardFd);
            drmVersion* version = drmGetVersion(_cardFd);
            TRACE(WPEFramework::Trace::Information, ("Initialized DRM backend for %s (%s)", name, version->name));
            drmFreeVersion(version);
        }

        virtual ~DRM()
        {
            char* name = drmGetDeviceNameFromFd2(_cardFd);
            drmVersion* version = drmGetVersion(_cardFd);
            TRACE(WPEFramework::Trace::Information, ("Destructing DRM backend for %s (%s)", name, version->name));
            drmFreeVersion(version);

            WPEFramework::Core::ResourceMonitor::Instance().Unregister(_monitor);

            if (_cardFd) {
                close(_cardFd);
            }
        }

        uint64_t Capability(const uint64_t capability) const
        {
            uint64_t value(0);

            int result(drmGetCap(_cardFd, capability, &value));

            if (result != 0) {
                TRACE(WPEFramework::Trace::Error, (("Failed to query capability 0x%016" PRIx64), capability));
            }

            return (result == 0) ? value : 0;
        }

        void FinishPageFlip(uint32_t crtc_id)
        {
            CommitRegister::iterator index(_pendingCommits.find(crtc_id));

            ConnectorImplementation* connector(nullptr);

            if (index != _pendingCommits.end()) {
                connector = index->second;
                _pendingCommits.erase(index);
            }

            ASSERT(crtc_id == connector->CrtcId());

            connector->Completed(false);
            connector->Release();
        }

    private:
        inline uint32_t CreateFrameBuffer(ConnectorImplementation& connector)
        {
            uint32_t framebufferId = 0;

            bool modifierSupported = (Capability(DRM_CAP_ADDFB2_MODIFIERS) == 1) ? true : false;

            TRACE(Trace::Backend, ("Framebuffers with modifiers %s", modifierSupported ? "supported" : "unsupported"));

            ASSERT(_cardFd > 0);

            uint16_t nPlanes(0);

            std::array<uint32_t, 4> handles = { 0, 0, 0, 0 };
            std::array<uint32_t, 4> pitches = { 0, 0, 0, 0 };
            std::array<uint32_t, 4> offsets = { 0, 0, 0, 0 };
            std::array<uint64_t, 4> modifiers;

            modifiers.fill(connector.Modifier());

            Compositor::Interfaces::IBuffer::IIterator* planes = connector.Planes(10);
            ASSERT(planes != nullptr);

            while ((planes->Next() == true) && (planes->IsValid() == true)) {
                ASSERT(planes->IsValid() == true);

                Compositor::Interfaces::IBuffer::IPlane* plane = planes->Plane();
                ASSERT(plane != nullptr);

                if (drmPrimeFDToHandle(_cardFd, plane->Accessor(), &handles[nPlanes]) != 0) {
                    TRACE(WPEFramework::Trace::Error, ("Failed to acquirer drm handle from plane accessor"));
                    CloseDrmHandles(handles);
                    break;
                }

                pitches[nPlanes] = plane->Stride();
                offsets[nPlanes] = plane->Offset();

                ++nPlanes;
            }

            if (modifierSupported && connector.Modifier() != DRM_FORMAT_MOD_INVALID) {

                if (drmModeAddFB2WithModifiers(_cardFd, connector.Width(), connector.Height(), connector.Format(), handles.data(), pitches.data(), offsets.data(), modifiers.data(), &framebufferId, DRM_MODE_FB_MODIFIERS) != 0) {
                    TRACE(WPEFramework::Trace::Error, ("Failed to allocate drm framebuffer with modifiers"));
                }
            } else {
                if (connector.Modifier() != DRM_FORMAT_MOD_INVALID && connector.Modifier() != DRM_FORMAT_MOD_LINEAR) {
                    TRACE(WPEFramework::Trace::Error, ("Cannot import drm framebuffer with explicit modifier 0x%" PRIX64, connector.Modifier()));
                    return 0;
                }

                int ret = drmModeAddFB2(_cardFd, connector.Width(), connector.Height(), connector.Format(), handles.data(), pitches.data(), offsets.data(), &framebufferId, 0);

                if (ret != 0 && connector.Format() == DRM_FORMAT_ARGB8888 /*&& nPlanes == 1*/ && offsets[0] == 0) {
                    TRACE(WPEFramework::Trace::Error, ("Failed to allocate drm framebuffer (%s), falling back to old school drmModeAddFB", strerror(-ret)));

                    uint32_t depth = 32;
                    uint32_t bpp = 32;

                    if (drmModeAddFB(_cardFd, connector.Width(), connector.Height(), depth, bpp, pitches[0], handles[0], &framebufferId) != 0) {
                        TRACE(WPEFramework::Trace::Error, ("Failed to allocate a drm framebuffer the old school way..."));
                    }

                } else if (ret != 0) {
                    TRACE(WPEFramework::Trace::Error, ("Failed to allocate a drm framebuffer..."));
                }
            }

            CloseDrmHandles(handles);

            // just unlock and go, we still need to draw something,.
            connector.Completed(false);

            TRACE(Trace::Backend, ("Framebuffer %u allocated for buffer %p", framebufferId, &connector));

            return framebufferId;
        }

        inline void DestroyFrameBuffer(uint32_t frameBufferId)
        {
            if ((frameBufferId > 0) && (drmModeRmFB(_cardFd, frameBufferId) != 0)) {
                TRACE(WPEFramework::Trace::Error, ("Failed to destroy framebuffer %u", frameBufferId));
            }
        }

        inline uint32_t PageFlip(ConnectorImplementation& connector)
        {
            connector.AddRef();

            uint32_t frameBufferId = CreateFrameBuffer(connector);
            ASSERT(frameBufferId != 0);

            _pendingCommits.emplace(std::piecewise_construct,
                std::forward_as_tuple(connector.CrtcId()),
                std::forward_as_tuple(&connector));

            _output->Commit(_cardFd, connector.CrtcId(), connector.ConnectorId(), frameBufferId, DRM_MODE_PAGE_FLIP_EVENT, this);

            return 0;
        }

        inline int DrmEventHandler() const
        {
            drmEventContext eventContext = {
                .version = 3,
                .page_flip_handler2 = PageFlipHandler,
            };

            if (drmHandleEvent(_cardFd, &eventContext) != 0) {
                TRACE(WPEFramework::Trace::Error, ("Failed to handle drm event"));
            }
            return 1;
        }

        static void PageFlipHandler(int cardFd, unsigned seq, unsigned tv_sec, unsigned tv_usec, unsigned crtc_id, void* userData)
        {
            ASSERT(userData != nullptr);

            DRM* backend = reinterpret_cast<DRM*>(userData);

            backend->FinishPageFlip(crtc_id);
        }

        static int OpenGPU(const std::string& gpuNode)
        {
            int fd(InvalidFd);

            ASSERT(drmAvailable() > 0);

            if (drmAvailable() > 0) {

                std::vector<std::string> nodes;

                Compositor::DRM::GetNodes(DRM_NODE_PRIMARY, nodes);

                const auto& it = std::find(nodes.cbegin(), nodes.cend(), gpuNode);

                if (it != nodes.end()) {
                    // The node might be privileged and the call will fail.
                    // Do not close fd with exec functions! No O_CLOEXEC!
                    fd = open(it->c_str(), O_RDWR);
                } else {
                    TRACE_GLOBAL(WPEFramework::Trace::Error, ("Could not find gpu %s", gpuNode.c_str()));
                }
            }

            return fd;
        }

        int Descriptor() const
        {
            return _cardFd;
        }

        uint32_t FindConnectorId(const std::string& connectorName)
        {
            uint32_t connectorId = 0;

            if (_cardFd > 0) {
                drmModeResPtr resources = drmModeGetResources(_cardFd);

                uint8_t i(0);

                for (i = 0; i < resources->count_connectors; i++) {
                    drmModeConnectorPtr connector = drmModeGetConnector(_cardFd, resources->connectors[i]);

                    if (nullptr != connector) {
                        char name[59];
                        int nameLength;
                        nameLength = snprintf(name, sizeof(name), "%s-%u",
                            drmModeGetConnectorTypeName(connector->connector_type), connector->connector_type_id);

                        name[nameLength] = '\0';

                        if (connectorName.compare(name) == 0) {
                            connectorId = connector->connector_id;
                            break;
                        }

                        drmModeFreeConnector(connector);
                    }
                }
            }

            return connectorId;
        }

        void CloseDrmHandles(std::array<uint32_t, 4>& handles)
        {
            for (uint8_t currentIndex = 0; currentIndex < handles.size(); ++currentIndex) {
                if (handles.at(currentIndex) == 0) {
                    continue;
                }

                // If multiple planes share the same BO handle, avoid double-closing it
                bool alreadyClosed = false;

                for (uint8_t previousIndex = 0; previousIndex < currentIndex; ++previousIndex) {
                    if (handles.at(currentIndex) == handles.at(previousIndex)) {
                        alreadyClosed = true;
                        break;
                    }
                }
                if (alreadyClosed == true) {
                    TRACE(Trace::Backend, ("Skipping DRM handle %u, already closed.", handles.at(currentIndex)));
                    continue;
                }

                if (drmCloseBufferHandle(_cardFd, handles.at(currentIndex)) != 0) {
                    TRACE(WPEFramework::Trace::Error, ("Failed to close drm handle %u", handles.at(currentIndex)));
                }
            }

            handles.fill(0);
        }

        WPEFramework::Core::ProxyType<Compositor::Interfaces::IAllocator> Allocator()
        {
            _allocator.AddRef();
            return _allocator;
        }

    private:
        mutable Core::CriticalSection _adminLock;
        Monitor _monitor;
        int _cardFd;
        std::shared_ptr<IOutput> _output;
        WPEFramework::Core::ProxyType<Compositor::Interfaces::IAllocator> _allocator;
        CommitRegister _pendingCommits;

    }; // class DRM

    static void ParseConnectorName(const std::string& name, std::string& card, std::string& connector)
    {
        card.clear();
        connector.clear();

        constexpr char delimiter = '-';

        // Assumption that the connector name is always on of <card id>-<connector type id> e.g. "card0-HDMI-A-1" "card1-DP-2"
        if (name.empty() == false) {
            uint16_t delimiterPosition(name.find(delimiter));
            card = drmDevicesRootPath + name.substr(0, delimiterPosition);
            connector = name.substr((delimiterPosition + 1));
        }

        TRACE_GLOBAL(Trace::Backend, ("Card='%s' ConnectorId='%s'", card.c_str(), connector.c_str()));
    }
} // namespace Backend

WPEFramework::Core::ProxyType<Interfaces::IBuffer> Interfaces::IBackend::Connector(const std::string& connectorName, const WPEFramework::Exchange::IComposition::ScreenResolution resolution, const Compositor::PixelFormat& format, const bool force)
{
    ASSERT(drmAvailable() == 1);

    TRACE_GLOBAL(Trace::Backend, ("Requesting connector '%s' %uhx%uw", connectorName.c_str(), HeightFromResolution(resolution), WidthFromResolution(resolution)));

    std::string gpu;
    std::string connector;

    Backend::ParseConnectorName(connectorName, gpu, connector);

    ASSERT((gpu.empty() == false) && (connector.empty() == false));

    static WPEFramework::Core::ProxyMapType<std::string, Backend::DRM> backends;
    static WPEFramework::Core::ProxyMapType<std::string, Interfaces::IBuffer> gbmConnectors;

    return gbmConnectors.Instance<Backend::DRM::ConnectorImplementation>(connector, backends.Instance<Backend::DRM>(gpu, gpu), connector, resolution, format, force);
}
} // namespace Compositor
