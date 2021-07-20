#include "ModeSet.h"

#include <vector>
#include <list>
#include <string>
#include <cassert>
#include <limits>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <mutex>

#include <drm/drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

namespace WPEFramework {

static constexpr uint8_t DrmMaxDevices()
{
    // Just an arbitrary choice
    return 16;
}

static void GetNodes(uint32_t type, std::vector<std::string>& list)
{
    drmDevicePtr devices[DrmMaxDevices()];

    static_assert(sizeof(DrmMaxDevices()) <= sizeof(int));
    static_assert(std::numeric_limits<decltype(DrmMaxDevices())>::max() <= std::numeric_limits<int>::max());

    int device_count = drmGetDevices2(0 /* flags */, &devices[0], static_cast<int>(DrmMaxDevices()));

    if (device_count > 0)
    {
        for (int i = 0; i < device_count; i++)
        {
            switch (type)
            {
                case DRM_NODE_PRIMARY   :   // card<num>, always created, KMS, privileged
                case DRM_NODE_CONTROL   :   // ControlD<num>, currently unused
                case DRM_NODE_RENDER    :   // Solely for render clients, unprivileged
                                            {
                                                if ((1 << type) == (devices[i]->available_nodes & (1 << type)))
                                                {
                                                    list.push_back( std::string(devices[i]->nodes[type]) );
                                                }
                                                break;
                                            }
                case DRM_NODE_MAX       :
                default                 :   // Unknown (new) node type
                                        ;
            }
        }

        drmFreeDevices(&devices[0], device_count);
    }
}

static uint32_t GetConnectors(int fd, uint32_t type)
{
    uint32_t bitmask = 0;

    drmModeResPtr resources = drmModeGetResources(fd);

    if(nullptr != resources)
    {
        for (int i = 0; i < resources->count_connectors; i++)
        {
            drmModeConnectorPtr connector = drmModeGetConnector(fd, resources->connectors[i]);

            if(nullptr != connector)
            {
                if ((type == connector->connector_type) && (DRM_MODE_CONNECTED == connector->connection))
                {
                    bitmask = bitmask | (1 << i);
                }

                drmModeFreeConnector(connector);
            }
        }

        drmModeFreeResources(resources);
    }

    return bitmask;
}

static uint32_t GetCRTCS(int fd, bool valid)
{
    uint32_t bitmask = 0;

    drmModeResPtr resources = drmModeGetResources(fd);

    if(nullptr != resources)
    {
        for(int i = 0; i < resources->count_crtcs; i++)
        {
            drmModeCrtcPtr crtc = drmModeGetCrtc(fd, resources->crtcs[i]);

            if(nullptr != crtc)
            {
                bool currentSet = (crtc->mode_valid == 1);

                if(valid == currentSet)
                {
                    bitmask = bitmask | (1 << i);
                }
                drmModeFreeCrtc(crtc);
            }
        }

        drmModeFreeResources(resources);
    }

    return bitmask;
}

static bool FindProperDisplay(int fd, uint32_t& crtc, uint32_t& encoder, uint32_t& connector, uint32_t& fb)
{
    bool found = false;

    assert(fd != -1);

    // Only connected connectors are considered
    uint32_t connectorMask = GetConnectors(fd, DRM_MODE_CONNECTOR_HDMIA);

    // All CRTCs are considered for the given mode (valid / not valid)
    uint32_t crtcs = GetCRTCS(fd, true);

    drmModeResPtr resources = drmModeGetResources(fd);

    if(nullptr != resources)
    {
        int32_t connectorIndex = 0;

        while ((found == false) && (connectorIndex < resources->count_connectors))
        {
            if ((connectorMask & (1 << connectorIndex)) != 0)
            {
                drmModeConnectorPtr connectors = drmModeGetConnector(fd, resources->connectors[connectorIndex]);

                if(nullptr != connectors)
                {
                    int32_t encoderIndex = 0;

                    while ((found == false) && (encoderIndex < connectors->count_encoders))
                    {
                        drmModeEncoderPtr encoders = drmModeGetEncoder(fd, connectors->encoders[encoderIndex]);

                        if (nullptr != encoders)
                        {
                            uint32_t  matches = (encoders->possible_crtcs & crtcs);
                            uint32_t* pcrtc   = resources->crtcs;

                            while ((found == false) && (matches > 0))
                            {
                                if ((matches & 1) != 0)
                                {
                                    drmModeCrtcPtr modePtr = drmModeGetCrtc(fd, *pcrtc);

                                    if(nullptr != modePtr)
                                    {
                                        // A viable set found
                                        crtc = *pcrtc;
                                        encoder = encoders->encoder_id;
                                        connector = connectors->connector_id;
                                        fb = modePtr->buffer_id;

                                        drmModeFreeCrtc(modePtr);
                                        found = true;
                                    }
                                }
                                matches >>= 1;
                                pcrtc++;
                            }
                            drmModeFreeEncoder(encoders);
                        }
                        encoderIndex++;
                    }
                    drmModeFreeConnector(connectors);
                }
            }
            connectorIndex++;
        }

        drmModeFreeResources(resources);
    }

    return (found);
}

static bool CreateBuffer(int fd, const uint32_t connector, gbm_device*& device, uint32_t& modeIndex, uint32_t& id, struct gbm_bo*& buffer, uint32_t & vrefresh)
{
    assert(fd != -1);

    bool created = false;
    buffer = nullptr;
    modeIndex = 0;
    id = 0;
    device = gbm_create_device(fd);

    if (nullptr != device)
    {
        drmModeConnectorPtr pconnector = drmModeGetConnector(fd, connector);

        if (nullptr != pconnector)
        {
            bool found = false;
            uint32_t index = 0;
            uint64_t area = 0;

            while ( (found == false) && (index < static_cast<uint32_t>(pconnector->count_modes)) )
            {
                uint32_t type = pconnector->modes[index].type;

                // At least one preferred mode should be set by the driver, but dodgy EDID parsing might not provide it
                if (DRM_MODE_TYPE_PREFERRED == (DRM_MODE_TYPE_PREFERRED & type))
                {
                    modeIndex = index;
                    vrefresh = pconnector->modes [modeIndex].vrefresh;

                    // Found a suitable mode; break the loop
                    found = true;
                }
                else if (DRM_MODE_TYPE_DRIVER == (DRM_MODE_TYPE_DRIVER & type))
                {
                    // Calculate screen area
                    uint64_t size = pconnector->modes[index].hdisplay * pconnector->modes[index].vdisplay;
                    if (area < size) {
                        area = size;

                        // Use another selection criterium
                        // Select highest clock and vertical refresh rate

                        if ( (pconnector->modes[index].clock > pconnector->modes[modeIndex].clock) ||
                             ( (pconnector->modes[index].clock == pconnector->modes[modeIndex].clock) && 
                               (pconnector->modes[index].vrefresh > pconnector->modes[modeIndex].vrefresh) ) )
                        {
                            modeIndex = index;
                            vrefresh = pconnector->modes [modeIndex].vrefresh;
                        }
                    }
                }
                index++;
            }

            // A large enough initial buffer for scan out
            struct gbm_bo* bo = gbm_bo_create(
                                  device, 
                                  pconnector->modes[modeIndex].hdisplay,
                                  pconnector->modes[modeIndex].vdisplay,
                                  ModeSet::SupportedBufferType(),
                                  GBM_BO_USE_SCANOUT /* presented on a screen */ | GBM_BO_USE_RENDERING /* used for rendering */);

            drmModeFreeConnector(pconnector);

            if(nullptr != bo)
            {
                // Associate a frame buffer with this bo
                int32_t fb_fd = gbm_device_get_fd(device);

                uint32_t format = gbm_bo_get_format(bo);

                assert (format == DRM_FORMAT_XRGB8888 || format == DRM_FORMAT_ARGB8888);

                uint32_t bpp = gbm_bo_get_bpp(bo);

                int32_t ret = drmModeAddFB(
                                fb_fd, 
                                gbm_bo_get_width(bo), 
                                gbm_bo_get_height(bo), 
                                format != DRM_FORMAT_ARGB8888 ? bpp - 8 : bpp,
                                bpp,
                                gbm_bo_get_stride(bo), 
                                gbm_bo_get_handle(bo).u32, &id);

                if(0 == ret)
                {
                    buffer = bo;
                    created = true;
                }
            }
        }
    }

   return created;
}

ModeSet::ModeSet()
    : _crtc(0)
    , _encoder(0)
    , _connector(0)
    , _mode(0)
    , _vrefresh (0)
    , _device(nullptr)
    , _buffer(nullptr)
    , _fd(-1)
{
}

ModeSet::~ModeSet()
{
    if (_buffer != nullptr) {
        Close();
    }
}

uint32_t ModeSet::Open (const string& /* name */) {
    uint32_t result = Core::ERROR_UNAVAILABLE;

    if (drmAvailable() > 0) {

        std::vector<std::string> nodes;

        GetNodes(DRM_NODE_PRIMARY, nodes);

        std::vector<std::string>::iterator index(nodes.begin());

        while ((index != nodes.end()) && (_fd == -1)) {
            // Select the first from the list
            if (index->empty() == false) {
                // The node might be priviliged and the call will fail.
                // Do not close fd with exec functions! No O_CLOEXEC!
                _fd = open(index->c_str(), O_RDWR); 

                if (_fd >= 0) {
                    bool success = false;

                    printf("Test Card: %s\n", index->c_str());
                    if ( (FindProperDisplay(_fd, _crtc, _encoder, _connector, _fb) == true) && 
                         /* TODO: Changes the original fb which might not be what is intended */
                         (CreateBuffer(_fd, _connector, _device, _mode, _fb, _buffer, _vrefresh) == true) &&
                         (drmSetMaster(_fd) == 0) ) {

                        drmModeConnectorPtr pconnector = drmModeGetConnector(_fd, _connector);

                        /* At least one mode has to be set */
                        if (pconnector != nullptr) {

                            success = (0 == drmModeSetCrtc(_fd, _crtc, _fb, 0, 0, &_connector, 1, &(pconnector->modes[_mode])));
                            drmModeFreeConnector(pconnector);
                        }
                    }
                    if (success == true) {
                        printf("Opened Card: %s\n", index->c_str());
                    }
                    else {
                        Close();
                    }
                }
            }
            index++;
        }
        result = (_fd > 0 ? Core::ERROR_NONE : Core::ERROR_GENERAL);
    }

    return (result);
}

uint32_t ModeSet::Close()
{
    // Destroy the initial buffer if one exists
    if (nullptr != _buffer)
    {
        gbm_bo_destroy(_buffer);
        _buffer = nullptr;
    }

    if (nullptr != _device)
    {
        gbm_device_destroy(_device);
        _device = nullptr;
    }

    if(_fd >= 0) {
        close(_fd);
        _fd = -1;
    }

    _crtc = 0;
    _encoder = 0;
    _connector = 0;

    return (Core::ERROR_NONE);
}

uint32_t ModeSet::Width() const
{
    // Derived from modinfo if CreateBuffer was called prior to this
    uint32_t width = 0;

    if (nullptr != _buffer)
    {
        width = gbm_bo_get_width(_buffer);
    }

    return width;
}

uint32_t ModeSet::Height() const
{
    // Derived from modinfo if CreateBuffer was called prior to this
    uint32_t height = 0;

    if (nullptr != _buffer)
    {
        height = gbm_bo_get_height(_buffer);
    }

    return height;
}

uint32_t ModeSet::RefreshRate () const {
    // Derived from modinfo if CreateBuffer was called prior to this

    return _vrefresh;
}

bool ModeSet::Interlaced () const {
// TODO: For now assume interlaced is always false
    bool _ret = false;

    return _ret;
}

// These created resources are automatically destroyed if gbm_device is destroyed
struct gbm_surface* ModeSet::CreateRenderTarget(const uint32_t width, const uint32_t height) const
{
    struct gbm_surface* result = nullptr;

