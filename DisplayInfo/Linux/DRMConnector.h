/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#pragma once

#include <core/Portability.h>
#include <tracing/Logging.h>
#include <interfaces/IDisplayInfo.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <string>
#include <vector>

// FIXME:
#ifndef DRM_MODE_HDCP_CONTENT_TYPE0
#define DRM_MODE_HDCP_CONTENT_TYPE0 0
#endif
#ifndef DRM_MODE_HDCP_CONTENT_TYPE1
#define DRM_MODE_HDCP_CONTENT_TYPE1 1
#endif

// FIXME: property strings

namespace WPEFramework {
namespace Plugin {
    namespace Linux {

        bool operator>(const drmModeModeInfo& lhs, const drmModeModeInfo& rhs)
        {
            const int64_t area[2] = {lhs.hdisplay * lhs.vdisplay, rhs.hdisplay * rhs.vdisplay};
            const uint64_t rate[2] = {lhs.vrefresh, rhs.vrefresh};
            const uint64_t clock[2] = {lhs.clock, rhs.clock};

            bool result =    area[0] > area[1]
                          || (area[0] == area[1] && rate[0] > rate[1])
                          || (   area[0] == area[1] && rate[0] == rate[1]
                              && clock[0] > clock[1]
                             );
            return result;
        }
        bool operator==(const drmModeModeInfo& lhs, const drmModeModeInfo& rhs)
        {
            return    lhs.hdisplay == rhs.hdisplay
                   && lhs.vdisplay == rhs.vdisplay
                   && lhs.vrefresh && rhs.vrefresh
                   && lhs.clock == rhs.clock;
        }

        bool operator<(const drmModeModeInfo& lhs, const drmModeModeInfo& rhs)
        {
            return !(lhs == rhs || lhs > rhs);
        }

        class DRMConnector {
        public:
            DRMConnector(const std::string& drmDevice, uint32_t drmConnectorType, bool usePreferredMode = false)
                : _connected{false}
                , _width{0}
                , _height{0}
                , _refreshRate{0}
                , _drmFd{-1}
                , _drmFdPath{""}
                , _connectorId{}
                , _propertyIds{}
            {
// FIXME: PLATFORM (compositor) also opens the node and may do some (non) (atomic) mode setting

                // On no device or no (suitable) connectors the initialization values are used

                std::vector<std::string> list;

                if (!drmDevice.empty()) {
                    list.push_back(drmDevice);
                }

                // Duplicates are allowed
                GetDRMNodesByType(DRM_NODE_PRIMARY, list);
                GetDRMNodesByType(DRM_NODE_RENDER, list);

                for (auto it = list.begin(), end = list.end(); it != end; it++) {
                   _drmFd = open(it->c_str(), O_RDWR | O_CLOEXEC);

                    if (_drmFd < 0) {
                        TRACE(Trace::Error, (_T("Could not open DRM device with path: %s"), it->c_str()));
                    } else {
                        _drmFdPath = *it;

                        drmModeResPtr drmResources = drmModeGetResources(_drmFd);

                        if (drmResources != nullptr) {
                            for (int i = 0; i < drmResources->count_connectors; ++i) {
                                drmModeConnector* connector = drmModeGetConnector(_drmFd, drmResources->connectors[i]);
                                // Only connected conectors are valid
                                if (   connector != nullptr
                                    && connector->connection == DRM_MODE_CONNECTED
                                    && connector->connector_type == drmConnectorType
                                   ) {
                                    InitializeWithConnector(*connector, usePreferredMode);

                                    // Break the loop(s) on valid mode
                                    if (_connected) {
                                        GetDRMConnectorProperties(connector);

                                        break;
                                    }

                                    drmModeFreeConnector(connector);
                                }
                            }

                            // On no device or no (suitable) connectors the initialization values are used
                            drmModeFreeResources(drmResources);
                        }
                    }

                    // Break the loop(s) on valid mode
                    if (_connected) {
                        break;
                    }
                }
            }

            DRMConnector(const DRMConnector&) =  delete;
            DRMConnector& operator=(const DRMConnector&) = delete;

            ~DRMConnector() {
                if (_drmFd < 0) {
                    close(_drmFd);
                    _drmFd=-1;
                }
            }

            bool IsConnected() const { return _connected; }

            uint32_t Height() const { return _height; }

            uint32_t Width() const { return _width; }

            uint32_t RefreshRate() const { return _refreshRate; }

            std::vector<uint8_t> EDID() const { return _edid; }

