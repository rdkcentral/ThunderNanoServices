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

#include "../Module.h"
#include "IOutput.h"

#include <IBuffer.h>
#include <interfaces/IComposition.h>

// #define __GBM__

#include <DRM.h>
#include <drm.h>
#include <drm_fourcc.h>
#include <gbm.h>
#include <libudev.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

MODULE_NAME_ARCHIVE_DECLARATION

// clang-format on
// good reads
// https://docs.nvidia.com/jetson/l4t-multimedia/group__direct__rendering__manager.html
// http://github.com/dvdhrm/docs
//
namespace WPEFramework {

namespace Compositor {

    namespace Backend {

        constexpr TCHAR drmDevicesRootPath[] = _T("/dev/dri/");

        class DRM {
        public:
            struct FrameBuffer {
                Core::ProxyType<Exchange::ICompositionBuffer> data;
                Compositor::Identifier identifier;
            };

            class ConnectorImplementation : public Exchange::ICompositionBuffer, public IOutput::IConnector {
            private:
                class DoubleBuffer : public Exchange::ICompositionBuffer {
                public:
                    DoubleBuffer() = delete;
                    DoubleBuffer(DoubleBuffer&&) = delete;
                    DoubleBuffer(const DoubleBuffer&) = delete;
                    DoubleBuffer& operator=(const DoubleBuffer&) = delete;

                    DoubleBuffer(Core::ProxyType<DRM> backend, const Exchange::IComposition::ScreenResolution resolution, const Compositor::PixelFormat format)
                        : _backend(backend)
                        , _buffers()
                        , _backBuffer(1)
                        , _frontBuffer(0)
                    {
                        Compositor::Identifier id = _backend->Descriptor();
                        uint32_t width = Exchange::IComposition::WidthFromResolution(resolution);
                        uint32_t height = Exchange::IComposition::HeightFromResolution(resolution);

                        for (auto& buffer : _buffers) {
                            buffer.data = Compositor::CreateBuffer(id, width, height, format);
                            buffer.identifier = Compositor::DRM::CreateFrameBuffer(id, buffer.data.operator->());
                        }
                    }

                    virtual ~DoubleBuffer() override
                    {
                        for (auto& buffer : _buffers) {
                            Compositor::DRM::DestroyFrameBuffer(_backend->Descriptor(), buffer.identifier);
                            buffer.data.Release();
                        }

                        _backend.Release();
                    }
                    /**
                     * Exchange::ICompositionBuffer implementation
                     */
                    uint32_t Identifier() const override
                    {
                        return _buffers.at(_backBuffer).data->Identifier();
                    }
                    IIterator* Planes(const uint32_t timeoutMs) override
                    {
                        return _buffers.at(_backBuffer).data->Planes(timeoutMs);
                    }
                    uint32_t Completed(const bool dirty) override
                    {
                        return _buffers.at(_backBuffer).data->Completed(dirty);
                    }
                    void Render() override
                    {
                        return _buffers.at(_backBuffer).data->Render();
                    }
                    uint32_t Width() const override
                    {
                        return _buffers.at(_backBuffer).data->Width();
                    }
                    uint32_t Height() const override
                    {
                        return _buffers.at(_backBuffer).data->Height();
                    }
                    uint32_t Format() const
                    {
                        return _buffers.at(_backBuffer).data->Format();
                    }
                    uint64_t Modifier() const override
                    {
                        return _buffers.at(_backBuffer).data->Modifier();
                    }

                    Compositor::Identifier Swap()
                    {
                        std::swap(_backBuffer, _frontBuffer);
                        return _buffers.at(_frontBuffer).identifier;
                    }

                private:
                    Core::ProxyType<DRM> _backend;
                    std::array<FrameBuffer, 2> _buffers;
                    uint8_t _backBuffer;
                    uint8_t _frontBuffer;
                };

            public:
                ConnectorImplementation() = delete;
                ConnectorImplementation(ConnectorImplementation&&) = delete;
                ConnectorImplementation(const ConnectorImplementation&) = delete;
                ConnectorImplementation& operator=(const ConnectorImplementation&) = delete;

