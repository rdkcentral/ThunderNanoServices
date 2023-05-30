#include "ModeSet.h"

#include <vector>
#include <list>
#include <string>
#include <cassert>
#include <limits>
#include <mutex>

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <drm/drm_fourcc.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

#ifdef __cplusplus
}
#endif


// User space, kernel space, and other places do not always agree

static_assert(!narrowing<decltype(drmModeFB::fb_id), uint32_t, true>::value);

static_assert(!narrowing<decltype(drmModeCrtc::crtc_id), uint32_t, true>::value);
static_assert(!narrowing<decltype(drmModeCrtc::x), uint32_t, true>::value);
static_assert(!narrowing<decltype(drmModeCrtc::y), uint32_t, true>::value);

static_assert(!narrowing<decltype(drmModeEncoder::encoder_id), uint32_t, true>::value);

static_assert(!narrowing<decltype(drmModeConnector::connector_id), uint32_t, true>::value);

static_assert(!narrowing<decltype(drmModeFB2::fb_id), uint32_t, true>::value);
static_assert(!narrowing<decltype(drmModeFB2::width), uint32_t, true>::value);
static_assert(!narrowing<decltype(drmModeFB2::height), uint32_t, true>::value);
static_assert(!narrowing<decltype(drmModeFB2::pixel_format), uint32_t, true>::value);
static_assert(!narrowing<decltype(drmModeFB2::modifier), uint64_t, true>::value);
static_assert(!narrowing<remove_pointer<std::decay<decltype(drmModeFB2::handles)>::type>::type, uint32_t, true>::value);
static_assert(!narrowing<remove_pointer<std::decay<decltype(drmModeFB2::pitches)>::type>::type, uint32_t, true>::value);
static_assert(!narrowing<remove_pointer<std::decay<decltype(drmModeFB2::offsets)>::type>::type, uint32_t, true>::value);

static_assert(!narrowing<decltype(drmModeFB2::width), uint32_t, true>::value);

static_assert(!narrowing<decltype(gbm_import_fd_modifier_data::width), uint32_t, true>::value);
static_assert(!narrowing<decltype(gbm_import_fd_modifier_data::height), uint32_t, true>::value);
static_assert(!narrowing<decltype(gbm_import_fd_modifier_data::format), uint32_t, true>::value);

static_assert(!narrowing<remove_pointer<std::decay<decltype(gbm_import_fd_modifier_data::fds)>::type>::type, int, true>::value);
static_assert(!narrowing<remove_pointer<std::decay<decltype(gbm_import_fd_modifier_data::strides)>::type>::type, int, true>::value);
static_assert(!narrowing<remove_pointer<std::decay<decltype(gbm_import_fd_modifier_data::offsets)>::type>::type, int, true>::value);
static_assert(!narrowing<decltype(gbm_import_fd_modifier_data::modifier), uint64_t, true>::value);

static_assert(   std::is_integral<decltype(GBM_MAX_PLANES)>::value
              && GBM_MAX_PLANES == 4
             );

namespace WPEFramework {
    class BufferInfo {
    public:

        BufferInfo() = delete;
        BufferInfo(struct gbm_surface*, struct gbm_bo*, uint32_t);
        ~BufferInfo() = default;

        BufferInfo(const BufferInfo&);
        BufferInfo& operator=(const BufferInfo&);

        BufferInfo(BufferInfo&&);
        BufferInfo& operator=(BufferInfo&&);

        struct gbm_surface*& Surface() { return _surface; };
        struct gbm_bo*& Buffer() { return _bo; };
        uint32_t& Identifier() { return _id; }

    private:

        struct gbm_surface* _surface;
        struct gbm_bo* _bo;
        uint32_t _id;
    };

    BufferInfo::BufferInfo(struct gbm_surface* surface, struct gbm_bo* buffer, uint32_t id)
        : _surface{surface}
        , _bo{buffer}
        , _id{id}
    {
    }

    BufferInfo::BufferInfo(const BufferInfo& other)
        : _surface{ModeSet::GBM::InvalidSurface()}
        , _bo{ModeSet::GBM::InvalidBuffer()}
        , _id{ModeSet::DRM::InvalidFramebuffer()}
    {
        *this = other;
    }

