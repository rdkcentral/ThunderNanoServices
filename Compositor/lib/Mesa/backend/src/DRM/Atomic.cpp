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

#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace WPEFramework {
namespace Compositor {
namespace Backend {

    class AtomicCrtc : public IOutput {
    private:
        class Request {
        public:
            Request() = delete;
            Request(Request&&) = delete;
            Request(const Request&) = delete;
            Request& operator= (const Request&) = delete;

            Request(const uint32_t /* flags */)
                : _request(drmModeAtomicAlloc()) {
            }
            ~Request() {
                if (_request != nullptr) {
                    drmModeAtomicFree(request);
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
            uint32_t AddProperty(uint32_t objectId, uint32_t propertyId, uint64_t value)
            {
                int result();

                if ((request != nullptr) && (result = drmModeAtomicAddProperty(_request, objectId, propertyId, value) < 0)) {
                    TRACE(Trace::Error, "Failed to add atomic DRM property %u: %s", propertyId, strerror(-request));
                }

                return (result == 0) ? Core::ERROR_NONE : Core::ERROR_GENERIC;
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
                    TRACE(Trace::Error, "Atomic commit failed: %s", strerror(-request));
                }

                return (result == 0) ? Core::ERROR_NONE : Core::ERROR_GENERIC;
            }

        private:
            const uint32_t _flags;
            drmModeAtomicReqPtr* _request
        };

    public:
        AtomicCrtc(AtomicCrtc&&) = delete;
        AtomicCrtc(const AtomicCrtc&) = delete;
        AtomicCrtc& operator= (const AtomicCrtc&) = delete;

        AtomicCrtc() = default;
        ~AtomicCrtc() override = default;

    public:
        void Reevaluate() override
        {
            // _doModeSet = true;
        }

        uint32_t Commit(const int fd, const IConnector* connector, const uint32_t flags, void* userData) override
        {
            ASSERT(connector->drmFd > 0);

            // switch on the Atomic API.
            int setAtomic = drmSetClientCap(connector->CardFd(), DRM_CLIENT_CAP_ATOMIC, 1);
            ASSERT(setAtomic == 0);
 
            ASSERT((data.Flags & ~DRM_MODE_PAGE_FLIP_FLAGS) == 0); // only allow page flip flags

            Core::ProxyType<Request> request = Core::ProxyType<Request>::Create(flags);

            ASSERT(request.operator->() == nullptr);

            request->AddProperty(connectorId, "CRTC_ID", crtcId);

            request->Commit(fd, userData);

            return Core::ERROR_NONE;
        }

    private:
        const IConnector* _connector;
        uint32_t _mode_id;
        uint32_t _gamma_lut;
    }; // class AtomicCrtc

    /* static */ IOutput& IOutput::Instance()
    {
        static AtomicCrtc& output = Core::SingletonType<AtomicCrtc>::Instance();
        return output;
    }


} // namespace Backend
} // namespace Compositor
} // namespace WPEFramework
