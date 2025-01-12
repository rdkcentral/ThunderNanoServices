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
#include "Connector.h"

#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace Thunder {
    namespace Compositor {
        namespace Backend {
            class Transaction {
            private: 
                class Request {
                public:
                    Request() = delete;
                    Request(Request&&) = delete;
                    Request(const Request&) = delete;
                    Request& operator=(Request&&) = delete;
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
                    void Clear() {
                        // TODO: Discuss with Bram if we can support a clear here..
                    }
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
                        ASSERT(_request != nullptr);

                        if (propertyId == DRM::InvalidIdentifier) {
                            TRACE(Trace::Error, ("ObjectId[%u] property not found.", objectId));
                        }
                        else {
                            int presult(0);

                            TRACE(Trace::Backend, ("ObjectId[%u] Set propertyId %u to %" PRIu64, objectId, propertyId, value));

                            if ((presult = drmModeAtomicAddProperty(_request, objectId, propertyId, value)) >= 0) {
                                result = Core::ERROR_NONE;
                            }
                            else {
                                TRACE(Trace::Error, ("Failed to add atomic DRM property %u: %s", propertyId, strerror(-result)));
                                result = Core::ERROR_GENERAL;
                            }

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
                Transaction() = delete;
                Transaction(Transaction&&) = delete;
                Transaction(const Transaction&) = delete;
                Transaction& operator= (Transaction&&) = delete;
                Transaction& operator= (const Transaction&) = delete;

                Transaction(const int fd, const bool modeSet, void* userData) 
                    : _flags(DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_NONBLOCK)
                    , _modeSet(modeSet)
                    , _request(fd) 
                    , _userData(userData) {
                }
                ~Transaction() = default;

            public:
                bool ModeSet() const {
                    bool lastValue = _modeSet;
                    _modeSet = false;
                    return (lastValue);
                }
                uint32_t Flags() const {
                    return (_flags);
                }
                void Flags(const uint32_t flags) {
                    _flags = flags;
                }
                void Clear() {
                    _request.Clear();
                }
                uint32_t Add (Connector& connector) {
                    const uint32_t connectorId(connector.Properties().Id());
                    const uint32_t crtcId(connector.CrtController().Id());
                    const uint32_t planeId(connector.Plane().Id());

                    TRACE(Trace::Backend, ("Commit for connector: %d, CRTC: %d, Plane: %d, Framebuffer: %d", connectorId, crtcId, planeId, connector.FrameBufferId()));

                    _request.Property(connectorId, connector.Properties().Id(DRM::property::CrtcId), connector.IsEnabled() ? crtcId : 0);

                    if ((connector.IsEnabled()) && (_modeSet == true)) {
                        const drmModeModeInfo* mode = connector.CrtController();
                        _request.Blob(crtcId, connector.CrtController().Id(DRM::property::ModeId), reinterpret_cast<const void*>(mode), sizeof(drmModeModeInfo));
                        Flags(Flags() | DRM_MODE_ATOMIC_ALLOW_MODESET);
                    }

                    _request.Property(crtcId, connector.CrtController().Id(DRM::property::Active), connector.IsEnabled() ? 1 : 0);

                    if (connector.IsEnabled() == true) {
                        constexpr uint32_t x = 0;
                        constexpr uint32_t y = 0;
                        const uint32_t width(connector.Width());
                        const uint32_t height(connector.Height());

                        _request.Property(connectorId, connector.Properties().Id(DRM::property::LinkStatus), DRM_MODE_LINK_STATUS_GOOD);
                        _request.Property(connectorId, connector.Properties().Id(DRM::property::ContentType), DRM_MODE_CONTENT_TYPE_GRAPHICS);

                        _request.Property(planeId, connector.Plane().Id(DRM::property::FbId), connector.FrameBufferId());
                        _request.Property(planeId, connector.Plane().Id(DRM::property::CrtcId), crtcId);

                        _request.Property(planeId, connector.Plane().Id(DRM::property::SrcX), 0);
                        _request.Property(planeId, connector.Plane().Id(DRM::property::SrcY), 0);
                        _request.Property(planeId, connector.Plane().Id(DRM::property::SrcW), uint64_t(width << 16));
                        _request.Property(planeId, connector.Plane().Id(DRM::property::SrcH), uint64_t(height << 16));

                        _request.Property(planeId, connector.Plane().Id(DRM::property::CrtcX), x);
                        _request.Property(planeId, connector.Plane().Id(DRM::property::CrtcY), y);
                        _request.Property(planeId, connector.Plane().Id(DRM::property::CrtcW), width);
                        _request.Property(planeId, connector.Plane().Id(DRM::property::CrtcH), height);

                    } else {
                        _request.Property(planeId, connector.Plane().Id(DRM::property::FbId), 0);
                        _request.Property(planeId, connector.Plane().Id(DRM::property::CrtcId), 0);
                    }

                    return (Core::ERROR_NONE);
                }
                uint32_t Commit() {
                    return(_request.Commit(Flags(), _userData));
                }

            private:
                uint32_t _flags;
                mutable bool _modeSet;
                Request _request;
                void* _userData;
            };
        } // namespace Backend
    } // namespace Compositor
} // namespace Thunder