    BufferInfo& BufferInfo::operator=(const BufferInfo& other)
    {
        if (this != &other) {
            _surface = other._surface;
            _bo = other._bo;
            _id = other._id;
        }

        return *this;
    }

    BufferInfo::BufferInfo(BufferInfo&& other)
        : _surface{ModeSet::GBM::InvalidSurface()}
        , _bo{ModeSet::GBM::InvalidBuffer()}
        , _id{ModeSet::DRM::InvalidFramebuffer()}
    {
        *this = std::move(other);
    }

    BufferInfo& BufferInfo::operator=(BufferInfo&& other)
    {
        if (this != &other) {
            _surface = std::move(other._surface);
            _bo = std::move(other._bo);
            _id = std::move(other._id);

            other._surface = ModeSet::GBM::InvalidSurface();
            other._bo = ModeSet::GBM::InvalidBuffer();
            other._id = ModeSet::DRM::InvalidFramebuffer();
        }

        return *this;
    }

    ModeSet::DRM::DRM()
        : _fd{InvalidFileDescriptor()}
        , _crtc{InvalidCrtc()}
        , _encoder{InvalidEncoder()}
        , _connector{InvalidConnector()}
        , _fb{InvalidFramebuffer()}
        , _mode{0}
        , _width{InvalidWidth()}
        , _height{InvalidHeight()}
        , _vrefresh{0}
        , _handle2fb()
    {}

    ModeSet::DRM::~DRM()
    {
        /* bool */ DisableDisplay();
        /* bool */ Close();
    }

    bool ModeSet::DRM::Open(uint32_t type)
    {
        bool result = false; 

        if (   drmAvailable() == 1
            && _fd == InvalidFileDescriptor()
           ) {
            std::vector<std::string> nodes;

            AvailableNodes(type, nodes);

            for (auto it = nodes.begin(), end = nodes.end(); it != end; it++) {
                if (!it->empty()) {
                    _fd = open(it-> c_str(), O_RDWR);

                    result =    _fd != InvalidFileDescriptor()
                             && InitializeDisplayProperties() 
                             && drmSetMaster(_fd) == 0
                             ;

                    if (!result) {
                        /* bool */ Close();
                    } else {
                        break;
                    }
                }
            }
        }

        return result;
    }

    bool ModeSet::DRM::AddFramebuffer(struct gbm_bo* bo)
    {
        bool result = false;

        if (   IsValid()
            && bo != GBM::InvalidBuffer()
           ) {
            const uint32_t handle = gbm_bo_get_handle(bo).u32;

            const uint32_t width = gbm_bo_get_width(bo);
            const uint32_t height = gbm_bo_get_height(bo);

            const uint32_t stride = gbm_bo_get_stride(bo);
            const uint32_t format = gbm_bo_get_format(bo);
            const uint64_t modifier = gbm_bo_get_modifier(bo);

            const uint32_t handles[GBM_MAX_PLANES] = { handle, 0, 0, 0 };
            const uint32_t pitches[GBM_MAX_PLANES] = { stride, 0, 0, 0 };
            const uint32_t offsets[GBM_MAX_PLANES] = { 0, 0, 0, 0 };
            const uint64_t modifiers[GBM_MAX_PLANES] = { modifier, 0, 0, 0 };

            uint32_t fb = InvalidFramebuffer();

            if (   width != InvalidWidth()
                && height != InvalidHeight()
                && format != InvalidFormat()
                && modifier != InvalidModifier()
                && drmModeAddFB2WithModifiers(_fd, width, height, format, &handles[0], &pitches[0], &offsets[0], &modifiers[0], &fb, 0 /* flags */) == 0
                && fb != InvalidFramebuffer()
               ) {
                /* size_type  */ _handle2fb.erase(handle);

                auto it = _handle2fb.insert({handle, fb});
                result = it.second;

                if (!result) {
                    /* bool */ RemoveFramebuffer(bo);
                } else {
                    _fb = fb;
                }
            }
        }

        return result;
    }

