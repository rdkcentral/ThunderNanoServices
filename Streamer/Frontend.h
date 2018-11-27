#ifndef __LINEARBROADCAST_FRONTEND_H
#define __LINEARBROADCAST_FRONTEND_H

#include "Module.h"
#include "Geometry.h"

namespace WPEFramework {

namespace Player {

namespace Implementation {

class Administrator {
private:
    Administrator(const Administrator&) = delete;
    Administrator& operator= (const Administrator&) = delete;

public:
    Administrator() 
        : _adminLock()
        , _frontends(0)
        , _decoders(0)
        , _streams(nullptr)
        , _slots(nullptr) {
    }
    ~Administrator() {}

public:
    uint32_t Initialize(const string& config);
    uint32_t Deinitialize();

public:
    // These methods create and destroy the Exchange::IStream administration slots.
    // -----------------------------------------------------------------------------
    Exchange::IStream* Aquire(const Exchange::IStream::streamtype streamType);

    void Destroy(Exchange::IStream* element) {
        uint8_t index = 0;
        _adminLock.Lock();
        while ((index < _frontends) && (_streams[index] != element)) { index++; }

        ASSERT (index < _frontends);

        if (index < _frontends) {
            _streams[index] = nullptr;
        }
        _adminLock.Unlock();
    }

    // These methods allocate and deallocate a Decoder slot
    // -----------------------------------------------------------------------------
    uint8_t Allocate() {
        uint8_t index = 0;
        _adminLock.Lock();
        while ((index < _decoders) && (_slots[index] == 1)) { index++; }
        if (index < _decoders) {
            _slots[index] = 1;
        }
        else {
            index = ~0;
        }
        _adminLock.Unlock();

        return (index); 
    }
    void Deallocate(uint8_t index) {
        ASSERT (index < _decoders);

        _adminLock.Lock();

        if ((index < _decoders) && (_slots[index] == 1)) {
            _slots[index] = 0;
        }
        _adminLock.Unlock();
    }

private:
    // This list does not maintain a ref count. The interface is ref counted 
    // and determines the lifetime of the IStream!!!!
    Core::CriticalSection _adminLock;
    uint8_t _frontends;
    uint8_t _decoders;
    Exchange::IStream** _streams;
    uint8_t* _slots;
};

template<typename IMPLEMENTATION>
class FrontendType : public Exchange::IStream {
private:
    FrontendType() = delete;
    FrontendType(const FrontendType<IMPLEMENTATION>&) = delete;
    FrontendType<IMPLEMENTATION>& operator= (const FrontendType<IMPLEMENTATION>&) = delete;

    class CallbackImplementation : public Player::Implementation::ICallback {
    private:
        CallbackImplementation() = delete;
        CallbackImplementation(const CallbackImplementation&) = delete;
        CallbackImplementation& operator= (const CallbackImplementation&) = delete;

    public:
        CallbackImplementation(FrontendType<IMPLEMENTATION>* parent)
            : _parent(*parent) {
        }
        virtual ~CallbackImplementation() {
        }

    public:
        virtual void TimeUpdate(uint64_t position) {
            _parent.TimeUpdate(position);
        }
        virtual void DRM(uint32_t state) {
            _parent.DRM(state);
        }
        virtual void StateChange(Exchange::IStream::state newState) {
            _parent.StateChange(newState);
        }

    private:
        FrontendType<IMPLEMENTATION>& _parent;
    };

    template <typename ACTUALCLASS >
    class DecoderImplementation : public Exchange::IStream::IControl {
    private:
        DecoderImplementation() = delete;
        DecoderImplementation(const DecoderImplementation&) = delete;
        DecoderImplementation& operator= (const DecoderImplementation&) = delete;

    public:
        DecoderImplementation(FrontendType<ACTUALCLASS>* parent, const uint8_t decoderId)
            : _referenceCount(1)
            , _parent(*parent)
            , _index(decoderId)
            , _geometry()
            , _player(&(parent->Implementation()))
            , _callback(nullptr) {
        }
        virtual ~DecoderImplementation() {
            _parent.Detach();
        }