            bool SetHDCPType(Exchange::IConnectionProperties::HDCPProtectionType value) {
                bool result = false;

                auto it_hdcp = _propertyIds.find("HDCP Content Type");

                if (it_hdcp != _propertyIds.end()) {
                    // HDCP support is available

                    switch (value) {
                    case Exchange::IConnectionProperties::HDCPProtectionType::HDCP_1X           :   result = SetDRMConnectorPropertyValue(_connectorId, it_hdcp->second, DRM_MODE_HDCP_CONTENT_TYPE0);
                                                                                                    break;
                    case Exchange::IConnectionProperties::HDCPProtectionType::HDCP_2X           :   result = SetDRMConnectorPropertyValue(_connectorId, it_hdcp->second, DRM_MODE_HDCP_CONTENT_TYPE1);
                                                                                                    break;
                    case Exchange::IConnectionProperties::HDCPProtectionType::HDCP_AUTO         :   // Try Type 1 first then type 0
                                                                                                    result =    SetDRMConnectorPropertyValue(_connectorId, it_hdcp->second, DRM_MODE_HDCP_CONTENT_TYPE1)
                                                                                                             || SetDRMConnectorPropertyValue(_connectorId, it_hdcp->second, DRM_MODE_HDCP_CONTENT_TYPE0);
                                                                                                    break;
                    case Exchange::IConnectionProperties::HDCPProtectionType::HDCP_Unencrypted  :  __attribute__((fallthrough));
                    default                                                                     :   // No type setting
                                                                                                    ;
                    }
                }

                if (result) {
                    auto it_prot = _propertyIds.find("Content Protection");

                    if (it_prot != _propertyIds.end()) {
                        // Protection support is available

                        switch (value) {
                        case Exchange::IConnectionProperties::HDCPProtectionType::HDCP_1X           :   __attribute__((fallthrough));
                        case Exchange::IConnectionProperties::HDCPProtectionType::HDCP_2X           :   __attribute__((fallthrough));
                        case Exchange::IConnectionProperties::HDCPProtectionType::HDCP_AUTO         :   __attribute__((fallthrough));
                                                                                                        result = SetDRMConnectorPropertyValue(_connectorId, it_prot->second, DRM_MODE_CONTENT_PROTECTION_DESIRED);
                                                                                                        break;
                        case Exchange::IConnectionProperties::HDCPProtectionType::HDCP_Unencrypted  :   __attribute__((fallthrough));
                        default                                                                     :   result = SetDRMConnectorPropertyValue(_connectorId, it_prot->second, DRM_MODE_CONTENT_PROTECTION_UNDESIRED);
                        }
                    }
                }

                return result;
            }

            bool GetHDCPType(Exchange::IConnectionProperties::HDCPProtectionType& value) const {
                bool result = false;

                auto it_prot = _propertyIds.find("Content Protection");

                if (it_prot != _propertyIds.end()) {
                    // Protection support is available
                    uint64_t prop_val;

                    if (GetDRMConnectorPropertyValue(_connectorId, it_prot->second, prop_val)) {
                        switch (prop_val) {
                        case DRM_MODE_CONTENT_PROTECTION_ENABLED    :   // HDCP type has meaning
                                                                        {
                                                                        auto it_hdcp = _propertyIds.find("HDCP Content Type");

                                                                        result =    it_hdcp != _propertyIds.end()
                                                                                 && GetDRMConnectorPropertyValue(_connectorId, it_hdcp->second, prop_val);

                                                                        if (result) {
                                                                            switch (prop_val) {
                                                                            case DRM_MODE_HDCP_CONTENT_TYPE0    :   value = Exchange::IConnectionProperties::HDCPProtectionType::HDCP_1X;
                                                                                                                    break;
                                                                            case DRM_MODE_HDCP_CONTENT_TYPE1    :   value = Exchange::IConnectionProperties::HDCPProtectionType::HDCP_2X;
                                                                                                                    break;
                                                                            default                             :   result = false;
                                                                                                                    ASSERT(result);
                                                                            }
                                                                        }

                                                                        break;
                                                                        }
                        case DRM_MODE_CONTENT_PROTECTION_UNDESIRED  :   // HDCP type is irrelevant
                        case DRM_MODE_CONTENT_PROTECTION_DESIRED    :   // HDCP is still pending, nothing to say just yet
                                                                        value = Exchange::IConnectionProperties::HDCPProtectionType::HDCP_Unencrypted;
                                                                        result = true;
                                                                        break;
                        default                                     :   result = false;
                                                                        ASSERT(result);
                        }
                    }
                }

                return result;
            }

        private:

