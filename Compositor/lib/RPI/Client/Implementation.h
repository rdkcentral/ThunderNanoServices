#ifndef RPI_CPP_ABSTRACTION_H
#define RPI_CPP_ABSTRACTION_H

#include <bcm_host.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <interfaces/IComposition.h>
#include "../../Client/Client.h"

#define ELEMENT_CHANGE_DEST_RECT (1<<2)
#define ELEMENT_CHANGE_OPACITY (1<<1)

namespace WPEFramework {
namespace Rpi {

class AccessorCompositor;

class Display : public Compositor::IDisplay {
private:
    Display() = delete;
    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;

    class SurfaceImplementation : public Compositor::IDisplay::ISurface {
    private:
        SurfaceImplementation() = delete;
        SurfaceImplementation(const SurfaceImplementation&) = delete;
        SurfaceImplementation& operator=(const SurfaceImplementation&) = delete;

        class RaspberryPiClient : public Exchange::IComposition::IClient {
        public:
            RaspberryPiClient() = delete;
            RaspberryPiClient(const RaspberryPiClient&) = delete;
            RaspberryPiClient& operator=(const RaspberryPiClient&) = delete;

            RaspberryPiClient(
                    SurfaceImplementation& surface,
                    const std::string& name, const uint32_t x,
                    const uint32_t y, const uint32_t width,
                    const uint32_t height);
            virtual ~RaspberryPiClient();
            virtual void Opacity(const uint32_t value) override;
            virtual void Geometry(const uint32_t X, const uint32_t Y,
                    const uint32_t width, const uint32_t height) override;
            virtual void SetTop() override;
            virtual void SetInput() override;
            virtual void Visible(const bool visible) override;

            virtual void AddRef() const override {
                _refcount++;
            }
            virtual uint32_t Release() const override {
                if (--_refcount == 0) {
                    delete const_cast<RaspberryPiClient*>(this);
                }
                return (0);
            }
            virtual string Name() const override {
                return _name;
            }
            virtual void Kill() override {
                //todo: implement
                fprintf(stderr, "Kill called!!!\n");
            }
            EGLNativeDisplayType Native() const {
                return (static_cast<EGLNativeWindowType>(_nativeSurface));
            }
            uint32_t X() const {
                return _x;
            }
            uint32_t Y() const {
                return _y;
            }
            uint32_t Width() const {
                return _width;
            }
            uint32_t Height() const {
                return _height;
            }
            const string& ClientName() const {
                return _name;
            }

            BEGIN_INTERFACE_MAP(Entry)
            INTERFACE_ENTRY(Exchange::IComposition::IClient)
            END_INTERFACE_MAP

        private:
            SurfaceImplementation& _parent;
            mutable uint32_t _refcount;
            std::string _name;
            uint32_t _x;
            uint32_t _y;
            uint32_t _width;
            uint32_t _height;
            uint32_t _opacity;
            int32_t _layerNum;

            EGLSurface _nativeSurface;
            EGL_DISPMANX_WINDOW_T _nativeWindow;
            DISPMANX_DISPLAY_HANDLE_T _dispmanDisplay;
            DISPMANX_UPDATE_HANDLE_T _dispmanUpdate;
            DISPMANX_ELEMENT_HANDLE_T _dispmanElement;

            VC_RECT_T _dstRect;
            VC_RECT_T _srcRect;
        };

    public:
        SurfaceImplementation(
                Display& compositor, const std::string& name,
                const uint32_t width, const uint32_t height);
        virtual ~SurfaceImplementation();
        virtual uint32_t AddRef() const override {
            _refcount++;
            return (_refcount);
        }
        virtual uint32_t Release() const override {
            if (--_refcount == 0) {
                delete const_cast<SurfaceImplementation*>(this);
            }
            return (0);
        }
        virtual EGLNativeWindowType Native() const override {
            return _client->Native();
        }
        virtual const string& Name() const override {
            return _client->ClientName();
        }
        virtual int32_t Height() const override {
            return _client->Height();
        }
        virtual int32_t Width() const override {
            return (_client->Width());
        }
        virtual void Keyboard(
                Compositor::IDisplay::IKeyboard* keyboard) override {
            assert((_keyboard == nullptr) ^ (keyboard == nullptr));
            _keyboard = keyboard;
        }
        virtual int32_t X() const override {
            return (_client->X());
        }
        virtual int32_t Y() const override {
            return (_client->Y());
        }
        inline void SendKey(
                const uint32_t key,
                const IKeyboard::state action, const uint32_t time) {
            if (_keyboard != nullptr) {
                _keyboard->Direct(key, action);
            }
        }
        inline const Exchange::IComposition::IClient* Client() const {
            return _client;
        }
        inline Exchange::IComposition::IClient* Client() {
            return _client;
        }
        void SetInput() {
            _parent.SetInput(this);
        }
        int32_t getLayerNum() {
            return _parent.getLayerNum();
        }

    private:
        Display& _parent;
        mutable uint32_t _refcount;
        IKeyboard* _keyboard;
        RaspberryPiClient* _client;
    };

private:
    Display(const std::string& displayName);

public:
    virtual ~Display();
    virtual uint32_t AddRef() const {
        return (0);
    }
    virtual uint32_t Release() const {
        return (0);
    }
    virtual EGLNativeDisplayType Native() const override {
        return (static_cast<EGLNativeDisplayType>(EGL_DEFAULT_DISPLAY));
    }
    virtual const std::string& Name() const override {
        return (_displayName);
    }
    virtual int Process (const uint32_t data) override;
    virtual int FileDescriptor() const override;
    virtual ISurface* Create(
            const std::string& name,
            const uint32_t width, const uint32_t height) override;
    static Compositor::IDisplay* Instance(const std::string& displayName);

private:
    inline void Register(SurfaceImplementation* surface);
    inline void Unregister(SurfaceImplementation* surface);
    void SetInput(SurfaceImplementation* surface) {
        _inputSurface = surface;
    }
    int32_t getLayerNum() {
        return _glayerNum++;
    }

    const std::string _displayName;
    int32_t _glayerNum;
    void* _virtualkeyboard;
    SurfaceImplementation* _inputSurface;
    std::list<SurfaceImplementation*> _surfaces;
    AccessorCompositor* _accessorCompositor;
};
} // Rpi
} // WPEFramework

#endif // RPI_CPP_ABSTRACTION_H
