#pragma once

#include <interfaces/ICompositionBuffer.h>

#include "../../src/GL/RenderAPI.h"
#include "../../src/GL/EGL.h"

#include "Textures.h"

namespace WPEFramework {
namespace Compositor {
    class DmaBuffer : public Exchange::ICompositionBuffer {
        class Plane : public Exchange::ICompositionBuffer::IPlane {
        public:
            Plane(DmaBuffer& source)
                : _source(source)
            {
            }

            virtual ~Plane() = default;

            Exchange::ICompositionBuffer::buffer_id Accessor() const override
            {
                return reinterpret_cast<Exchange::ICompositionBuffer::buffer_id>(_source.Identifier());
            }

            uint32_t Stride() const override
            {
                return _source.Stride();
            }

            uint32_t Offset() const override
            {
                return _source.Offset();
            }

            uint32_t Width() const
            {
                return _source.Width();
            }

            uint32_t Height() const
            {
                return _source.Height();
            }

        private:
            DmaBuffer& _source;
        };

        class Iterator : public Exchange::ICompositionBuffer::IIterator {
        public:
            Iterator(DmaBuffer& parent)
                : _parent(parent)
                , _index(0)
            {
            }

            virtual ~Iterator() = default;

            bool IsValid() const override
            {
                return ((_index > 0) && (_index <= 1));
            }
            void Reset() override
            {
                _index = 0;
            }

            bool Next() override
            {
                if (_index <= 1) {
                    _index++;
                }
                return (IsValid());
            }

            Exchange::ICompositionBuffer::IPlane* Plane() override
            {
                return &_parent.GetPlanes();
            }

        private:
            DmaBuffer& _parent;
            uint8_t _index;
        };

    public:
        DmaBuffer() = delete;

        DmaBuffer(int fd, const Texture::PixelData& source);

        DmaBuffer(const DmaBuffer&) = delete;
        DmaBuffer& operator=(const DmaBuffer&) = delete;

        virtual ~DmaBuffer();

        void AddRef() const override;
        uint32_t Release() const override;

        uint32_t Identifier() const override;

        Exchange::ICompositionBuffer::IIterator* Planes(const uint32_t /*timeoutMs*/) override;

        uint32_t Completed(const bool /*dirty*/) override;

        void Render() override;

        uint32_t Width() const override;
        uint32_t Height() const override;
        uint32_t Format() const override;
        uint64_t Modifier() const override;
        uint32_t Stride() const;
        uint64_t Offset() const;
        Exchange::ICompositionBuffer::DataType Type() const override;

    protected:
        Plane& GetPlanes();

    private:
        int _id;
        API::EGL _api;
        Renderer::EGL _egl;

        Plane _plane;
        Iterator _iterator;

        EGLContext _context;
        GLenum _target;

        const uint32_t _width;
        const uint32_t _height;
        const uint32_t _format;
        int _planes;

        GLuint _textureId;
        EGLImage _image;
        int _fourcc;
        EGLuint64KHR _modifiers;
        EGLint _stride;
        EGLint _offset;

    }; // class DmaBuffer
}
}