    if(nullptr != _device)
    {
        result = gbm_surface_create(_device, width, height, SupportedBufferType(), GBM_BO_USE_SCANOUT /* presented on a screen */ | GBM_BO_USE_RENDERING /* used for rendering */);
    }

    return result;
}

void ModeSet::DestroyRenderTarget(struct BufferInfo& buffer)
{
    if (nullptr != buffer._bo) 
    {
        if (static_cast<uint32_t>(~0) == buffer._id) {
            drmModeRmFB(_fd, buffer._id);
        }

        gbm_surface_release_buffer(buffer._surface, buffer._bo);
        buffer._bo = nullptr;
    }

    if (nullptr != buffer._surface)
    {
        DestroyRenderTarget (buffer._surface);
    }
}

void ModeSet::DestroyRenderTarget (struct gbm_surface * surface) const {
    if (surface != nullptr) {
        /* void */ gbm_surface_destroy (surface);
    }

    surface = nullptr;
}

struct gbm_bo * ModeSet::CreateBufferObject (uint32_t const width, uint32_t const height) {
    return _device != nullptr ? gbm_bo_create (_device, width, height, SupportedBufferType (), GBM_BO_USE_SCANOUT /* presented on a screen */ | GBM_BO_USE_RENDERING /* used for rendering */) : nullptr;
}

void ModeSet::DestroyBufferObject (struct gbm_bo * bo) {
    if (bo != nullptr) {
        /* void */ gbm_bo_destroy (bo);
    }
}

using data_t = std::atomic < bool >;

static void PageFlip (int, unsigned int, unsigned int, unsigned int, void* data) {
    assert (data != nullptr);

    if (data != nullptr) {
        data_t * _data = reinterpret_cast <data_t *>  (data);
        * _data = false;
    }
    else {
//        TRACE (Trace::Error, (_T ("Invalid callback data.")));
    }
}

void ModeSet::Swap(struct BufferInfo& buffer) {
    assert (_fd > 0);

    // Lock the a new buffer so we can use it
    buffer._bo = gbm_surface_lock_front_buffer (buffer._surface);

    if (buffer._bo != nullptr) {
        uint32_t width = gbm_bo_get_width(buffer._bo);
        uint32_t height = gbm_bo_get_height(buffer._bo);
        uint32_t format = gbm_bo_get_format (buffer._bo);
        uint32_t bpp = gbm_bo_get_bpp (buffer._bo);
        uint32_t stride = gbm_bo_get_stride (buffer._bo);
        uint32_t handle = gbm_bo_get_handle (buffer._bo).u32;

        if (drmModeAddFB (_fd, width, height, format != DRM_FORMAT_ARGB8888 ? bpp - BPP () + ColorDepth () : bpp, bpp, stride, handle, &(buffer._id)) != 0) {
            buffer._id = 0;

            TRACE (Trace::Error, (_T ("Unable to contruct a frame buffer for scan out.")));
        }
        else {
            static data_t _cdata (true);
            _cdata = true;

            int err = drmModePageFlip (_fd, _crtc, buffer._id, DRM_MODE_PAGE_FLIP_EVENT, &_cdata);
            // Many causes, but the most obvious is a busy resource or a missing drmModeSetCrtc
            // Probably a missing drmModeSetCrtc or an invalid _crtc
            // See ModeSet::Create, not recovering here
            assert (err != -EINVAL);
            assert (err != -EBUSY);

            if (err == 0) {
                // No error
                // Asynchronous, but never called more than once, waiting in scope
                // Use the magic constant here because the struct is versioned!
                drmEventContext context = { .version = 2, . vblank_handler = nullptr, .page_flip_handler = PageFlip };
                struct timespec timeout = { .tv_sec = 1, .tv_nsec = 0 };
                fd_set fds;

                // Going fast could trigger an (unrecoverable) EBUSY
                bool _waiting = _cdata;

                while (_waiting != false) {
                    FD_ZERO(&fds);
                    FD_SET(_fd, &fds);

                    // Race free
                    if ((err = pselect(_fd + 1, &fds, nullptr, nullptr, &timeout, nullptr)) < 0) {
                        // Error; break the loop
                        TRACE (Trace::Error, (_T ("Event processing for page flip failed.")));
                        break;
                    }
                    else if (err == 0) {
                        // Timeout; retry
                        // TODO: add an additional condition to break the loop to limit the 
                        // number of retries, but then deal with the asynchronous nature of 
                        // the callback

                        TRACE (Trace::Information, (_T ("Unable to execute a timely page flip. Trying again.")));
                    }
                    else if (FD_ISSET (_fd, &fds) != 0) {
                        // Node is readable
                        if (drmHandleEvent (_fd, &context) != 0) {
                            // Error; break the loop
                            break;
                        }
                        // Flip probably occured already otherwise it loops again
                    }

                    _waiting = _cdata;

                    if (_waiting != true) {
                        // Do not prematurely remove the FB to prevent an EBUSY
                        static BufferInfo _binfo = {nullptr, nullptr, 0};
                        std::swap (_binfo, buffer);
                    }
                }
            }
            else {
                TRACE (Trace::Error, (_T ("Unable to execute a page flip.")));
            }

            if (_fd <= 0 && buffer._id == 0 && drmModeRmFB(_fd, buffer._id) != 0) {
                TRACE (Trace::Error, (_T ("Unable to remove (old) frame buffer.")));
            }
        }

        if (buffer._surface != nullptr && buffer._bo != nullptr) {
            gbm_surface_release_buffer(buffer._surface, buffer._bo);
            buffer._bo = nullptr;
        }
        else {
            TRACE (Trace::Error, (_T ("Unable to release (old) buffer.")));
        }
    }
    else {
        TRACE (Trace::Error, (_T ("Unable to obtain a buffer to support scan out.")));
    }
}

}