                ConnectorImplementation(string gpu, const string& connector, const Exchange::IComposition::ScreenResolution resolution, const Compositor::PixelFormat format, const bool forceResolution)
                    : _backend(_backends.Instance<Backend::DRM>(gpu, gpu))
                    , _connectorId(_backend->FindConnectorId(connector))
                    , _ctrController(InvalidIdentifier)
                    , _primaryPlane(InvalidIdentifier)
                    , _bufferId(InvalidIdentifier)
                    , _buffer()
                    , _refreshRate(Exchange::IComposition::RefreshRateFromResolution(resolution))
                    , _forceOutputResolution(forceResolution)
                    , _connectorProperties()
                    , _crtcProperties()
                    , _drmModeStatus()
                    , _dpmsPropertyId(InvalidIdentifier)
                {
                    ASSERT(_connectorId != InvalidIdentifier);

                    _buffer = Compositor::CreateBuffer(
                        _backend->Descriptor(), 
                        Exchange::IComposition::WidthFromResolution(resolution), 
                        Exchange::IComposition::HeightFromResolution(resolution), 
                        format);

                    _bufferId = Compositor::DRM::CreateFrameBuffer(_backend->Descriptor(), _buffer.operator->());

                    GetPropertyIds(_connectorId, DRM_MODE_OBJECT_CONNECTOR, _connectorProperties);

                    _dpmsPropertyId = Compositor::DRM::GetPropertyId(_connectorProperties, "DPMS");

                    if (Scan() == false) {
                        TRACE(Trace::Backend, ("Connector %s is not in a valid state", connector.c_str()));
                    } else {
                        ASSERT(_ctrController != InvalidIdentifier);
                        ASSERT(_primaryPlane != InvalidIdentifier);

                        TRACE(Trace::Backend, ("Connector %p for Id=%u Crtc=%u, PrimaryPlane=%u", this, _connectorId, _ctrController, _primaryPlane));
                    }
                }

                virtual ~ConnectorImplementation() override
                {
                    _backend.Release();
                    
                    //_buffer->CompositRelease();

                    TRACE(Trace::Backend, ("Connector %p Destroyed", this));
                }

            public:
                /**
                 * Exchange::ICompositionBuffer implementation
                 */
                uint32_t Identifier() const override
                {
                    return _backend->Descriptor(); //_buffer->Identifier();
                }
                IIterator* Planes(const uint32_t timeoutMs) override
                {
                    return _buffer->Planes(timeoutMs);
                }
                uint32_t Completed(const bool dirty) override
                {
                    return _buffer->Completed(dirty);
                }
                void Render() override
                {
                    // _bufferId = _buffer->Swap();

                    // TRACE(Trace::Backend, ("Curent buffer: %u", _bufferId));

                    if (IsConnected() == true) {
                        _backend->PageFlip(*this);
                    }
                }
                uint32_t Width() const override
                {
                    return _buffer->Width();
                }
                uint32_t Height() const override
                {
                    return _buffer->Height();
                }
                uint32_t Format() const
                {
                    return _buffer->Format();
                }
                uint64_t Modifier() const override
                {
                    return _buffer->Modifier();
                }

                Exchange::ICompositionBuffer::DataType Type() const
                {
                    return _buffer->Type();
                }

                /**
                 * IOutput::IConnector implementation
                 */

                bool IsEnabled() const
                {
                    // Todo: this will control switching on or of the display by controlling the Display Power Management Signaling (DPMS)
                    return true;
                }
                Compositor::Identifier ConnectorId() const override
                {
                    return _connectorId;
                }
                Compositor::Identifier CtrControllerId() const override
                {
                    return _ctrController;
                }
                Compositor::Identifier PrimaryPlaneId() const override
                {
                    return _primaryPlane;
                }
                Compositor::Identifier FrameBufferId() const override
                {
                    return _bufferId;
                }
                const drmModeModeInfo& ModeInfo() const override
                {
                    return _drmModeStatus;
                }
                Compositor::Identifier DpmsPropertyId() const override
                {
                    return _dpmsPropertyId;
                }

            private:
                bool IsConnected() const
                {
                    return (_ctrController != Compositor::InvalidIdentifier);
                }