    bool ModeSet::DRM::RemoveFramebuffer(struct gbm_bo* bo)
    {
        bool result = false;

        if (   IsValid()
            && bo != ModeSet::GBM::InvalidBuffer()
           ) {
            uint32_t handle = gbm_bo_get_handle(bo).u32;

            auto it = _handle2fb.find(handle);

            result =    it != _handle2fb.end()
                     && drmModeRmFB(_fd, it->second) == 0
                     && _handle2fb.erase(handle) == 1
                     ;
        }

        return result;
    }

    bool ModeSet::DRM::EnableDisplay() const
    {
        bool result = false;

        if (IsValid()) {
            drmModeConnectorPtr ptr = drmModeGetConnector(_fd, _connector);

            if (ptr != nullptr) {
                uint32_t connector = _connector;

                result = drmModeSetCrtc(_fd, _crtc, _fb, 0, 0, &connector, 1, &(ptr->modes[_mode])) == 0;

                drmModeFreeConnector(ptr);
            }
        }

        return result;
    }

    bool ModeSet::DRM::DisableDisplay() const
    {
// FIXME: more to be done
        bool result = IsValid();
        return result;
    }

#ifdef __cplusplus
extern "C" {
#endif
    void PageFlip(int, unsigned int, unsigned int, unsigned int, void* data)
    {
        ASSERT(data != nullptr);

        if (data != nullptr) {
            *reinterpret_cast<std::atomic<bool>*>(data) = false;
        } else {
            TRACE_GLOBAL(Trace::Error, (_T("Invalid callback data.")));
        }
    }
#ifdef __cplusplus
}
#endif

    bool ModeSet::DRM::ShowFramebuffer(struct gbm_bo* bo) const 
    {
        bool result = false;

        if (   IsValid()
            && bo != GBM::InvalidBuffer()
           ) {
            const uint32_t handle = gbm_bo_get_handle(bo).u32;

            const auto it = _handle2fb.find(handle);

            if (it != _handle2fb.end()) {
                // Reset by callback
                static std::atomic<bool> data;
                data = true;

                int status = drmModePageFlip(_fd, _crtc, it->second, DRM_MODE_PAGE_FLIP_EVENT, &data);

                ASSERT(status != -EINVAL);
                ASSERT(status != -EBUSY);

                if (status == 0) {
                    static_assert(   !narrowing<decltype(DRM_EVENT_CONTEXT_VERSION), int, true>::value
                                  || (   DRM_EVENT_CONTEXT_VERSION >= 0
                                      && in_signed_range<int, DRM_EVENT_CONTEXT_VERSION>::value
                                     )
                                 );

                    drmEventContext context = {
                          .version = 2
                        , .vblank_handler = nullptr
                        , .page_flip_handler = PageFlip
#if DRM_EVENT_CONTEXT_VERSION > 2
                        , .page_flip_handler2 = nullptr
#endif
#if DRM_EVENT_CONTEXT_VERSION > 3
                        , .sequence_handler = nullptr
#endif
                    };

                    struct timespec timeout = { .tv_sec = 1, .tv_nsec = 0 };
                    fd_set fds;

                    while (data) {
                        FD_ZERO(& fds);
                        FD_SET(_fd, &fds);

                        status = pselect(_fd+1, &fds, nullptr, nullptr, &timeout, nullptr);

                        if (status < 0) {
                            TRACE(Trace::Error, (_T("Event processing for page flip failed.")));
                            break;
                        } else {

                            if (status == 0) {
                                // Timeout; retry
                                TRACE(Trace::Error, (_T("Unable to execute a timely page flip. Trying again.")));
                            } else {
                                result =    FD_ISSET(_fd, &fds) != 0
                                         && drmHandleEvent(_fd, &context) == 0
                                         ;

                                if (!result) {
                                    break;
                                }
                            }
                        }
                    }
                } else {
                     TRACE(Trace::Error, (_T("Unable to execute a page flip.")));
                }
            }
        }

        return result;
    }