            void GetDRMNodesByType(uint32_t type, std::vector<std::string>& list) const
            {
                // Arbitrary value
                constexpr uint8_t const DrmMaxDevices = 16;

                drmDevicePtr devices[DrmMaxDevices];

                const int count = drmGetDevices2(0 /* flags */ , &devices[0], static_cast<int>(DrmMaxDevices));

                if (count > 0) {
                    for (int i = 0; i < count; i++) {
                        switch (type) {
                        case DRM_NODE_PRIMARY   :   // card<num>, always created, KMS, privileged
                                                    __attribute__((fallthrough));
                        case DRM_NODE_CONTROL   :   // ControlD<num>, currently unused
                                                    __attribute__((fallthrough));
                        case DRM_NODE_RENDER    :   // Solely for render clients, unprivileged
                                                    {
                                                        if ((1 << type) == ((1 << type) & devices[i]->available_nodes)) {
                                                            list.push_back(std::string(devices[i]->nodes[type]));
                                                        }
                                                        break;
                                                    }
                        case DRM_NODE_MAX       :
                        default                 :   // Unknown (new) node type
                                                    ;
                        }
                    }

                    drmFreeDevices(&devices[0], count);
                }
            }

            bool GetDRMConnectorProperty(VARIABLE_IS_NOT_USED uint32_t connectorId, uint32_t propertyId, drmModePropertyPtr& property) const {
                bool result = false;

                if ( _drmFd > -1
                    && property == nullptr
                   ) {
                    property = drmModeGetProperty(_drmFd, propertyId);

                    result = property != nullptr;
                }

                return result;
            }

            bool GetDRMConnectorPropertyValue(uint32_t connectorId, uint32_t propertyId, uint64_t& value) const {
                bool result = false;

                if (_drmFd > -1) {
                    drmModeConnectorPtr connector = drmModeGetConnector(_drmFd, connectorId);

                    if (connector != nullptr) {
                        for (int index = 0; index < connector->count_props; index++) {
                            result = propertyId == connector->props[index];

                            if (result) {
                                value = connector->prop_values[index]; 
                                break;
                            }
                        }

                        /* void */ drmModeFreeConnector(connector);
                    }
                }

                return result;
            }

            bool SetDRMConnectorPropertyValue(uint32_t connectorId, uint32_t propertyId, uint64_t value) {
                bool result = false;

                if (_drmFd > -1) {
                    result = (drmModeConnectorSetProperty(_drmFd, connectorId, propertyId, value) != -1);
                }

                return result;
            }

