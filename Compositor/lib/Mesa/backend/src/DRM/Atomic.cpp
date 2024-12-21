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
#include "IGpu.h"

#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace Thunder {
namespace Compositor {
    namespace Backend {

        class Atomic : public IGpu {
        private:
            static uint64_t MaxBpc(uint32_t format)
            {
                switch (format) {
                case DRM_FORMAT_XBGR16161616F:
                case DRM_FORMAT_ABGR16161616F:
                case DRM_FORMAT_XBGR16161616:
                case DRM_FORMAT_ABGR16161616:
                    return 16;
                case DRM_FORMAT_XRGB2101010:
                case DRM_FORMAT_ARGB2101010:
                case DRM_FORMAT_XBGR2101010:
                case DRM_FORMAT_ABGR2101010:
                    return 10;
                default:
                    return 8;
                }
            }

            class Request {
            public:
                Request() = delete;
                Request(Request&&) = delete;
                Request(const Request&) = delete;
                Request& operator=(const Request&) = delete;

                Request(const int fd)
                    : _fd(fd)
                    , _request(drmModeAtomicAlloc())
                {
                    ASSERT(_request != nullptr);

                    char* node = drmGetDeviceNameFromFd2(_fd);
                    ASSERT(node != nullptr);

                    TRACE(Trace::Backend, ("New request for %s", node));
                }
                ~Request()
                {
                    for (auto& blobId : _blobs) {
                        drmModeDestroyPropertyBlob(_fd, blobId);
                    }

                    if (_request != nullptr) {
                        drmModeAtomicFree(_request);
                    }
                }

            public:
                /**
                 * @brief Adds a property and value to an atomic request.
                 *
                 * @param objectId Object ID of a CRTC, plane, or connector to be modified.
                 * @param propertyId Property ID of the property to be modified.
                 * @param value The new value for the property.
                 * @return uint32_t
                 */
                uint32_t Property(uint32_t objectId, uint32_t propertyId, uint64_t value)
                {
                    uint32_t result(Core::ERROR_UNAVAILABLE);

                    ASSERT(objectId != DRM::InvalidIdentifier);

                    if (propertyId != DRM::InvalidIdentifier) {
                        int presult(0);

                        TRACE(Trace::Backend, ("ObjectId[%u] Set propertyId %u to %" PRIu64, objectId, propertyId, value));

                        if ((_request != nullptr) && (presult = drmModeAtomicAddProperty(_request, objectId, propertyId, value) < 0)) {
                            TRACE(Trace::Error, ("Failed to add atomic DRM property %u: %s", propertyId, strerror(-result)));
                        }

                        result = (presult == 0) ? Core::ERROR_NONE : Core::ERROR_GENERAL;
                    } else {
                        TRACE(Trace::Error, ("ObjectId[%u] property not found.", objectId));
                    }

                    return result;
                }

                /**
                 * @brief Adds a blob and value to an atomic request.
                 *
                 * @param objectId Object ID of a CRTC, plane, or connector to be modified.
                 * @param propertyId Property ID of the property to be modified.
                 * @param value The new value for the property.
                 * @return uint32_t
                 */
                uint32_t Blob(uint32_t objectId, uint32_t propertyId, const void* data, size_t size)
                {
                    int presult;
                    uint32_t id(0);
                    uint32_t result(Core::ERROR_UNAVAILABLE);

                    if ((presult = drmModeCreatePropertyBlob(_fd, data, size, &id)) < 0) {
                        TRACE(Trace::Error, ("blob failed to create: %s", strerror(-presult)));
                    } else {
                        _blobs.push_back(id);
                        result = Property(objectId, propertyId, id);
                    }

                    return result;
                }

                /**
                 * @brief
                 *
                 * @param fd
                 * @param userData
                 * @return uint32_t
                 */
                uint32_t Commit(uint32_t flags, void* userData)
                {
                    int drmResult;

                    if ((drmResult = drmModeAtomicCommit(_fd, _request, flags, userData)) < 0) {
                        TRACE(Trace::Error, ("Atomic commit failed: [%d] %s", drmResult, strerror(errno)));
                    }

                    return (drmResult == 0) ? Core::ERROR_NONE : Core::ERROR_BAD_REQUEST;
                }

