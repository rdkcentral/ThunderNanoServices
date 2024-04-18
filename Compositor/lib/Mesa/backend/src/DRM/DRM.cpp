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

#include <IBackend.h>
#include <IBuffer.h>
#include <interfaces/IComposition.h>

// #define __GBM__

#include <DRM.h>
#include <drm.h>
#include <drm_fourcc.h>
#include <gbm.h>
// #include <libudev.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

MODULE_NAME_ARCHIVE_DECLARATION

// clang-format on
// good reads
// https://docs.nvidia.com/jetson/l4t-multimedia/group__direct__rendering__manager.html
// http://github.com/dvdhrm/docs
// https://drmdb.emersion.fr
//
namespace WPEFramework {

namespace Compositor {

    namespace Backend {

        constexpr TCHAR drmDevicesRootPath[] = _T("/dev/dri/");

        Identifier FindConnectorId(const int fd, const string& connectorName)
        {
            Identifier connectorId(InvalidIdentifier);

            if (fd > 0) {
                drmModeResPtr resources = drmModeGetResources(fd);

                if (resources != nullptr) {
                    for (uint8_t i = 0; i < resources->count_connectors; i++) {
                        drmModeConnectorPtr connector = drmModeGetConnector(fd, resources->connectors[i]);

                        if (nullptr != connector) {
                            char name[59];
                            int nameLength;
                            nameLength = snprintf(name, sizeof(name), "%s-%u", drmModeGetConnectorTypeName(connector->connector_type), connector->connector_type_id);
                            name[nameLength] = '\0';

                            if (connectorName.compare(name) == 0) {
                                connectorId = connector->connector_id;
                                break;
                            }

                            drmModeFreeConnector(connector);
                        }
                    }

                    drmModeFreeResources(resources);
                }
            }

            return connectorId;
        }

        std::string GetGPUNode(const string& connectorName)
        {
            std::string file;
            std::vector<std::string> nodes;

            Compositor::DRM::GetNodes(DRM_NODE_PRIMARY, nodes);

            for (const auto& node : nodes) {
                int fd = ::open(node.c_str(), O_RDWR);

                if (FindConnectorId(fd, connectorName) != InvalidIdentifier) {
                    file = node;
                }

                ::close(fd);

                if (file.empty() == false) {
                    break;
                }
            }

            return file;
        }
        class DRM {
        public:
            struct FrameBuffer {
                Core::ProxyType<Exchange::ICompositionBuffer> data;
                Compositor::Identifier identifier;
            };

            class ConnectorImplementation : public Exchange::ICompositionBuffer, public IOutput::IConnector {
            public:
                ConnectorImplementation() = delete;
                ConnectorImplementation(ConnectorImplementation&&) = delete;
                ConnectorImplementation(const ConnectorImplementation&) = delete;
                ConnectorImplementation& operator=(ConnectorImplementation&&) = delete;
                ConnectorImplementation& operator=(const ConnectorImplementation&) = delete;

