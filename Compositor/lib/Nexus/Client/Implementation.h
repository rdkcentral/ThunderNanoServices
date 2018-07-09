#ifndef NEXUS_CPP_ABSTRACTION_H
#define NEXUS_CPP_ABSTRACTION_H

#define EGL_EGLEXT_PROTOTYPES 1

#include <string>
#include <map>
#include <cassert>
#include <list>
#include <signal.h>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#if __cplusplus <= 199711L
#define nullptr NULL
#endif

#include "../../Client/Client.h"

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
            virtual ~SurfaceImplementation()
            {
            }

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
                return (static_cast<EGLNativeWindowType>(_eglSurfaceWindow));
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

        private:
            friend Display;

            mutable uint32_t _refcount;
            std::string _name;
            int32_t _x;
            int32_t _y;
            int32_t _width;
            int32_t _height;
            EGLSurface _eglSurfaceWindow;
            IKeyboard* _keyboard;
        };

        Display(const std::string& displayName)
            : _displayName(displayName)
            , _eglDisplay(EGL_NO_DISPLAY)
            , _eglConfig(0)
            , _eglContext(EGL_NO_CONTEXT)
            , _fd(-1) {
            Initialize();
        }

    public:
        static Compositor::IDisplay* Instance(const std::string& displayName);

        virtual ~Display() { 
            Deinitialize();
        }

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
            return (static_cast<EGLNativeDisplayType>(_eglDisplay));
        }
        virtual const std::string& Name() const override
        {
            return (_displayName);
        }
        virtual int Process (const uint32_t data) override 
        {
            return (0);
        }
        virtual int FileDescriptor() const override;
        virtual ISurface* Create(const std::string& name, const uint32_t width, const uint32_t height) override;

    private:
        void Initialize();
        void Deinitialize();

    private:
        friend class Surface;
        friend class Image;

        const std::string _displayName;

        // EGL related info, if initialized and used.
        EGLDisplay _eglDisplay;
        EGLConfig _eglConfig;
        EGLContext _eglContext;

        int _fd;
    };
} // Nexus 
} // WPEFramework

#endif // NEXUS_CPP_ABSTRACTION_H
