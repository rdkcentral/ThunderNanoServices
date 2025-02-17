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
#include "Connector.h"

#include <DRM.h>
#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace Thunder {
    namespace Compositor {
        namespace Backend {

            class EXTERNAL Transaction {
            public:
                Transaction() = delete;
                Transaction(Transaction&&) = delete;
                Transaction(const Transaction&) = delete;
                Transaction& operator= (Transaction&&) = delete;
                Transaction& operator= (const Transaction&) = delete;

                Transaction(const int fd, const bool modeSet, void* userData) 
                    : _fd(fd)
                    , _flags()
                    , _modeSet(modeSet)
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
                }
                uint32_t Add (Connector& connector) {

                    uint32_t result = Core::ERROR_NONE;

                    ASSERT(connector.CrtController() != nullptr);

                    uint32_t connectorId(connector.Properties().Id());
                    const uint32_t crtcId(connector.CrtController().Id());

                    TRACE_GLOBAL(Trace::Information, ("Commit for connector: %d , CRTC: %d", connectorId, crtcId));

                    int drmResult(0);

                    if ((connector.IsEnabled()) && (ModeSet() == true)) {

                        const drmModeModeInfo* mode(nullptr);
                        uint32_t dpms = DRM_MODE_DPMS_OFF;

                        mode = connector.CrtController();
                        dpms = DRM_MODE_DPMS_ON;

                        if ((drmResult = drmModeConnectorSetProperty(_fd, connectorId, connector.Properties().Id(Compositor::DRM::property::Dpms), dpms)) != 0) {
                            TRACE_GLOBAL(Trace::Error, ("Failed setting DPMS to %s for connector %d: [%d] %s", connector.IsEnabled() ? "on" : "off", connectorId, drmResult, strerror(errno)));
                            result = Core::ERROR_GENERAL;
                        }
                        else {
                            constexpr uint32_t X = 0;
                            constexpr uint32_t Y = 0;

                            /*
                                * Use the same mode as the previous operation on the CRTC and specified connector(s)
                                * New framebuffer Id, x, and y properties will set at vblank.
                                */
                            if ((drmResult = drmModeSetCrtc(_fd, crtcId, connector.FrameBufferId(), X, Y, &connectorId, 1, const_cast<drmModeModeInfoPtr>(mode)) != 0)) {
                                TRACE_GLOBAL(Trace::Error, ("Failed to set CRTC: %d: [%d] %s", crtcId, drmResult, strerror(errno)));
                                result = Core::ERROR_INCOMPLETE_CONFIG;
                            }
                        }
                    }

                    /*
                    * clear cursor image
                    */

                    if ((result == Core::ERROR_NONE) && (drmResult = drmModeSetCursor(_fd, crtcId, 0, 0, 0)) != 0) {
                        TRACE_GLOBAL(Trace::Error, ("Failed to clear cursor: [%d] %s", drmResult, strerror(errno)));
                        result = Core::ERROR_GENERAL;
                    }
                    else if ((drmResult = drmModePageFlip(_fd, crtcId, connector.FrameBufferId(), DRM_MODE_PAGE_FLIP_EVENT, _userData)) != 0) {
                        TRACE_GLOBAL(Trace::Error, ("Page flip failed: [%d] %s", drmResult, strerror(errno)));
                        result = Core::ERROR_GENERAL;
                    }
                    return (result);
                }
                uint32_t Commit() {
                    return (Core::ERROR_NONE);
                }

            private:
                int _fd;
                uint32_t _flags;
                mutable bool _modeSet;
                void* _userData;
            };
            
        } // namespace Backend
    } // namespace Compositor
} // namespace Thunder
