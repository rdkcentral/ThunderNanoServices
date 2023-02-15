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

#include <CompositorTypes.h>
#include <DrmCommon.h>
#include <IAllocator.h>

#include <compositorbuffer/IBuffer.h>

#include <IOutput.h>

#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <core/core.h>

#if HAVE_GBM_MODIFIERS
#ifndef GBM_MAX_PLANES
#define GBM_MAX_PLANES 4
#endif
#endif

namespace Compositor {
namespace Backend {
    class AtomicCrtc : virtual public IOutput {

        class Request {
        public:
            Request() = delete;

            Request(const uint32_t flags)
                : _request(drmModeAtomicAlloc());

            ~Request()
            {
                if (_request != nullptr) {
                    drmModeAtomicFree(request);
                }
            }

            /**
             * @brief Adds a property and value to an atomic request.
             *
             * @param objectId Object ID of a CRTC, plane, or connector to be modified.
             * @param propertyId Property ID of the property to be modified.
             * @param value The new value for the property.
             * @return uint32_t
             */
            uint32_t AddProperty(uint32_t objectId, uint32_t propertyId, uint64_t value)
            {
                int result();

                if ((request != nullptr) && (result = drmModeAtomicAddProperty(_request, objectId, propertyId, value) < 0)) {
                    TRACE(Tracing::Error, "Failed to add atomic DRM property %u: %s", propertyId, strerror(-request));
                }

                return (result == 0) ? WPEFramework::Core::ERROR_NONE : WPEFramework::Core::ERROR_GENERIC;
            }

            /**
             * @brief
             *
             * @param fd
             * @param userData
             * @return uint32_t
             */
            uint32_t Commit(int fd, void* userData)
            {
                int result();

                if (result = drmModeAtomicCommit(fd, _request, _flags, userData) < 0) {
                    TRACE(Tracing::Error, "Atomic commit failed: %s", strerror(-request));
                }

                return (result == 0) ? WPEFramework::Core::ERROR_NONE : WPEFramework::Core::ERROR_GENERIC;
            }

        private:
            const uint32_t _flags;
            drmModeAtomicReqPtr* _request
        };

    public:
        AtomicCrtc(const IConnector* connector)
            : _connector(connector)
        {
            ASSERT(connector->drmFd > 0);

            // switch on the Atomic API.
            int setAtomic = drmSetClientCap(connector->CardFd(), DRM_CLIENT_CAP_ATOMIC, 1);
            ASSERT(setAtomic == 0);
        }

        ~AtomicCrtc()
        {
        }

        uint32_t Commit(const int fd, const uint32_t crtcId, const uint32_t connectorId, const  uint32_t frameBufferId, const uint32_t flags, void* userData) override
        {
            ASSERT((data.Flags & ~DRM_MODE_PAGE_FLIP_FLAGS) == 0); // only allow page flip flags

            WPEFramework::Core::ProxyType<Request> request = WPEFramework::Core::ProxyType<Request>::Create(flags);

            ASSERT(request.operator->() == nullptr);

            request->AddProperty(connectorId, "CRTC_ID", crtcId);

            request->Commit(fd, userData);

            return WPEFramework::Core::ERROR_NONE;
        }

    private:
        const IConnector* _connector;
        uint32_t _mode_id;
        uint32_t _gamma_lut;
    };

    class AtomicCrtcFactory : virtual public IOutput::IOutputFactory {
    public:
        AtomicCrtcFactory() = default;
        ~AtomicCrtcFactory() = default;

        std::shared_ptr<IOutput> Create(const int connector)
        {
            return std::make_shared<AtomicCrtc>(connector);
        }
    };

    IOutput::IOutputFactory* IOutput::IOutputFactory::Instance()
    {
        static AtomicCrtcFactory backend;
        return (&backend);
    }

} // namespace Backend
} // namespace Compositor