                ConnectorImplementation(const string& connector, VARIABLE_IS_NOT_USED const Exchange::IComposition::Rectangle& rectangle, const Compositor::PixelFormat format, Compositor::ICallback* callback)
                    : _backend(_backends.Instance<Backend::DRM>(GetGPUNode(connector), GetGPUNode(connector)))
                    , _connectorId(FindConnectorId(_backend->Descriptor(), connector))
                    , _ctrController(InvalidIdentifier)
                    , _primaryPlane(InvalidIdentifier)
                    , _bufferId(InvalidIdentifier)
                    , _buffer()
                    , _connectorProperties()
                    , _crtcProperties()
                    , _drmModeStatus()
                    , _dpmsPropertyId(InvalidIdentifier)
                    , _callback(callback)
                    , _format(format)
                {
                    ASSERT(_connectorId != InvalidIdentifier);

                    GetPropertyIds(_connectorId, DRM_MODE_OBJECT_CONNECTOR, _connectorProperties);

                    _dpmsPropertyId = Compositor::DRM::GetPropertyId(_connectorProperties, "DPMS");

                    if (Scan() == false) {
                        TRACE(Trace::Backend, ("Connector %s is not in a valid state", connector.c_str()));
                    } else {
                        ASSERT(_ctrController != InvalidIdentifier);
                        ASSERT(_primaryPlane != InvalidIdentifier);
                        ASSERT(_buffer.IsValid());
                        ASSERT(_bufferId != InvalidIdentifier);

                        if (callback != nullptr) {
                            char* node = drmGetDeviceNameFromFd2(_backend->Descriptor());

                            if (node) {
                                callback->Display(Identifier(), node);
                                free(node);
                            }
                        }

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
                    return _buffer->Identifier();
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
                uint32_t Format() const override
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

                Compositor::ICallback* Callback() const
                {
                    return _callback;
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

                    TRACE(Trace::Backend, ("Found %d connectors", drmModeResources->count_connectors));
                    TRACE(Trace::Backend, ("Found %d encoders", drmModeResources->count_encoders));
                    TRACE(Trace::Backend, ("Found %d CRTCs", drmModeResources->count_crtcs));
                    TRACE(Trace::Backend, ("Found %d planes", drmModePlaneResources->count_planes));

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

                            drmModePlanePtr plane(nullptr);
                            drmModeCrtcPtr crtc(nullptr);

                            ASSERT(encoder->crtc_id != Compositor::InvalidIdentifier);
                            TRACE(Trace::Information, ("Start looking for crtc_id: %d", encoder->crtc_id));

                            for (uint8_t c = 0; c < drmModeResources->count_crtcs; c++) {

                                if (drmModeResources->crtcs[c] == encoder->crtc_id) {
                                    crtc = drmModeGetCrtc(_backend->Descriptor(), drmModeResources->crtcs[c]);
                                    break;
                                }
                            }

                            /* Ensure the CRTC is active. */
                            ASSERT(crtc != nullptr);
                            ASSERT(crtc->crtc_id != 0);
                            /*
                             * On the bananpi, the following ASSERT fired but, although the documentation
                             * suggestst that buffer_id means disconnected, the screen was not disconnected
                             * Hence wht the ASSERT is, until further investigation, commented out !!!
                             * ASSERT(crtc->buffer_id != 0);
                             */

                            /*
                             * The kernel doesn't directly tell us what it considers to be the
                             * single primary plane for this CRTC (i.e. what would be updated
                             * by drmModeSetCrtc), but if it's already active then we can cheat
                             * by looking for something displaying the same framebuffer ID,
                             * since that information is duplicated.
                             */
                            for (uint8_t p = 0; p < drmModePlaneResources->count_planes; p++) {
                                plane = drmModeGetPlane(_backend->Descriptor(), drmModePlaneResources->planes[p]);

                                TRACE(Trace::Backend, ("[PLANE: %" PRIu32 "] CRTC ID %" PRIu32 ", FB ID %" PRIu32, plane->plane_id, plane->crtc_id, plane->fb_id));

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

                            if (_buffer.IsValid()) {
                                _buffer.Release();
                            }

                            if (_bufferId != Compositor::InvalidIdentifier) {
                                Compositor::DRM::DestroyFrameBuffer(_backend->Descriptor(), _bufferId);
                                _bufferId = Compositor::InvalidIdentifier;
                            }

                            _buffer = Compositor::CreateBuffer(
                                _backend->Descriptor(),
                                crtc->width,
                                crtc->height,
                                _format);

                            _bufferId = Compositor::DRM::CreateFrameBuffer(_backend->Descriptor(), _buffer.operator->());
                            // _refreshRate = crtc->mode.vrefresh;

                            plane = drmModeGetPlane(_backend->Descriptor(), _primaryPlane);

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

                /* simple stringification operator to make defines and errorcodes human readable */
#define CASE_TO_STRING(value) \
    case value:               \
        return #value;

                static const char* ObjectSting(const uint32_t object)
                {
                    switch (object) {
                        CASE_TO_STRING(DRM_MODE_OBJECT_CRTC)
                        CASE_TO_STRING(DRM_MODE_OBJECT_CONNECTOR)
                        CASE_TO_STRING(DRM_MODE_OBJECT_PLANE)
                    default:
                        return "Invalid";
                    }
                }
#undef CASE_TO_STRING

                bool GetPropertyIds(const Compositor::Identifier object, const uint32_t type, Compositor::DRM::PropertyRegister& registry)
                {
                    registry.clear();

                    TRACE(Trace::Backend, ("Scanning %s Properties", ObjectSting(type)));

                    drmModeObjectProperties* properties = drmModeObjectGetProperties(_backend->Descriptor(), object, type);

                    if (properties != nullptr) {
                        for (uint32_t i = 0; i < properties->count_props; ++i) {
                            drmModePropertyRes* property = drmModeGetProperty(_backend->Descriptor(), properties->props[i]);

                            TRACE(Trace::Backend, (" - %s [%" PRIu32 "]", property->name, property->prop_id));

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

                Compositor::DRM::PropertyRegister _connectorProperties;
                Compositor::DRM::PropertyRegister _crtcProperties;
                Compositor::DRM::PropertyRegister _planeProperties;

                drmModeModeInfo _drmModeStatus;
                Compositor::Identifier _dpmsPropertyId;

                Compositor::ICallback* _callback;

                static Core::ProxyMapType<string, Backend::DRM> _backends;

                const Compositor::PixelFormat _format;
            };

        private:
            class Monitor : public Core::IResource {
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
                        _parent.DrmEventHandler(Descriptor());
                    }
                }

            private:
                const DRM& _parent;
            };

            using CommitRegister = std::unordered_map<uint32_t, ConnectorImplementation*>;

        public:
            DRM(DRM&&) = delete;
            DRM(const DRM&) = delete;
            DRM& operator=(DRM&&) = delete;
            DRM& operator=(const DRM&) = delete;

            DRM() = delete;

            explicit DRM(const string& gpuNode)
                : _adminLock()
                , _cardFd(Compositor::DRM::OpenGPU(gpuNode))
                , _monitor(*this)
                , _output(IOutput::Instance())
                , _pendingCommits()
            {
                if (_cardFd != Compositor::InvalidFileDescriptor) {
                    if (drmSetMaster(_cardFd) != 0) {
                        TRACE(Trace::Error, ("Could not become DRM master. Error: [%s]", strerror(errno)));
                    }

                    if (drmSetClientCap(_cardFd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1) != 0) {
                        TRACE(Trace::Error, ("Could not set basic information. Error: [%s]", strerror(errno)));
                        close(_cardFd);
                        _cardFd = Compositor::InvalidFileDescriptor;
                    } else {
                        Core::ResourceMonitor::Instance().Register(_monitor);
                    }
                }

                ASSERT(_cardFd != Compositor::InvalidFileDescriptor);
            }
            virtual ~DRM()
            {
                if (_cardFd > 0) {
                    Core::ResourceMonitor::Instance().Unregister(_monitor);

                    if ((drmIsMaster(_cardFd) != 0) && (drmDropMaster(_cardFd) != 0)) {
                        TRACE(Trace::Error, ("Could not drop DRM master. Error: [%s]", strerror(errno)));
                    }

                    if (TRACE_ENABLED(Trace::Information)) {
                        drmVersion* version = drmGetVersion(_cardFd);
                        TRACE(Trace::Information, ("Destructing DRM backend for %s (%s)", drmGetDeviceNameFromFd2(_cardFd), version->name));
                        drmFreeVersion(version);
                    }

                    close(_cardFd);
                }
            }

        public:
            bool IsValid() const
            {
                return (_cardFd > 0);
            }

        private:
            uint32_t PageFlip(ConnectorImplementation& connector)
            {
                _tpageflipstart = Core::Time::Now().Ticks();

                uint32_t result(Core::ERROR_NONE);

                if (connector.CtrControllerId() != Compositor::InvalidIdentifier) {
                    _adminLock.Lock();

                    const auto index(_pendingCommits.find(connector.CtrControllerId()));

                    if (index == _pendingCommits.end()) {
                        connector.AddRef();

                        if ((result = _output.Commit(_cardFd, &connector, DRM_MODE_PAGE_FLIP_EVENT, this)) != Core::ERROR_NONE) {
                            TRACE(Trace::Error, ("DRM Commit failed for CRTC %u", connector.CtrControllerId()));
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

                    if (connector->Callback() != nullptr) {
                        struct timespec presentationTimestamp;

                        presentationTimestamp.tv_sec = sec;
                        presentationTimestamp.tv_nsec = usec * 1000;

                        connector->Callback()->LastFrameTimestamp(connector->Identifier(), Core::Time(presentationTimestamp).Ticks());
                    }

                    connector->Release();
                }
                _adminLock.Unlock();
                const uint64_t tpageflipend(Core::Time::Now().Ticks());
                TRACE(Trace::Frame, ("Pageflip took %" PRIu64 "us", (tpageflipend - _tpageflipstart)));
            }

            static void PageFlipHandlerV2(int cardFd VARIABLE_IS_NOT_USED, unsigned seq VARIABLE_IS_NOT_USED, unsigned sec, unsigned usec, unsigned crtc_id, void* userData)
            {
                ASSERT(userData != nullptr);

                DRM* backend = reinterpret_cast<DRM*>(userData);

                backend->FinishPageFlip(crtc_id, sec, usec);
            }

            int DrmEventHandler(int fd) const
            {
                drmEventContext eventContext;

                eventContext.version = 3;
                eventContext.vblank_handler = nullptr;
                eventContext.page_flip_handler = nullptr;
                eventContext.page_flip_handler2 = PageFlipHandlerV2;
                eventContext.sequence_handler = nullptr;

                if (drmHandleEvent(fd, &eventContext) != 0) {
                    TRACE(Trace::Error, ("Failed to handle drm event"));
                }

                return 1;
            }

            int Descriptor() const
            {
                return _cardFd;
            }

        private:
            mutable Core::CriticalSection _adminLock;
            int _cardFd;
            Monitor _monitor;
            IOutput& _output;
            CommitRegister _pendingCommits;
            uint64_t _tpageflipstart;
        }; // class DRM

        /* static */ Core::ProxyMapType<string, DRM> DRM::ConnectorImplementation::_backends;
    } // namespace Backend

    /* static */ Core::ProxyType<Exchange::ICompositionBuffer> Connector(const string& connectorName, const Exchange::IComposition::Rectangle& rectangle, const Compositor::PixelFormat& format, Compositor::ICallback* callback)
    {
        ASSERT(drmAvailable() == 1);
        ASSERT(connectorName.empty() == false);

        TRACE_GLOBAL(Trace::Backend, ("Requesting connector '%s'", connectorName.c_str()));

        static Core::ProxyMapType<string, Exchange::ICompositionBuffer> gbmConnectors;

        return gbmConnectors.Instance<Backend::DRM::ConnectorImplementation>(connectorName, connectorName, rectangle, format, callback);
    }
} // namespace Compositor
} // WPEFramework