            private:
                const int _fd;
                drmModeAtomicReqPtr _request;
                std::vector<uint32_t> _blobs;
            };

        public:
            Atomic(Atomic&&) = delete;
            Atomic(const Atomic&) = delete;
            Atomic& operator=(const Atomic&) = delete;

            Atomic()
                : _doModeset(true)
            {
            }

            ~Atomic() override = default;

        public:
            uint32_t Commit(const int fd, const std::vector<IConnector*> connectors, void* userData) override
            {
                ASSERT(fd > 0);

                Core::ProxyType<Request> request = Core::ProxyType<Request>::Create(fd);

                uint32_t commitFlags(DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_NONBLOCK);

                for (auto& connector : connectors) {
                    ASSERT(connector != nullptr);
                    ASSERT(connector->CrtController() != nullptr);
                    ASSERT(connector->Plane() != nullptr);

                    const uint32_t ConnectorId(connector->Properties().Id());
                    const uint32_t crtcId(connector->CrtController().Id());
                    const uint32_t planeId(connector->Plane().Id());

                    TRACE(Trace::Backend, ("Commit for connector: %d , CRTC: %d, Plane: %d", ConnectorId, crtcId, planeId));

                    request->Property(ConnectorId, connector->Properties().Id(DRM::property::CrtcId), connector->IsEnabled() ? crtcId : 0);

                    if ((connector->IsEnabled()) && (_doModeset == true)) {
                        const drmModeModeInfo* mode = connector->CrtController();
                        request->Blob(crtcId, connector->CrtController().Id(DRM::property::ModeId), reinterpret_cast<const void*>(mode), sizeof(drmModeModeInfo));
                        commitFlags |= DRM_MODE_ATOMIC_ALLOW_MODESET;
                        _doModeset = false;
                    }

                    request->Property(crtcId, connector->CrtController().Id(DRM::property::Active), connector->IsEnabled() ? 1 : 0);

                    if (connector->IsEnabled() == true) {
                        Core::ProxyType<Thunder::Exchange::ICompositionBuffer> frameBuffer = connector->FrameBuffer();

                        constexpr uint32_t x = 0;
                        constexpr uint32_t y = 0;
                        const uint32_t width(frameBuffer->Width());
                        const uint32_t height(frameBuffer->Height());

                        request->Property(ConnectorId, connector->Properties().Id(DRM::property::LinkStatus), DRM_MODE_LINK_STATUS_GOOD);
                        request->Property(ConnectorId, connector->Properties().Id(DRM::property::ContentType), DRM_MODE_CONTENT_TYPE_GRAPHICS);

                        request->Property(planeId, connector->Plane().Id(DRM::property::FbId), connector->FrameBufferId());
                        request->Property(planeId, connector->Plane().Id(DRM::property::CrtcId), crtcId);

                        request->Property(planeId, connector->Plane().Id(DRM::property::SrcX), 0);
                        request->Property(planeId, connector->Plane().Id(DRM::property::SrcY), 0);
                        request->Property(planeId, connector->Plane().Id(DRM::property::SrcW), uint64_t(width << 16));
                        request->Property(planeId, connector->Plane().Id(DRM::property::SrcH), uint64_t(height << 16));

                        request->Property(planeId, connector->Plane().Id(DRM::property::CrtcX), x);
                        request->Property(planeId, connector->Plane().Id(DRM::property::CrtcY), y);
                        request->Property(planeId, connector->Plane().Id(DRM::property::CrtcW), width);
                        request->Property(planeId, connector->Plane().Id(DRM::property::CrtcH), height);

                        frameBuffer.Release();
                    } else {
                        request->Property(planeId, connector->Plane().Id(DRM::property::FbId), 0);
                        request->Property(planeId, connector->Plane().Id(DRM::property::CrtcId), 0);
                    }
                }

                uint32_t result = request->Commit(commitFlags, userData);

                return result;
            }

        private:
            bool _doModeset;

        }; // class Atomic

        /* static */ IGpu& IGpu::Instance()
        {
            static Atomic& output = Core::SingletonType<Atomic>::Instance();
            return output;
        }
    } // namespace Backend
} // namespace Compositor
} // namespace Thunder