    public:
        virtual void AddRef() const
        {
            Core::InterlockedIncrement(_referenceCount);
        }
        virtual uint32_t Release() const
        {
            if (Core::InterlockedDecrement(_referenceCount) == 0) {
                delete this;

                return (Core::ERROR_DESTRUCTION_SUCCEEDED);
            }
            return (Core::ERROR_NONE);
        }
        virtual uint8_t Index() const {
            return (_index);
        }
        virtual void Speed(const int32_t request) override {
            _parent.Lock();
            if (_player != nullptr) {
                _player->Speed(request);
            }
            _parent.Unlock();
        }
        virtual int32_t Speed() const {
            uint32_t result = 0;
            _parent.Lock();
            if (_player != nullptr) {
                result = _player->Speed();
            }
            _parent.Unlock();
            return (result);
        }
        virtual void Position(const uint64_t absoluteTime) {
            _parent.Lock();
            if (_player != nullptr) {
                _player->Position(absoluteTime);
            }
            _parent.Unlock();
        }
        virtual uint64_t Position() const {
            uint64_t result = 0;
            _parent.Lock();
            if (_player != nullptr) {
                result = _player->Position();
            }
            _parent.Unlock();
            return (result);
        }
        virtual void TimeRange(uint64_t& begin, uint64_t& end) const {
            _parent.Lock();
            if (_player != nullptr) {
                _player->TimeRange(begin, end);
            }
            _parent.Unlock();
        }
        virtual IGeometry* Geometry() const {
            IGeometry* result = nullptr;
            _parent.Lock();
             if (_player != nullptr) {
                _geometry.Window(_player->Window());
                _geometry.Order(_player->Order());
                result = &_geometry;
            }
            _parent.Unlock();
            return (result);
        }
        virtual void Geometry(const IGeometry* settings) {
            _parent.Lock();
            if (_player != nullptr) {
                Rectangle window;
                window.X = settings->X();
                window.Y = settings->Y();
                window.Width = settings->Width();
                window.Height = settings->Height();
                _player->Window(window);
                _player->Order(settings->Z());
            }
            _parent.Unlock();
        }
        virtual void Callback(IControl::ICallback* callback) {
            _parent.Lock();

            if (callback != nullptr) {

                ASSERT (_callback == nullptr);

                if (_player != nullptr) {
                    _callback = callback;
                }
            }
            else {
                _callback = nullptr;
            }

            _parent.Unlock();
        }
        void Terminate () {
            _parent.Lock();
            ASSERT (_player != nullptr);
            _player = nullptr;
            _parent.Unlock();
        }

        BEGIN_INTERFACE_MAP(DecoderImplementation)
            INTERFACE_ENTRY(Exchange::IStream::IControl)
        END_INTERFACE_MAP

        inline void TimeUpdate(uint64_t position) {
            if (_callback != nullptr) {
                _callback->TimeUpdate(position);
            }
        }

    private:
        mutable uint32_t _referenceCount;
        FrontendType<ACTUALCLASS>& _parent;
        uint8_t _index;
        mutable Core::Sink<Implementation::Geometry> _geometry;
        ACTUALCLASS* _player;
        IControl::ICallback* _callback;
    };

public:
    FrontendType(Administrator* administration, const streamtype type, const uint8_t index)
        : _refCount(1)
        , _adminLock()
        , _index(index)
        , _administrator(administration)
        , _decoder(nullptr)
        , _callback(nullptr)
        , _sink(this)
        , _player(type, index, &_sink) {
    }
    virtual ~FrontendType() {
        if (_decoder != nullptr) {
            TRACE_L1("Forcefull destruction of a stream. Forcefully removing decoder: %d", __LINE__);
            delete _decoder;
        }
        ASSERT (_decoder == nullptr);
    }

public:
    void AddRef() const {
        Core::InterlockedIncrement(_refCount);
    }
    uint32_t Release() const {

        if (_administrator != nullptr) {
            if (Core::InterlockedDecrement(_refCount) == 0) {
                _administrator->Destroy(const_cast< FrontendType<IMPLEMENTATION> * >(this));
                delete this;
            }
        }
        else if (Core::InterlockedDecrement(_refCount) == 0) {
            delete this;
        }
    }
    virtual uint8_t Index() const {
        return (_index);
    }
    virtual streamtype Type() const {
        _adminLock.Lock();
        streamtype result = (_administrator != nullptr ? _player.Type() : streamtype::Stubbed);
        _adminLock.Unlock();
        return (result);
    }
    virtual drmtype DRM() const {
        _adminLock.Lock();
        drmtype result = (_administrator != nullptr ? _player.DRM() : drmtype::Unknown);
        _adminLock.Unlock();
        return (result);
    }
    virtual IControl* Control() {
        _adminLock.Lock();
        if ( _administrator != nullptr) {

            uint8_t decoderId;

            if ( (_decoder == nullptr) && ((decoderId = _administrator->Allocate()) != static_cast<uint8_t>(~0)) ) {

                _decoder = new DecoderImplementation<IMPLEMENTATION>(this, decoderId);

                if (_decoder != nullptr) {
                    _player.AttachDecoder(decoderId);

                    // AddRef ourselves as the Control, being handed out, needs the 
                    // Frontend created in this class. This is his parent class.....
                    AddRef();
                }
            }
            else {
                _decoder->AddRef();
            }
        }
        _adminLock.Unlock();
        return (_decoder);
    }
    virtual void Callback(IStream::ICallback* callback) {
        _adminLock.Lock();
        
        if (callback != nullptr) {

            ASSERT (_callback == nullptr);

            if (_administrator != nullptr) {
                _callback = callback;
            }
        }
        else {
            _callback = nullptr;
        }
        _adminLock.Unlock();
    }
    virtual state State() const {
        _adminLock.Lock();
        state result = (_administrator != nullptr ? _player.State() : NotAvailable);
        _adminLock.Unlock();
        return (result);
    }
    virtual uint32_t Load(std::string configuration) {
        _adminLock.Lock();
        uint32_t result = (_administrator != nullptr ? _player.Load(configuration) : 0x80000001);
        _adminLock.Unlock();
        return (result);
    }
    virtual string Metadata() const {
        _adminLock.Lock();
        string result = (_administrator != nullptr ? _player.Metadata() : string());
        _adminLock.Unlock();
        return (result);
    }
    void Terminate() {
        _adminLock.Lock();

        ASSERT (_administrator != nullptr);
       
        if (_decoder != nullptr) {
            _player.DetachDecoder(_decoder->Index());
            _administrator->Deallocate(_decoder->Index());
        } 

        _player.Terminate();

        _administrator = nullptr;

        _adminLock.Unlock();
    }

