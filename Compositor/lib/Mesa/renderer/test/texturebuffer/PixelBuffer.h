#pragma once

#include <interfaces/ICompositionBuffer.h>

#include "Textures.h"

namespace WPEFramework {
namespace Compositor {
    class PixelBuffer : public Exchange::ICompositionBuffer {

        using Pixel = std::array<uint8_t, 4>;

        class Pixels : public Exchange::ICompositionBuffer::IPlane {
        public:
            Pixels(const Texture::PixelData& source)
                : _source(source)
            {
            }

            virtual ~Pixels() = default;

            Exchange::ICompositionBuffer::buffer_id Accessor() const override
            {
                return reinterpret_cast<Exchange::ICompositionBuffer::buffer_id>(_source.data.data());
            }

            uint32_t Stride() const override
            {
                return _source.width * _source.bytes_per_pixel;
            }

            uint32_t Offset() const override
            {
                return 0;
            }

            uint32_t Width() const 
            {
                return _source.width;
            }

            uint32_t Height() const 
            {
                return _source.height;
            }

        private:
            const Texture::PixelData& _source;
        };
        
        class Iterator : public Exchange::ICompositionBuffer::IIterator {
        public:
            Iterator(PixelBuffer& parent)
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
                return &_parent.GetPixels();
            }

        private:
            PixelBuffer& _parent;
            uint8_t _index;
        };

    public:
        PixelBuffer() = delete;

        PixelBuffer(const Texture::PixelData& source)
            : _pixels(source)
            , _planes(*this)
        {
        }

        PixelBuffer(const PixelBuffer&) = delete;
        PixelBuffer& operator=(const PixelBuffer&) = delete;

        virtual ~PixelBuffer() = default;

        uint32_t AddRef() const override;

        uint32_t Release() const override;

        uint32_t Identifier() const override;

        Exchange::ICompositionBuffer::IIterator* Planes(const uint32_t timeoutMs) override;

        uint32_t Completed(const bool dirty) override;

        void Render() override;

        uint32_t Width() const override;

        uint32_t Height() const override;

        uint32_t Format() const override;

        uint64_t Modifier() const override;

        Exchange::ICompositionBuffer::DataType Type() const override;

    protected:
        Pixels& GetPixels();

    private:
        Pixels _pixels;
        Iterator _planes;
    }; // class PixelBuffer
} // namespace WPEFramework
} // namespace Compositor