    bool ModeSet::DRM::IsValid() const
    {
        bool result =    _fd != InvalidFileDescriptor()
                      && _crtc != InvalidCrtc()
                      && _encoder != InvalidEncoder()
                      && _connector != InvalidConnector()
                      && _fb != InvalidFramebuffer()
                      // _mode !=
                      && _width != InvalidWidth()
                      && _height != InvalidHeight()
                      // _vrefresh !=
                      ;

        return result;
    }

   void ModeSet::DRM::AvailableNodes(uint32_t type, std::vector<std::string>& list) const
   {
        // Arbitrary value
        constexpr const uint8_t DrmMaxDevices = 16;

        drmDevicePtr devices[DrmMaxDevices];

        list.clear();

        const int count = drmAvailable() == 1 ? drmGetDevices2(0 /* flags */, &devices[0], /*int*/ DrmMaxDevices) : 0;

        if (count > 0) {
            static_assert(   !narrowing<decltype(DRM_NODE_PRIMARY), uint32_t, true>::value
                          || (   DRM_NODE_PRIMARY >= 0
                              && in_unsigned_range<uint32_t, DRM_NODE_PRIMARY>::value
                             )
                          );
            static_assert(   !narrowing<decltype(DRM_NODE_CONTROL), uint32_t, true>::value
                          || (   DRM_NODE_CONTROL >= 0
                              && in_unsigned_range<uint32_t, DRM_NODE_CONTROL>::value
                             )
                          );
            static_assert(   !narrowing<decltype(DRM_NODE_RENDER), uint32_t, true>::value
                          || (   DRM_NODE_RENDER >= 0
                              && in_unsigned_range<uint32_t, DRM_NODE_RENDER>::value
                             )
                          );
            static_assert(   !narrowing<decltype(DRM_NODE_MAX), uint32_t, true>::value
                          || (   DRM_NODE_MAX >= 0
                              && in_unsigned_range<uint32_t, DRM_NODE_MAX>::value
                             )
                          );

            for (int i = 0; i < count; i++) {
                switch (type) {
                case DRM_NODE_PRIMARY   :   // card<num>, always created, KMS, privileged
                case DRM_NODE_CONTROL   :   // ControlD<num>, currently unused
                case DRM_NODE_RENDER    :   // Solely for render clients, unprivileged
                                            {
                                                if ( (1 << type) == (devices[i]->available_nodes & (1 << type))) {
                                                    list.push_back(std::string(devices[i]->nodes[type]));
                                                }
                                                break;
                                            }
                case DRM_NODE_MAX       :
                default                 :   // Unknown (new) node type
                                            ASSERT(false);
                                            ;
                }
            }

            drmFreeDevices(&devices[0], count);
        }
    }

    bool ModeSet::DRM::Close()
    {
        bool result =    _fd != InvalidFileDescriptor()
                      && (   drmDropMaster(_fd) == 0
                          || close(_fd) == 0
                         )
                      ;

        return result;
    }

    bool ModeSet::DRM::InitializeDisplayProperties()
    {
        bool result = false;

        drmModeResPtr resources =    drmAvailable() == 1 
                                  && _fd != InvalidFileDescriptor() 
                                  ? drmModeGetResources(_fd) : nullptr
                                  ;

        if (resources != nullptr) {
            for (int i = 0; i < resources->count_connectors; i++) {
                drmModeConnectorPtr connectors = drmModeGetConnector(_fd, resources->connectors[i]);

                static_assert(   !narrowing<decltype(DRM_MODE_CONNECTOR_HDMIA), uint32_t, true>::value
                              || (   DRM_MODE_CONNECTOR_HDMIA >= 0
                                  && in_unsigned_range<uint32_t, DRM_MODE_CONNECTOR_HDMIA>::value
                                 )
                             );
 
                if (  connectors != nullptr
                    && connectors->connector_type == DRM_MODE_CONNECTOR_HDMIA
                    && connectors->connection == DRM_MODE_CONNECTED
                    ) {
                    for (int j = 0; j < connectors->count_encoders; j++) {
                        drmModeEncoderPtr encoders = drmModeGetEncoder(_fd, connectors->encoders[j]);

                        if (encoders != nullptr) {
                            for (int k = 0; k < resources->count_crtcs; k++) {

                                if ((encoders->possible_crtcs & (static_cast<uint32_t>(1) << k)) == (static_cast<uint32_t>(1) << k)) {
                                    drmModeCrtcPtr crtc = drmModeGetCrtc(_fd, resources->crtcs[k]);

                                    if (   crtc != nullptr
                                           && crtc->mode_valid == 1
                                       ) {
                                        _crtc = crtc->crtc_id;
                                        _encoder = encoders->encoder_id;
                                        _connector = connectors->connector_id;
                                        _fb = crtc->buffer_id;

                                        result = PreferredDisplayMode();
                                    }

                                    // nullptr's are allowed in drm free's
                                    drmModeFreeCrtc(crtc);
                                }
                            }
                            drmModeFreeEncoder(encoders);
                        }
                    }
                    drmModeFreeConnector(connectors);
                }
            }
            drmModeFreeResources(resources);
        }

        return result;
    }