                bool Scan()
                {
                    bool result(false);

                    _ctrController = Compositor::InvalidIdentifier;
                    _primaryPlane = Compositor::InvalidIdentifier;

                    drmModeResPtr drmModeResources = drmModeGetResources(_backend->Descriptor());
                    drmModePlaneResPtr drmModePlaneResources = drmModeGetPlaneResources(_backend->Descriptor());

                    ASSERT(drmModeResources != nullptr);
                    ASSERT(drmModePlaneResources != nullptr);

                    drmModeEncoderPtr encoder = NULL;

                    TRACE(Trace::Backend, ("Found %d DRM CRTCs", drmModeResources->count_crtcs));
                    TRACE(Trace::Backend, ("Found %d DRM planes", drmModePlaneResources->count_planes));

                    for (int c = 0; c < drmModeResources->count_connectors; c++) {
                        drmModeConnectorPtr drmModeConnector = drmModeGetConnector(_backend->Descriptor(), drmModeResources->connectors[c]);

                        ASSERT(drmModeConnector != nullptr);

                        /* Find our drmModeConnector if it's connected*/
                        if ((drmModeConnector->connector_id != _connectorId) && (drmModeConnector->connection != DRM_MODE_CONNECTED)) {
                            continue;
                        }

                        /* Find the encoder (a deprecated KMS object) for this drmModeConnector. */
                        ASSERT(drmModeConnector->encoder_id != Compositor::InvalidIdentifier);

                        const char* name = drmModeGetConnectorTypeName(drmModeConnector->connector_type);
                        TRACE(Trace::Backend, ("Connector %s-%u connected", name, drmModeConnector->connector_type_id));

                        uint32_t _possibleCrtcs = drmModeConnectorGetPossibleCrtcs(_backend->Descriptor(), drmModeConnector);

                        if (_possibleCrtcs == 0) {
                            TRACE(Trace::Error, ("No CRTC possible for id", drmModeConnector->connector_id));
                        }

                        ASSERT(_possibleCrtcs != 0);

                        for (uint8_t e = 0; e < drmModeResources->count_encoders; e++) {
                            if (drmModeResources->encoders[e] == drmModeConnector->encoder_id) {
                                encoder = drmModeGetEncoder(_backend->Descriptor(), drmModeResources->encoders[e]);
                                break;
                            }
                        }

                        if (encoder != nullptr) { /*
                                                   * Find the CRTC currently used by this drmModeConnector. It is possible to
                                                   * use a different CRTC if desired, however unlike the pre-atomic API,
                                                   * we have to explicitly change every object in the routing path.
                                                   */

                            drmModePlanePtr plane = NULL;
                            drmModeCrtcPtr crtc = NULL;

                            ASSERT(encoder->crtc_id != Compositor::InvalidIdentifier);

                            for (uint8_t c = 0; c < drmModeResources->count_crtcs; c++) {
                                if (drmModeResources->crtcs[c] == encoder->crtc_id) {
                                    crtc = drmModeGetCrtc(_backend->Descriptor(), drmModeResources->crtcs[c]);
                                    break;
                                }
                            }

                            /* Ensure the CRTC is active. */
                            ASSERT(crtc != nullptr);
                            ASSERT(crtc->buffer_id != 0);
                            ASSERT(crtc->crtc_id != 0);

                            /*
                             * The kernel doesn't directly tell us what it considers to be the
                             * single primary plane for this CRTC (i.e. what would be updated
                             * by drmModeSetCrtc), but if it's already active then we can cheat
                             * by looking for something displaying the same framebuffer ID,
                             * since that information is duplicated.
                             */
                            for (uint8_t p = 0; p < drmModePlaneResources->count_planes; p++) {
                                plane = drmModeGetPlane(_backend->Descriptor(), drmModePlaneResources->planes[p]);

                                TRACE(Trace::Backend, ("[PLANE: %" PRIu32 "] CRTC ID %" PRIu32 ", FB %" PRIu32, plane->plane_id, plane->crtc_id, plane->fb_id));

                                if ((plane->crtc_id == crtc->crtc_id) && (plane->fb_id == crtc->buffer_id)) {
                                    break;
                                }
                            }

                            ASSERT(plane);

                            _ctrController = crtc->crtc_id;
                            _primaryPlane = plane->plane_id;
                            _drmModeStatus = crtc->mode;

                            uint64_t refresh = ((crtc->mode.clock * 1000000LL / crtc->mode.htotal) + (crtc->mode.vtotal / 2)) / crtc->mode.vtotal;

                            TRACE(Trace::Backend, ("[CRTC:%" PRIu32 ", CONN %" PRIu32 ", PLANE %" PRIu32 "]: active at %ux%u, %" PRIu64 " mHz", crtc->crtc_id, drmModeConnector->connector_id, plane->plane_id, crtc->width, crtc->height, refresh));

                            GetPropertyIds(_ctrController, DRM_MODE_OBJECT_CRTC, _crtcProperties);
                            GetPropertyIds(_primaryPlane, DRM_MODE_OBJECT_PLANE, _planeProperties);

                            drmModeFreeCrtc(crtc);
                            drmModeFreePlane(plane);

                            drmModeFreeEncoder(encoder);
                        } else {
                            TRACE(Trace::Error, ("Failed to get encoder for id %u", drmModeConnector->connector_id));
                        }

                        result = ((_ctrController != Compositor::InvalidIdentifier)
                            && (_primaryPlane != Compositor::InvalidIdentifier));

                        break;
                    }

                    drmModeFreeResources(drmModeResources);
                    drmModeFreePlaneResources(drmModePlaneResources);

                    return result;
                }

