#ifndef NEXUS_CPP_ABSTRACTION_H
#define NEXUS_CPP_ABSTRACTION_H

#define EGL_EGLEXT_PROTOTYPES 1

#include <string>
#include <cassert>
#include <list>
#include <algorithm>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#if __cplusplus <= 199711L
#define nullptr NULL
#endif

#include "../../Client/Client.h"

#include <refsw/nexus_config.h>
#include <refsw/nexus_platform.h>
#include <refsw/nexus_display.h>
#include <refsw/nexus_core_utils.h>
#include <refsw/default_nexus.h>

#ifdef BACKEND_BCM_NEXUS_NXCLIENT
#include <refsw/nxclient.h>
#endif

namespace WPEFramework {
namespace Nexus {

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
            SurfaceImplementation(Display& compositor, const std::string& name, const uint32_t width, const uint32_t height);
            virtual ~SurfaceImplementation();

        public:
            virtual uint32_t AddRef() const override
            {
                _refcount++;
                return (_refcount);
            }
            virtual uint32_t Release() const override
            {
                if (--_refcount == 0) {
                    delete const_cast<SurfaceImplementation*>(this);
                }
                return (0);
            }
            virtual EGLNativeWindowType Native() const override
            {
                return (static_cast<EGLNativeWindowType>(_nativeWindow));
            }
            virtual const std::string& Name() const override
            {
                return _name;
            }
            virtual int32_t Height() const override
            {
                return (_height);
            }
            virtual int32_t Width() const override
            {
                return (_width);
            }
            virtual void Keyboard(Compositor::IDisplay::IKeyboard* keyboard) override
            {
                assert((_keyboard == nullptr) ^ (keyboard == nullptr));
                _keyboard = keyboard;
            }
            virtual int32_t X() const override
            {
                return (_x);
            }
            virtual int32_t Y() const override
            {
                return (_y);
            }
            inline void SendKey (const uint32_t key, const IKeyboard::state action, const uint32_t time) {

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
        };

    private:
        Display(const std::string& displayName);

    public:
        static Compositor::IDisplay* Instance(const std::string& displayName);

        virtual ~Display();

    public:
        // Lifetime management
        virtual uint32_t AddRef() const 
        {
            // Display can not be destructed, so who cares :-)
            return (0);
        }
        virtual uint32_t Release() const
        {
            // Display can not be destructed, so who cares :-)
            return (0);
        }

        // Methods
        virtual EGLNativeDisplayType Native() const override 
        {
            return (static_cast<EGLNativeDisplayType>(EGL_DEFAULT_DISPLAY));
        }
        virtual const std::string& Name() const override
        {
            return (_displayName);
        }
        virtual int Process (const uint32_t data) override;
        virtual int FileDescriptor() const override;
        virtual ISurface* Create(const std::string& name, const uint32_t width, const uint32_t height) override;

    private:
        inline void Register(SurfaceImplementation* surface) {
            std::list<SurfaceImplementation*>::iterator index(std::find(_surfaces.begin(), _surfaces.end(), surface));

            if (index == _surfaces.end()) {
                _surfaces.push_back(surface);
            }
        }
        inline void Unregister(SurfaceImplementation* surface) {
            std::list<SurfaceImplementation*>::iterator index(std::find(_surfaces.begin(), _surfaces.end(), surface));

            if (index != _surfaces.end()) {
                _surfaces.erase(index);
            }
        }

    private:
        const std::string _displayName;
        NXPL_PlatformHandle _nxplHandle;
        void* _virtualkeyboard ;
        std::list<SurfaceImplementation*> _surfaces;
    };

} // Nexus 
} // WPEFramework

#endif // NEXUS_CPP_ABSTRACTION_H