    bool ModeSet::DRM::PreferredDisplayMode()
    {
        bool result = false;

        drmModeConnectorPtr connector =    drmAvailable() == 1 
                                        && _fd != InvalidFileDescriptor() 
                                        && _connector != InvalidConnector() 
                                        ? drmModeGetConnector(_fd, _connector) : nullptr
                                        ;

        _mode = 0;
        _vrefresh = 0;

        _width = InvalidWidth();
        _height = InvalidHeight();

        if (connector != nullptr) {
            uint64_t area[2] = { 0, 0 };
            uint64_t rate[2] = { 0, 0 };
            uint64_t clock[2] = { 0, 0 };

            for (int i = 0; i < connector->count_modes; i++) { 
                static_assert(    !narrowing<decltype(DRM_MODE_TYPE_PREFERRED), uint32_t, true>::value
                              || (    DRM_MODE_TYPE_PREFERRED >= 0
                                  && in_unsigned_range<uint32_t, DRM_MODE_TYPE_PREFERRED>::value
                                 )
                             );

                const uint32_t type = connector->modes[i].type;

                // At least one preferred mode should be set by the driver, but dodgy EDID parsing might not provide it
                if (DRM_MODE_TYPE_PREFERRED == (DRM_MODE_TYPE_PREFERRED & type)) {
                    _mode = i;
                    _vrefresh = connector->modes[i].vrefresh;
                    _width = connector->modes[i].hdisplay;
                    _height = connector->modes[i].vdisplay;

                    break;
                } else {
                    static_assert(   !narrowing<decltype(DRM_MODE_TYPE_DRIVER), uint32_t, true>::value 
                                  || (   DRM_MODE_TYPE_DRIVER >= 0
                                      && in_unsigned_range<uint32_t, DRM_MODE_TYPE_DRIVER>::value
                                     )
                                 );

                    if (DRM_MODE_TYPE_DRIVER == (DRM_MODE_TYPE_DRIVER & type)) {
                        area[1] = connector->modes[i].hdisplay * connector->modes[i].vdisplay;
                        rate[1] = connector->modes[i].vrefresh;
                        clock[1] = connector->modes[i].clock;

                        if (   area[0] < area[1]
                            || (   area[0] == area[1]
                                && rate[0] < rate[1]
                               )
                            || (   area[0] == area[1] 
                                && rate[0] == rate[1]
                                && clock[0] < clock[1]
                               )
                            ) {
                            _mode = i;
                            _vrefresh = connector->modes[i].vrefresh;
                            _width = connector->modes[i].hdisplay;
                            _height = connector->modes[i].vdisplay;
                        }
                    }
                }
            }
        }

        // nullptr allowed
        drmModeFreeConnector(connector);

        result = IsValid();

        return result;
    }



    ModeSet::GBM::GBM(DRM& drm)
        : _drm{drm}
        , _device{InvalidDevice()}
        , _bos()
        , _surfaces()
        , _bo2surface()
    {}

    ModeSet::GBM::~GBM()
    {
        ASSERT(_surfaces.size() == 0);
        ASSERT(_bo2surface.size() == 0);
        ASSERT(_bos.size() == 0);

        if (_device != InvalidDevice()) {
            // Releases all resources
            gbm_device_destroy(_device);
        }
    }