                bool GetPropertyIds(const Compositor::Identifier object, const uint32_t type, Compositor::DRM::PropertyRegister& registry)
                {
                    registry.clear();

                    drmModeObjectProperties* properties = drmModeObjectGetProperties(_backend->Descriptor(), object, type);

                    if (properties != nullptr) {
                        for (uint32_t i = 0; i < properties->count_props; ++i) {
                            drmModePropertyRes* property = drmModeGetProperty(_backend->Descriptor(), properties->props[i]);

                            registry.emplace(std::piecewise_construct,
                                std::forward_as_tuple(property->name),
                                std::forward_as_tuple(property->prop_id));

                            drmModeFreeProperty(property);
                        }

                        drmModeFreeObjectProperties(properties);
                    } else {
                        TRACE(Trace::Error, ("Failed to get DRM object properties"));
                        return false;
                    }

                    return true;
                }

            private:
                Core::ProxyType<DRM> _backend;

                const Compositor::Identifier _connectorId;
                Compositor::Identifier _ctrController;
                Compositor::Identifier _primaryPlane;

                Compositor::Identifier _bufferId;
                Core::ProxyType<Exchange::ICompositionBuffer> _buffer;

                // Core::ProxyObject<DoubleBuffer> _buffer;

                const uint16_t _refreshRate;
                const bool _forceOutputResolution;

                Compositor::DRM::PropertyRegister _connectorProperties;
                Compositor::DRM::PropertyRegister _crtcProperties;
                Compositor::DRM::PropertyRegister _planeProperties;

                drmModeModeInfo _drmModeStatus;
                Compositor::Identifier _dpmsPropertyId;

                static Core::ProxyMapType<string, Backend::DRM> _backends;
            };

        private:
            class Monitor : virtual public Core::IResource {
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

            DRM(const string& gpuNode)
                : _adminLock()
                , _cardFd(Compositor::DRM::OpenGPU(gpuNode))
                , _monitor(*this)
                , _output(IOutput::Instance())
                , _pendingCommits()
                , _lastPageFlip(0)
            {
                ASSERT(_cardFd > 0);

                int setUniversalPlanes(drmSetClientCap(_cardFd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1));
                ASSERT(setUniversalPlanes == 0);

                Core::ResourceMonitor::Instance().Register(_monitor);

                char* name = drmGetDeviceNameFromFd2(_cardFd);
                drmVersion* version = drmGetVersion(_cardFd);
                TRACE(Trace::Information, ("Initialized DRM backend for %s (%s)", name, version->name));
                drmFreeVersion(version);
            }

            virtual ~DRM()
            {
                Core::ResourceMonitor::Instance().Unregister(_monitor);

                char* name = drmGetDeviceNameFromFd2(_cardFd);
                drmVersion* version = drmGetVersion(_cardFd);
                TRACE(Trace::Information, ("Destructing DRM backend for %s (%s)", name, version->name));
                drmFreeVersion(version);

                if (_cardFd) {
                    close(_cardFd);
                }
            }

        private:
            uint32_t PageFlip(ConnectorImplementation& connector)
            {
                uint32_t result(Core::ERROR_NONE);

                if (connector.CtrControllerId() != Compositor::InvalidIdentifier) {
                    _adminLock.Lock();

                    const auto index(_pendingCommits.find(connector.CtrControllerId()));

                    if (index == _pendingCommits.end()) {
                        connector.AddRef();

                        if ((result = _output.Commit(_cardFd, &connector, DRM_MODE_PAGE_FLIP_EVENT, this)) != Core::ERROR_NONE) {
                            TRACE(Trace::Error, ("Pageflip failed for %u", connector.CtrControllerId()));
                        } else {
                            _pendingCommits.emplace(std::piecewise_construct,
                                std::forward_as_tuple(connector.CtrControllerId()),
                                std::forward_as_tuple(&connector));

                            TRACE(Trace::Backend, ("Commited pageflip for CRTC %u", connector.CtrControllerId()));
                        }
                    } else {
                        TRACE(Trace::Backend, ("Skipping commit, pageflip is still pending for CRTC %u", connector.CtrControllerId()));
                        result = Core::ERROR_INPROGRESS;
                    }

                    _adminLock.Unlock();
                }

                return result;
            }

