#pragma once

#include <core/Portability.h>
#include <tracing/Logging.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <string>
#include <vector>

namespace WPEFramework {
namespace Plugin {
    namespace AMLogic {

        bool operator<(const drmModeModeInfo& lhs, const drmModeModeInfo& rhs)
        {
            return (lhs.hdisplay * lhs.vdisplay) < (rhs.hdisplay * rhs.vdisplay);
        }

        bool operator>(const drmModeModeInfo& lhs, const drmModeModeInfo& rhs)
        {
            return !(lhs < rhs);
        }

        class DRMConnector {
        public:
            DRMConnector(const std::string& drmDevice, uint32_t drmConnectorType, bool usePreferredMode = false)
                : _connected(false)
                , _width(0)
                , _height(0)
                , _refreshRate(0)
            {
                int drmFd = open(drmDevice.c_str(), O_RDWR | O_CLOEXEC);
                if (drmFd < 0) {
                    TRACE(Trace::Error, (_T("Could not open DRM device with path: %s"), drmDevice.c_str()));
                } else {
                    drmModeResPtr drmResources = drmModeGetResources(drmFd);

                    for (int i = 0; i < drmResources->count_connectors; ++i) {
                        drmModeConnector* connector = drmModeGetConnector(drmFd, drmResources->connectors[i]);
                        if (connector && connector->connector_type == drmConnectorType) {
                            InitializeWithConnector(*connector, usePreferredMode);
                        }
                        drmModeFreeConnector(connector);
                    }

                    drmModeFreeResources(drmResources);
                }
                close(drmFd);
            }

            bool IsConnected() { return this->_connected; }

            uint32_t Height() { return this->_height; }

            uint32_t Width() { return this->_width; }

            uint32_t RefreshRate() { return this->_refreshRate; }

        private:
            void InitializeWithConnector(drmModeConnector& connector, bool usePreferredMode)
            {

                drmModeModeInfoPtr mode = usePreferredMode ? PreferredMode(connector.count_modes, connector.modes)
                                                           : BestMode(connector.count_modes, connector.modes);

                _connected = (mode != nullptr);
                _width = _connected ? mode->hdisplay : 0;
                _height = _connected ? mode->vdisplay : 0;
                _refreshRate = _connected ? mode->vrefresh : 0;
            }

            drmModeModeInfoPtr BestMode(uint32_t modeCount, drmModeModeInfoPtr modes) const
            {
                drmModeModeInfoPtr result = nullptr;
                for (int index = 0; index < modeCount; ++index) {
                    if (result == nullptr) {
                        result = modes + index;
                    } else {
                        result = (*result > modes[index]) ? result : (modes + index);
                    }
                }
                return result;
            }

            drmModeModeInfoPtr PreferredMode(uint32_t modeCount, drmModeModeInfoPtr modes) const
            {
                drmModeModeInfoPtr result = nullptr;
                for (int index = 0; index < modeCount; ++index) {
                    if (modes[index].type & DRM_MODE_TYPE_PREFERRED) {
                        result = modes + index;
                    }
                }
                return result;
            }

            bool _connected;
            uint32_t _width;
            uint32_t _height;
            uint32_t _refreshRate;
        };
    }
}
}