    bool ModeSet::GBM::CreateSurface(uint32_t width, uint32_t height, struct gbm_surface*& surface) const
    {
        struct gbm_device* device = InvalidDevice();

        bool result = false;

        if ((   IsValid()
             || Device(device)
            )
            && surface == InvalidSurface()
           ) {
            const uint64_t modifier = FormatModifier();

            surface = gbm_surface_create_with_modifiers(_device, width, height, SupportedBufferType(), &modifier, 1);

            result = surface != InvalidSurface();

            if (result) {
                _surfaces.push_back(surface);
            }
        }

        return result;
    }

    bool ModeSet::GBM::DestroySurface(struct gbm_surface*& surface) const
    {
        bool result = false;

        if (   IsValid()
            && surface != InvalidSurface()
           ) {
            auto its = std::find_if(  _surfaces.begin()
                                    , _surfaces.end()
                                    , [&surface](struct gbm_surface* surf) 
                                      {
                                         return surface == surf;
                                      }
                                   );

            if (its != _surfaces.end()) {
                auto itb = _bo2surface.end();
                do {
                    itb = std::find_if(  _bo2surface.begin()
                                       , _bo2surface.end()
                                       , [&surface](std::pair<struct gbm_bo*, struct gbm_surface*> item) 
                                         {
                                             return item.second == surface;
                                         }
                                      );

                    if (itb != _bo2surface.end()) {
                        /* size_type */ _bo2surface.erase(itb);
                    }
                } while (itb != _bo2surface.end());

                const auto count = _surfaces.size();

                /* iterator */ _surfaces.erase(its);

                result = count > _surfaces.size();

                if (result) { 
                    /* void */ gbm_surface_destroy(surface);
                    surface = InvalidSurface();
                }
            }
        }

        return result;
    }

    bool ModeSet::GBM::CreateBuffer(struct gbm_bo*& bo)
    {
        struct gbm_device* device = InvalidDevice();

        bool result = false;

        if ((   IsValid()
             || Device(device)
            )
            && bo == InvalidBuffer()
           ) {
            const uint64_t modifier  = ModeSet::GBM::FormatModifier();

            bo = gbm_bo_create_with_modifiers(_device, _drm.DisplayWidth(), _drm.DisplayHeight(), SupportedBufferType(), &modifier, 1);

            result = bo != InvalidBuffer();

            if (result) {
                _bos.push_back(bo);
            }
        }

        return result;
    }

    bool ModeSet::GBM::CreateBufferFromSurface(struct gbm_surface* surface, struct gbm_bo*& bo)
    {
        return Lock(surface, bo);
    }

    bool ModeSet::GBM::DestroyBufferFromSurface(struct gbm_surface* surface, struct gbm_bo*& bo)
    {
        return Unlock(surface, bo);
    }

    bool ModeSet::GBM::DestroyBuffer(struct gbm_bo* bo)
    {
        bool result = false;

        if (   IsValid()
            && bo != InvalidBuffer()
           ) {
            auto its = std::find_if(  _bos.begin()
                                    , _bos.end()
                                    , [&bo](struct gbm_bo* buffer) 
                                    {
                                        return buffer == bo;
                                    }
                                  );

            if (its != _bos.end()) {
                const auto count = _bos.size();

                /* iterator */ _bos.erase(its);

                result = count > _bos.size();

                if (result) {
                    /* void */ gbm_bo_destroy(bo);
                }
            }
        }

        return result;
    }

    bool ModeSet::GBM::Lock(struct gbm_surface* surface, struct gbm_bo*& bo)
    {
        bool result = false;

        if (   IsValid()
            && surface != InvalidSurface()
            && bo == InvalidBuffer()
           ) {
            bo = gbm_surface_lock_front_buffer(surface);

            if (bo != InvalidBuffer()) {
                auto it = _bo2surface.insert({bo, surface});

                result = it.second;
            }
        }
 
        return result;
    }

