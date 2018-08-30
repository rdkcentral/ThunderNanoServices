#ifndef RPI_CPP_ABSTRACTION_H
#define RPI_CPP_ABSTRACTION_H

#include <string>
#include <cassert>
#include <list>
#include <algorithm>

#include <bcm_host.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "../../Client/Client.h"

namespace WPEFramework {
namespace Rpi {

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

    public:
        class IpcClient {
        public:
            IpcClient();
            virtual ~IpcClient();

        private:
            int sock;
    #define IPC_DATABUF_SIZE 256
            char _sendBuf[IPC_DATABUF_SIZE];
            char _recvBuf[IPC_DATABUF_SIZE];
        };

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
            return (static_cast<EGLNativeWindowType>(_nativeWindow));
        }
        virtual const std::string& Name() const override {
            return _name;
        }
        virtual int32_t Height() const override {
            return (_height);
        }
        virtual int32_t Width() const override {
            return (_width);
        }
        virtual void Keyboard(
                Compositor::IDisplay::IKeyboard* keyboard) override {
            assert((_keyboard == nullptr) ^ (keyboard == nullptr));
            _keyboard = keyboard;
        }
        virtual int32_t X() const override {
            return (_x);
        }
        virtual int32_t Y() const override {
            return (_y);
        }
        inline void SendKey(
                const uint32_t key,
                const IKeyboard::state action, const uint32_t time) {
            if (_keyboard != nullptr) {
                _keyboard->Direct(key, action);
            }
        }

    private:
        Display& _parent;
        mutable uint32_t _refcount;
        std::string _name;
        int32_t _x;
        int32_t _y;
        int32_t _width;
        int32_t _height;
        EGLSurface _nativeWindow;
        IKeyboard* _keyboard;
        IpcClient* _ipcClient;

        EGL_DISPMANX_WINDOW_T nativeWindow;
        DISPMANX_DISPLAY_HANDLE_T dispman_display;
        DISPMANX_UPDATE_HANDLE_T dispman_update;
        DISPMANX_ELEMENT_HANDLE_T dispman_element;
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
    inline void Register(SurfaceImplementation* surface) {
        std::list<SurfaceImplementation*>::iterator index(
                std::find(_surfaces.begin(), _surfaces.end(), surface));
        if (index == _surfaces.end()) {
            _surfaces.push_back(surface);
        }
    }
    inline void Unregister(SurfaceImplementation* surface) {
        std::list<SurfaceImplementation*>::iterator index(
                std::find(_surfaces.begin(), _surfaces.end(), surface));
        if (index != _surfaces.end()) {
            _surfaces.erase(index);
        }
    }

    const std::string _displayName;
    void* _virtualkeyboard ;
    std::list<SurfaceImplementation*> _surfaces;
};

} // Rpi
} // WPEFramework

#endif // RPI_CPP_ABSTRACTION_H
