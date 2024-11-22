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

#ifndef MODULE_NAME
#define MODULE_NAME CompositorDrmScan
#endif

#include <core/core.h>
#include <localtracer/localtracer.h>
#include <messaging/messaging.h>

#include <DRM.h>

#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <libudev.h>

using namespace Thunder;

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace {
/* simple stringification operator to make errorcodes human readable */
#define CASE_TO_STRING(value) \
    case value:         \
        return #value;

static const char* PlaneTypeString(uint64_t type)
{
    switch (type) {
        CASE_TO_STRING(DRM_PLANE_TYPE_OVERLAY)
        CASE_TO_STRING(DRM_PLANE_TYPE_PRIMARY)
        CASE_TO_STRING(DRM_PLANE_TYPE_CURSOR)
    default:
        return "Unknown";
    }
}
#undef CASE_TO_STRING

uint32_t GetProperty(const int cardFd, const uint32_t objectId, const uint32_t propertyId, uint64_t& value)
{
    uint32_t result(Thunder::Core::ERROR_NOT_SUPPORTED);

    drmModeObjectProperties* properties = drmModeObjectGetProperties(cardFd, objectId, DRM_MODE_OBJECT_ANY);

    if (properties != nullptr) {
        std::vector<uint32_t> props;
        std::vector<uint32_t> values;

        for (uint32_t i = 0; i < properties->count_props; ++i) {
            props.emplace_back(properties->props[i]);
            values.emplace_back(properties->prop_values[i]);
        }

        for (uint32_t i = 0; i < properties->count_props; ++i) {
            const uint32_t currentPropertyId(properties->props[i]);

            if (currentPropertyId == propertyId) {
                value = properties->prop_values[i];
                result = Thunder::Core::ERROR_NONE;
                break;
            }
        }

        drmModeFreeObjectProperties(properties);
    }

    return result;
}

bool GetPropertyIds(const int cardFd, const uint32_t objectId, const uint32_t type, Compositor::DRM::PropertyRegister& registry)
{
    registry.clear();

    drmModeObjectProperties* properties = drmModeObjectGetProperties(cardFd, objectId, type);

    if (properties != nullptr) {
        for (uint32_t i = 0; i < properties->count_props; ++i) {
            drmModePropertyRes* property = drmModeGetProperty(cardFd, properties->props[i]);

            registry.emplace(std::piecewise_construct,
                std::forward_as_tuple(property->name),
                std::forward_as_tuple(property->prop_id));

            drmModeFreeProperty(property);
        }

        drmModeFreeObjectProperties(properties);
    } else {
        TRACE_GLOBAL(Thunder::Trace::Error, ("Failed to get DRM object properties"));
        return false;
    }

    return true;
}

void ScanResources(const int cardFd)
{
    drmModeRes* resources = drmModeGetResources(cardFd);
    drmModePlaneRes* planeResources = drmModeGetPlaneResources(cardFd);

    if (!resources)
        return;

    if (!planeResources)
        return;

    uint32_t possible_crtcs(0);

    TRACE_GLOBAL(Thunder::Trace::Information, ("%d CRT controllers reported", resources->count_crtcs));
    TRACE_GLOBAL(Thunder::Trace::Information, ("%d connectors reported", resources->count_connectors));
    TRACE_GLOBAL(Thunder::Trace::Information, ("%d Plane%s reported", planeResources->count_planes, (planeResources->count_planes == 1) ? "" : "s"));

    if (resources->count_crtcs <= 0)
        return;

    if (resources->count_connectors <= 0)
        return;

    if (planeResources->count_planes <= 0)
        return;

    for (uint8_t i = 0; i < resources->count_crtcs; ++i) {
        uint32_t id = resources->crtcs[i];

        Compositor::DRM::PropertyRegister registry;
        GetPropertyIds(cardFd, id, DRM_MODE_OBJECT_CRTC, registry);

        drmModeCrtc* drmCrtc = drmModeGetCrtc(cardFd, id);

        // TRACE_GLOBAL(Thunder::Trace::Information, ("\nCRTC[%u]\nid=%u\nproperties[%d]=%s", i, id, registry.size(), Compositor::DRM::PropertyString(registry, true).c_str()));

        drmModeFreeCrtc(drmCrtc);
    }

    for (int i = 0; i < resources->count_connectors; i++) {
        uint32_t id = resources->connectors[i];

        Compositor::DRM::PropertyRegister registry;
        GetPropertyIds(cardFd, id, DRM_MODE_OBJECT_CONNECTOR, registry);

        drmModeConnectorPtr connector = drmModeGetConnector(cardFd, id);

        possible_crtcs = drmModeConnectorGetPossibleCrtcs(cardFd, connector);

        if (possible_crtcs == 0) {
            TRACE_GLOBAL(Thunder::Trace::Error, ("No CRT controllers found for %s-%u", drmModeGetConnectorTypeName(connector->connector_type), connector->connector_type_id));
        }

        std::vector<Compositor::DRM::Identifier> crtcs;

        for (uint8_t index = 0; index < resources->count_crtcs; index++) {
            if ((possible_crtcs & (1 << index)) != 0) {
                crtcs.emplace_back(resources->crtcs[index]);
            }
        }

        TRACE_GLOBAL(Thunder::Trace::Information, ("\nConnector[%u]\nid=%u\nCRTCs=%s\nproperties[%d]=%s", i, id, Compositor::DRM::IdentifierString(crtcs, true).c_str(), registry.size(), Compositor::DRM::PropertyString(registry, true).c_str()));

        drmModeFreeConnector(connector);
    }

    for (uint8_t i = 0; i < planeResources->count_planes; ++i) {
        uint32_t id = planeResources->planes[i];

        Compositor::DRM::PropertyRegister registry;
        GetPropertyIds(cardFd, id, DRM_MODE_OBJECT_PLANE, registry);

        drmModePlane* drmPlane = drmModeGetPlane(cardFd, id);

        const uint32_t possible_crtcs(drmPlane->possible_crtcs);

        std::vector<Compositor::DRM::Identifier> crtcs;

        for (uint8_t index = 0; index < resources->count_crtcs; index++) {
            if ((possible_crtcs & (1 << index)) != 0) {
                crtcs.emplace_back(resources->crtcs[index]);
            }
        }

        uint64_t planeType;

        auto typeId = registry.find("type");

        Compositor::DRM::Identifier typPropId(0);

        if (typeId != registry.end()) {
            typPropId = typeId->second;
        }

        ASSERT(typPropId != 0);

        GetProperty(cardFd, id, typPropId, planeType);

        TRACE_GLOBAL(Thunder::Trace::Information, ("\nPlane[%u]:\nid=%u\ntype=%s\nCRTCs=%s\nproperties[%d]=%s", i, id, PlaneTypeString(planeType), Compositor::DRM::IdentifierString(crtcs, true).c_str(), registry.size(), Compositor::DRM::PropertyString(registry, true).c_str()));

        drmModeFreePlane(drmPlane);
    }

    drmModeFreePlaneResources(planeResources);
    drmModeFreeResources(resources);
}

bool HasCapability(const int cardFd, const uint64_t option)
{
    uint64_t cap(0);
    int r = drmGetCap(cardFd, option, &cap);
    return ((r == 0) && (cap > 0));
}

uint64_t CapabilityValue(const int cardFd, const uint64_t option)
{
    uint64_t cap;
    int r = drmGetCap(cardFd, option, &cap);
    return (r == 0) ? cap : 0;
}

void CheckCapabilities(const int cardFd)
{
    TRACE_GLOBAL(Thunder::Trace::Information, (" - DRM_CAP_ADDFB2_MODIFIERS: %ssupported value: %" PRIu64, HasCapability(cardFd, DRM_CAP_ADDFB2_MODIFIERS) ? "" : "not ", CapabilityValue(cardFd, DRM_CAP_ADDFB2_MODIFIERS)));
    TRACE_GLOBAL(Thunder::Trace::Information, (" - DRM_CAP_ASYNC_PAGE_FLIP: %ssupported value: %" PRIu64, HasCapability(cardFd, DRM_CAP_ASYNC_PAGE_FLIP) ? "" : "not ", CapabilityValue(cardFd, DRM_CAP_ASYNC_PAGE_FLIP)));
    TRACE_GLOBAL(Thunder::Trace::Information, (" - DRM_CAP_CRTC_IN_VBLANK_EVENT: %ssupported value: %" PRIu64, HasCapability(cardFd, DRM_CAP_CRTC_IN_VBLANK_EVENT) ? "" : "not ", CapabilityValue(cardFd, DRM_CAP_CRTC_IN_VBLANK_EVENT)));
    TRACE_GLOBAL(Thunder::Trace::Information, (" - DRM_CAP_CURSOR_HEIGHT: %ssupported value: %" PRIu64, HasCapability(cardFd, DRM_CAP_CURSOR_HEIGHT) ? "" : "not ", CapabilityValue(cardFd, DRM_CAP_CURSOR_HEIGHT)));
    TRACE_GLOBAL(Thunder::Trace::Information, (" - DRM_CAP_CURSOR_WIDTH: %ssupported value: %" PRIu64, HasCapability(cardFd, DRM_CAP_CURSOR_WIDTH) ? "" : "not ", CapabilityValue(cardFd, DRM_CAP_CURSOR_WIDTH)));
    TRACE_GLOBAL(Thunder::Trace::Information, (" - DRM_CAP_DUMB_BUFFER: %ssupported value: %" PRIu64, HasCapability(cardFd, DRM_CAP_DUMB_BUFFER) ? "" : "not ", CapabilityValue(cardFd, DRM_CAP_DUMB_BUFFER)));
    TRACE_GLOBAL(Thunder::Trace::Information, (" - DRM_CAP_DUMB_PREFERRED_DEPTH: %ssupported value: %" PRIu64, HasCapability(cardFd, DRM_CAP_DUMB_PREFERRED_DEPTH) ? "" : "not ", CapabilityValue(cardFd, DRM_CAP_DUMB_PREFERRED_DEPTH)));
    TRACE_GLOBAL(Thunder::Trace::Information, (" - DRM_CAP_DUMB_PREFER_SHADOW: %ssupported value: %" PRIu64, HasCapability(cardFd, DRM_CAP_DUMB_PREFER_SHADOW) ? "" : "not ", CapabilityValue(cardFd, DRM_CAP_DUMB_PREFER_SHADOW)));
    TRACE_GLOBAL(Thunder::Trace::Information, (" - DRM_CAP_PAGE_FLIP_TARGET: %ssupported value: %" PRIu64, HasCapability(cardFd, DRM_CAP_PAGE_FLIP_TARGET) ? "" : "not ", CapabilityValue(cardFd, DRM_CAP_PAGE_FLIP_TARGET)));
    TRACE_GLOBAL(Thunder::Trace::Information, (" - DRM_CAP_SYNCOBJ: %ssupported value: %" PRIu64, HasCapability(cardFd, DRM_CAP_SYNCOBJ) ? "" : "not ", CapabilityValue(cardFd, DRM_CAP_SYNCOBJ)));
    TRACE_GLOBAL(Thunder::Trace::Information, (" - DRM_CAP_SYNCOBJ_TIMELINE: %ssupported value: %" PRIu64, HasCapability(cardFd, DRM_CAP_SYNCOBJ_TIMELINE) ? "" : "not ", CapabilityValue(cardFd, DRM_CAP_SYNCOBJ_TIMELINE)));
    TRACE_GLOBAL(Thunder::Trace::Information, (" - DRM_CAP_TIMESTAMP_MONOTONIC: %ssupported value: %" PRIu64, HasCapability(cardFd, DRM_CAP_TIMESTAMP_MONOTONIC) ? "" : "not ", CapabilityValue(cardFd, DRM_CAP_TIMESTAMP_MONOTONIC)));
    TRACE_GLOBAL(Thunder::Trace::Information, (" - DRM_CAP_VBLANK_HIGH_CRTC: %ssupported value: %" PRIu64, HasCapability(cardFd, DRM_CAP_VBLANK_HIGH_CRTC) ? "" : "not ", CapabilityValue(cardFd, DRM_CAP_VBLANK_HIGH_CRTC)));
    TRACE_GLOBAL(Thunder::Trace::Information, (" - DRM_CAP_PRIME: %ssupported value: %" PRIu64, HasCapability(cardFd, DRM_CAP_PRIME) ? "" : "not ", CapabilityValue(cardFd, DRM_CAP_PRIME)));
}

void Scan()
{
    std::vector<std::string> nodes;

    Compositor::DRM::GetNodes(DRM_NODE_PRIMARY, nodes);

    for (const auto& node: nodes) {
        int fd = open(node.c_str(), O_RDWR);
        char* name = drmGetDeviceNameFromFd2(fd);
        drmVersion* version = drmGetVersion(fd);
        TRACE_GLOBAL(Thunder::Trace::Information, ("Opened DRM device %s (%s)", name, version->name));
        drmFreeVersion(version);
        int setUniversalPlanes(drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1));

        if (setUniversalPlanes == 0) {
            CheckCapabilities(fd);
            ScanResources(fd);
        }

        close(fd);
    }
}
} // namespace

int main(int /*argc*/, const char* argv[])
{
    Messaging::LocalTracer& tracer = Messaging::LocalTracer::Open();
    Messaging::ConsolePrinter printer(false);

    tracer.Callback(&printer);

    const std::vector<string> modules = {
        "CompositorDrmScan",
    };

    for (auto module : modules) {
        tracer.EnableMessage(module, "", true);
    }

    {
        TRACE_GLOBAL(Thunder::Trace::Information, ("Starting %s build %s", argv[0], __TIMESTAMP__));

        char keyPress;

        do {
            keyPress = toupper(getchar());

            switch (keyPress) {
            case 'Q': {
                break;
            }

            case 'S': {
                Scan();
                break;
            }

            case '\n': {
                break;
            }

            case '?': {
                printf("[S]can for info.\n");
                printf("[Q]uit application.\n");
                break;
            }

            default: {
                printf("Not known. Press '?' for help.\n");
                break;
            }
            }
        } while (keyPress != 'Q');
    }

    TRACE_GLOBAL(Trace::Information, ("Testing Done..."));
    tracer.Close();
    Thunder::Core::Singleton::Dispose();

    return 0;
}
