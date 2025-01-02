#pragma once

#include <interfaces/ICompositionBuffer.h>

#include "Textures.h"

namespace Thunder {
namespace Compositor {
    class PixelBuffer : public Exchange::ICompositionBuffer {

        using Pixel = std::array<uint8_t, 4>;
        class Iterator : public Exchange::ICompositionBuffer::IIterator {
        public:
            Iterator(PixelBuffer& parent, const Texture::PixelData& source)
                : _parent(parent)
                , _index(0)
                , _source(source)
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

            int Descriptor() const override
            {
                return (-1);
            }

            uint32_t Stride() const override
            {
                return _source.width * _source.bytes_per_pixel;
            }

            uint32_t Offset() const override
            {
                return 0;
            }

        private:
            PixelBuffer& _parent;
            uint8_t _index;
            const Texture::PixelData& _source;
        };

    public:
        PixelBuffer() = delete;

        PixelBuffer(const Texture::PixelData& source)
            : _source(source)
            , _planes(*this, source)
        {
        }

        PixelBuffer(const PixelBuffer&) = delete;
        PixelBuffer& operator=(const PixelBuffer&) = delete;

        virtual ~PixelBuffer() = default;

        Exchange::ICompositionBuffer::IIterator* Acquire(const uint32_t /* timeoutMs */) override
        {
            return &_planes;
        }

        void Relinquish() override
        {
        }

        uint32_t Width() const override
        {
            return _source.width;
        }

        uint32_t Height() const override
        {
            return _source.height;
        }

        uint32_t Format() const override
        {
            return DRM_FORMAT_ABGR8888;
        }

        uint64_t Modifier() const override
        {
            return DRM_FORMAT_MOD_LINEAR;
        }

        Exchange::ICompositionBuffer::DataType Type() const override
        {
            return Exchange::ICompositionBuffer::TYPE_RAW;
        }

    private:
        const Texture::PixelData& _source;
        Iterator _planes;
    }; // class PixelBuffer
} // namespace Thunder
} // namespace Compositor