    bool ModeSet::GBM::Unlock(struct gbm_surface* surface, struct gbm_bo*& bo)
    {
        bool result =    IsValid()
                      && surface != InvalidSurface()
                      && bo != InvalidBuffer()
                      && _bo2surface.erase(bo) > 0
                      ;

        if (result) {
            /* void* */ gbm_surface_release_buffer(surface, bo);
            bo = InvalidBuffer();
        }

        return result;
    }

    bool ModeSet::GBM::Device(struct gbm_device*& device) const
    {
        if (   _device == InvalidDevice()
            && device == InvalidDevice()
            && _drm.FileDescriptor() != DRM::InvalidFileDescriptor()
           ) {
            _device = gbm_create_device(_drm.FileDescriptor());
            
            device = _device;
        }

        bool result = _device != InvalidDevice();

        return result;
    }

    ModeSet::ModeSet()
        : _drm()
        , _gbm{_drm}
        , _bo{GBM::InvalidBuffer()}
    {
//        we should call open automatically
    }

    ModeSet::~ModeSet()
    {
        /* uint32_t */ Close ();
    }

    uint32_t ModeSet::Open()
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_drm.Open(DRM_NODE_PRIMARY)) {
            _bo = GBM::InvalidBuffer();

            result =    _gbm.CreateBuffer(_bo)
                     && _drm.AddFramebuffer(_bo)
                     && _drm.EnableDisplay()
                     ? Core::ERROR_NONE : Core::ERROR_GENERAL
                     ;
        }

        return result;
    }

    uint32_t ModeSet::Close()
    {
        uint32_t result =    _drm.DisableDisplay()
                          && _drm.RemoveFramebuffer(_bo)
                          && _gbm.DestroyBuffer(_bo)
                          ? Core::ERROR_NONE : Core::ERROR_GENERAL
                          ;

        return result;
    }

    struct gbm_device* /* const */ ModeSet::UnderlyingHandle() const
    {
        return _gbm.Device();
    }

    int ModeSet::Descriptor() const
    {
        return DRM::InvalidFileDescriptor();
    }

    uint32_t ModeSet::Width() const
    {
        return _drm.DisplayWidth();
    }

    uint32_t ModeSet::Height() const
    {
        return _drm.DisplayHeight();
    }

    uint32_t ModeSet::RefreshRate() const
    {
        return _drm.DisplayRefreshRate();
    }

    bool ModeSet::Interlaced() const {
        bool ret = false;
        return ret;
    }

    struct gbm_surface* ModeSet::CreateRenderTarget(uint32_t width, uint32_t height) const
    {
        struct gbm_surface* result = GBM::InvalidSurface();

        /* bool */ _gbm.CreateSurface(width, height, result);

        return result;
    }

    void ModeSet::DestroyRenderTarget(struct gbm_surface*& surface) const
    {
        if (_gbm.DestroySurface(surface)) {
            surface = GBM::InvalidSurface();
        }
    }

    struct gbm_bo* ModeSet::CreateBufferObject(uint32_t width, uint32_t height)
    {
        struct gbm_bo* result = GBM::InvalidBuffer();

        /* bool */ _gbm.CreateBuffer(result);

        return result;
    }

    void ModeSet::DestroyBufferObject(struct gbm_bo*& bo)
    {
        if (_gbm.DestroyBuffer(bo)) {
            bo = GBM::InvalidBuffer();
        }
    }

    void ModeSet::Swap(struct gbm_surface* surface)
    {
        static std::queue<BufferInfo> frames;

        BufferInfo frame(surface, GBM::InvalidBuffer(), DRM::InvalidFramebuffer()); 

        if (   _drm.IsValid()
            && _gbm.IsValid()
            && _gbm.CreateBufferFromSurface(frame.Surface(), frame.Buffer())
            && _drm.AddFramebuffer(frame.Buffer())
            && _drm.ShowFramebuffer(frame.Buffer())
           ) {
            frames.push(frame);

            if (frames.size() > 1) {
                frame = frames.front();
                frames.pop();
            }
        } else {
            TRACE(Trace::Error, (_T("Unable to scan out new frame.")));
        }

        /* bool */ _gbm.DestroyBufferFromSurface(frame.Surface(), frame.Buffer());
        /* bool */ _drm.RemoveFramebuffer(frame.Buffer());
    }
}