            void GetDRMConnectorProperties(drmModeConnectorPtr connector) {
                ASSERT(nullptr != connector);

                auto edid_cb = [this](VARIABLE_IS_NOT_USED uint32_t id, VARIABLE_IS_NOT_USED std::string name, uint32_t length, void* data) -> void {
                    this->_edid.clear();

                    this->_edid.reserve(length);

                    for (uint32_t i = 0; i < length; i++) {
                        this->_edid.push_back(static_cast<uint8_t*>(data)[i]);
                    }
                };

                auto hdr_cb = [this](VARIABLE_IS_NOT_USED uint32_t id, std::string name, VARIABLE_IS_NOT_USED uint32_t length, VARIABLE_IS_NOT_USED void* data) -> void {
                    TRACE(Trace::Information, (_T(name.c_str())));
                };

                auto path_cb = [this](VARIABLE_IS_NOT_USED uint32_t id, std::string name, VARIABLE_IS_NOT_USED uint32_t length, VARIABLE_IS_NOT_USED void* data) -> void {
                    TRACE(Trace::Information, (_T(name.c_str())));
                };

                auto tile_cb = [this](VARIABLE_IS_NOT_USED uint32_t id, std::string name, VARIABLE_IS_NOT_USED uint32_t length, VARIABLE_IS_NOT_USED void* data) -> void {
                    TRACE(Trace::Information, (_T(name.c_str())));
                };


                using blob_callback_t = std::function<void (uint32_t /* id */, std::string /* name */, uint32_t /* data length */, void* /* data */)>;

                static const std::map<std::string, blob_callback_t> blob_cb_lookup = {
                      { "EDID", edid_cb }
                    , { "HDR_OUTPUT_METADATA", hdr_cb }
                    , { "PATH", path_cb }
                    , { "TILE", tile_cb }
                };


                auto hdcp_cb = [this](VARIABLE_IS_NOT_USED uint32_t id, std::string name, uint64_t value, VARIABLE_IS_NOT_USED int count, VARIABLE_IS_NOT_USED struct drm_mode_property_enum* data) -> void {
                    // User space is responsible for polling (change in) this property or monitoring uevent

                    if (std::string("Content Protection") == name) {
                        switch (value) {
                        case DRM_MODE_CONTENT_PROTECTION_UNDESIRED  :   TRACE(Trace::Information, (_T(" (DRM_MODE_CONTENT_PROTECTION_UNDESIRED)")));
                                                                        break;
                        case DRM_MODE_CONTENT_PROTECTION_DESIRED    :   TRACE(Trace::Information, (_T(" (DRM_MODE_CONTENT_PROTECTION_DESIRED)")));
                                                                        break;
                        case DRM_MODE_CONTENT_PROTECTION_ENABLED    :   // Only driver can set this
                                                                        TRACE(Trace::Information, (_T(" (DRM_MODE_CONTENT_PROTECTION_ENABLED)")));
                                                                        break;
                        default                                     :   TRACE(Trace::Information, (_T(" (UNKNOWN)")));
                        }
                    }

                    // User space (then) declares the content type

                    if (std::string("HDCP Content Type") == name) {
                        switch (value) {
                        case DRM_MODE_HDCP_CONTENT_TYPE0    :   // 1.4 or 2.2, but user space can be ignorant of this value
                                                                TRACE(Trace::Information, (_T(" (HDCP Type0)")));
                                                                break;
                        case DRM_MODE_HDCP_CONTENT_TYPE1    :   // Only 2.2 but user space can be ignorant of this value
                                                                TRACE(Trace::Information, (_T(" (HDCP Type1)")));
                                                                break;
                        default                             :   TRACE(Trace::Information, (_T(" (UNKNOWN")));
                        }
                    }
                };


                using enum_callback_t = std::function<void (uint32_t /* id */, std::string /* name */, uint64_t /* current value */, int /* length of list */, struct drm_mode_property_enum* /* list of possible values */)>;

                static const std::map <std::string, enum_callback_t> enum_cb_lookup = {
                      { "Content Protection", hdcp_cb }
                    , { "HDCP Content Type", hdcp_cb }
                };


                for (int i = 0 ; i < connector->count_props ; i++) {
                    drmModePropertyPtr property = nullptr;

                    GetDRMConnectorProperty(connector->connector_id, connector->props[i], property);

                    if (property != nullptr) {
                        /* void */ auto it = _propertyIds.insert({property->name, property->prop_id});
                        ASSERT(it.second);

                        if ((property->flags & DRM_MODE_PROP_BLOB) == DRM_MODE_PROP_BLOB) {
                            auto it = blob_cb_lookup.find(property->name);

                            if (it != blob_cb_lookup.end ()) {
                                drmModePropertyBlobPtr blob = drmModeGetPropertyBlob(_drmFd, connector->prop_values[i]);

                                if (blob != nullptr) {
                                    it->second(connector->props[i], property->name, blob->length, blob->data);
                                    /* void */ drmModeFreePropertyBlob(blob);
                                }
                            }
                        }

                        if ((property->flags & DRM_MODE_PROP_ENUM) == DRM_MODE_PROP_ENUM) {
                            auto it = enum_cb_lookup.find(property->name);

                            if (it != enum_cb_lookup.end()) {
                                it->second(connector->props[i], property->name, connector->prop_values[i], property->count_enums, property->enums);
                            }
                        }

                        /* void */ drmModeFreeProperty(property);
                    }
                }
            }

            void InitializeWithConnector(drmModeConnector& connector, bool usePreferredMode)
            {
                // A preferred mode is not guaranteed to exist among other avaiable modes
                drmModeModeInfoPtr mode = PreferredMode(connector.count_modes, connector.modes);

                mode = (mode != nullptr && !usePreferredMode) ? mode : BestMode(connector.count_modes, connector.modes);

                _connected = (mode != nullptr);
                _width = _connected ? mode->hdisplay : 0;
                _height = _connected ? mode->vdisplay : 0;
                _refreshRate = _connected ? mode->vrefresh : 0;
            }

            drmModeModeInfoPtr BestMode(uint32_t modeCount, drmModeModeInfoPtr modes) const
            {
                drmModeModeInfoPtr result = nullptr;
                for (uint32_t index = 0; index < modeCount; ++index) {
                    if (result == nullptr) {
                        result = modes + index;
                    } else {
                        result = ((*result) > (modes[index])) ? result : (modes + index);
                    }
                }
                return result;
            }

            drmModeModeInfoPtr PreferredMode(uint32_t modeCount, drmModeModeInfoPtr modes) const
            {
                drmModeModeInfoPtr result = nullptr;
                for (uint32_t index = 0; index < modeCount; ++index) {
                    if (((modes[index].type & DRM_MODE_TYPE_PREFERRED)) == DRM_MODE_TYPE_PREFERRED) {
                        result = modes + index;
                    }
                }
                return result;
            }

            bool _connected;
            uint32_t _width;
            uint32_t _height;
            uint32_t _refreshRate;
            std::vector<uint8_t> _edid;

            int _drmFd;
            std::string _drmFdPath;
            uint32_t _connectorId;
            std::map<std::string, uint32_t> _propertyIds;

        };
    }
}
}