            void FinishPageFlip(const Compositor::Identifier crtc, const unsigned sec, const unsigned usec)
            {
                _adminLock.Lock();

                CommitRegister::iterator index(_pendingCommits.find(crtc));

                ConnectorImplementation* connector(nullptr);

                if (index != _pendingCommits.end()) {
                    connector = index->second;
                    _pendingCommits.erase(index);

                    ASSERT(crtc == connector->CtrControllerId());

                    connector->Completed(false);
                    connector->Release();
                }

                const struct timespec presentationTimestamp {
                    .tv_sec = sec, .tv_nsec = usec * 1000
                };

                _lastPageFlip = Core::Time(presentationTimestamp).Ticks();

                _adminLock.Unlock();
            }

            static void PageFlipHandler(int /*cardFd*/, unsigned /*seq*/, unsigned sec, unsigned usec, unsigned crtc_id, void* userData)
            {
                ASSERT(userData != nullptr);

                DRM* backend = reinterpret_cast<DRM*>(userData);

                backend->FinishPageFlip(crtc_id, sec, usec);
            }

            int DrmEventHandler() const
            {
                drmEventContext eventContext = {
                    .version = 3,
                    .vblank_handler = nullptr,
                    .page_flip_handler = nullptr,
                    .page_flip_handler2 = PageFlipHandler,
                    .sequence_handler = nullptr
                };

                if (drmHandleEvent(_cardFd, &eventContext) != 0) {
                    TRACE(Trace::Error, ("Failed to handle drm event"));
                }

                return 1;
            }

            int Descriptor() const
            {
                return _cardFd;
            }

            Identifier FindConnectorId(const string& connectorName)
            {
                ASSERT(_cardFd > 0);

                drmModeResPtr resources = drmModeGetResources(_cardFd);

                ASSERT(resources != nullptr);

                Identifier connectorId(InvalidIdentifier);

                TRACE_GLOBAL(Trace::Backend, ("Lets see if we can find connector: %s", connectorName.c_str()));

                if ((_cardFd > 0) && (resources != nullptr)) {
                    for (uint8_t i = 0; i < resources->count_connectors; i++) {
                        drmModeConnectorPtr connector = drmModeGetConnector(_cardFd, resources->connectors[i]);

                        if (nullptr != connector) {
                            char name[59];
                            int nameLength;
                            nameLength = snprintf(name, sizeof(name), "%s-%u", drmModeGetConnectorTypeName(connector->connector_type), connector->connector_type_id);
                            name[nameLength] = '\0';

                            if (connectorName.compare(name) == 0) {
                                TRACE_GLOBAL(Trace::Backend, ("Found connector %s", name));
                                connectorId = connector->connector_id;
                                break;
                            } else {
                                TRACE_GLOBAL(Trace::Backend, ("Passed out on connector %s", name));
                            }

                            drmModeFreeConnector(connector);
                        }
                    }
                }

                if ((connectorId == InvalidIdentifier)) {
                    TRACE_GLOBAL(Trace::Error, ("Unable to find connector %s", connectorName.c_str()));
                }

                ASSERT(connectorId != InvalidIdentifier);

                return connectorId;
            }

        private:
            mutable Core::CriticalSection _adminLock;
            int _cardFd;
            Monitor _monitor;
            IOutput& _output;
            CommitRegister _pendingCommits;
            uint64_t _lastPageFlip;
        }; // class DRM

        /* static */ Core::ProxyMapType<string, DRM> DRM::ConnectorImplementation::_backends;

        static void ParseConnectorName(const string& name, string& card, string& connector)
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

    /*
       TODO:  ScreenResolution_Unknown for auto setting preferred mode
    */
    /* static */ Core::ProxyType<Exchange::ICompositionBuffer> Connector(const string& connectorName, const Exchange::IComposition::ScreenResolution resolution, const Compositor::PixelFormat& format, const bool force)
    {
        ASSERT(drmAvailable() == 1);

        TRACE_GLOBAL(Trace::Backend, ("Requesting connector '%s'", connectorName.c_str()));

        string gpu;
        string connector;

        Backend::ParseConnectorName(connectorName, gpu, connector);

        ASSERT((gpu.empty() == false) && (connector.empty() == false));

        static Core::ProxyMapType<string, Exchange::ICompositionBuffer> gbmConnectors;

        return gbmConnectors.Instance<Backend::DRM::ConnectorImplementation>(connector, gpu, connector, resolution, format, force);
    }
} // namespace Compositor
} // WPEFramework