    static uint32_t Initialize(const string& configuration) {
      return(__Initialize<IMPLEMENTATION>(configuration));
    } 
    static uint32_t Deinitialize() {
      return(__Deinitialize<IMPLEMENTATION>());
    } 
 
    BEGIN_INTERFACE_MAP(FrontendType<IMPLEMENTATION>)
       INTERFACE_ENTRY(Exchange::IStream)
    END_INTERFACE_MAP

private:
    HAS_STATIC_MEMBER(Initialize, hasInitialize);
    HAS_STATIC_MEMBER(Deinitialize, hasDeinitialize);

    typedef hasInitialize<IMPLEMENTATION, uint32_t (IMPLEMENTATION::*)(const string&)> TraitInitialize;
    typedef hasDeinitialize<IMPLEMENTATION, uint32_t (IMPLEMENTATION::*)()> TraitDeinitialize;

    template <typename TYPE>
    inline static typename Core::TypeTraits::enable_if<FrontendType<TYPE>::TraitInitialize::value, uint32_t>::type
    __Initialize(const string& config)
    {
        return(IMPLEMENTATION::Initialize(config));
    }

    template <typename TYPE>
    inline static typename Core::TypeTraits::enable_if<!FrontendType<TYPE>::TraitInitialize::value, uint32_t>::type
    __Initialize(const string&)
    {
        return (Core::ERROR_NONE);
    }

    template <typename TYPE>
    inline static typename Core::TypeTraits::enable_if<FrontendType<TYPE>::TraitDeinitialize::value, uint32_t>::type
    __Deinitialize()
    {
        return (IMPLEMENTATION::Deinitialize());
    }

    template <typename TYPE>
    inline static typename Core::TypeTraits::enable_if<!FrontendType<TYPE>::TraitDeinitialize::value, uint32_t>::type
    __Deinitialize()
    {
        return (Core::ERROR_NONE);
    }

    inline void TimeUpdate(uint64_t position) {
        _adminLock.Lock();

        if ((_administrator != nullptr) && (_decoder != nullptr)) {
            _decoder->TimeUpdate(position);
        }
        _adminLock.Unlock();
    }
    void DRM(uint32_t state) {
        _adminLock.Lock();
        if ((_administrator != nullptr) && (_callback != nullptr)) {
            _callback->DRM(state);
        }
        _adminLock.Unlock();
    }
    void StateChange(Exchange::IStream::state newState) {
        _adminLock.Lock();
        if ((_administrator != nullptr) && (_callback != nullptr)) {
            _callback->StateChange(newState);
        }
        _adminLock.Unlock();
    }
    void Detach() {
        _adminLock.Lock();
        if (_administrator != nullptr) {
            ASSERT (_decoder != nullptr);
            _player.DetachDecoder(_decoder->Index());
            _administrator->Deallocate(_decoder->Index());
        }
        if (_decoder != nullptr) {
            _decoder = nullptr;
        }
        _adminLock.Unlock();
        Release();
    }
    IMPLEMENTATION& Implementation () {
        return (_player);
    }
    inline void Lock() const {
        _adminLock.Lock();
    }
    inline void Unlock() const {
        _adminLock.Unlock();
    }

private:
    mutable uint32_t _refCount;
    mutable Core::CriticalSection _adminLock;
    uint8_t _index;
    mutable Administrator* _administrator;
    DecoderImplementation<IMPLEMENTATION>* _decoder;
    IStream::ICallback* _callback;
    CallbackImplementation _sink;
    IMPLEMENTATION _player;
};

} } } // WPEFramework::Player::Implementation

#endif // __LINEARBROADCAST_FRONTEND_H
