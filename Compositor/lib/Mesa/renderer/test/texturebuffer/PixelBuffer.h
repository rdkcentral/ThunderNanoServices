#pragma once

#include <interfaces/IGraphicsBuffer.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>

#include "Textures.h"

namespace Thunder {
namespace Compositor {
    class PixelBuffer : public Exchange::IGraphicsBuffer {

        using Pixel = std::array<uint8_t, 4>;
        
        class Iterator : public Exchange::IGraphicsBuffer::IIterator {
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
                return _parent.GetSharedMemoryFD();
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
            , _shmFd(-1)
            , _shmSize(0)
            , _tempFilePath()
        {
            CreateSharedMemoryBuffer();
        }

        PixelBuffer(const PixelBuffer&) = delete;
        PixelBuffer& operator=(const PixelBuffer&) = delete;

        virtual ~PixelBuffer() 
        {
            CleanupSharedMemoryBuffer();
        }

        Exchange::IGraphicsBuffer::IIterator* Acquire(const uint32_t /* timeoutMs */) override
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

        Exchange::IGraphicsBuffer::DataType Type() const override
        {
            return Exchange::IGraphicsBuffer::TYPE_RAW;
        }

        int GetSharedMemoryFD() const 
        {
            return _shmFd;
        }

    private:
        void CreateSharedMemoryBuffer()
        {
            // Calculate buffer size
            const size_t stride = _source.width * _source.bytes_per_pixel;
            _shmSize = stride * _source.height;

            // Try different methods in order of preference
            if (!TryMemfdCreate() && !TryTmpfileShm() && !TryTempFileShm()) {
                TRACE(Trace::Error, ("All shared memory creation methods failed"));
                return;
            }

            // Map and copy data
            if (_shmFd != -1) {
                CopyDataToSharedMemory();
            }
        }

        bool TryMemfdCreate()
        {
#ifdef __linux__
            // Linux-specific memfd_create (kernel 3.17+)
            _shmFd = memfd_create("test_pixel_buffer", MFD_CLOEXEC);
            if (_shmFd != -1) {
                if (ftruncate(_shmFd, _shmSize) == 0) {
                    TRACE(Trace::Information, ("Created SHM buffer using memfd_create: fd=%d", _shmFd));
                    return true;
                } else {
                    close(_shmFd);
                    _shmFd = -1;
                }
            }
#endif
            return false;
        }

        bool TryTmpfileShm()
        {
            // Try /dev/shm first (RAM-backed filesystem)
            const char* shmPath = "/dev/shm";
            struct stat st;
            
            if (stat(shmPath, &st) == 0 && S_ISDIR(st.st_mode)) {
                char tempTemplate[] = "/dev/shm/test_pixel_buffer_XXXXXX";
                _shmFd = mkstemp(tempTemplate);
                
                if (_shmFd != -1) {
                    _tempFilePath = std::string(tempTemplate);
                    
                    if (ftruncate(_shmFd, _shmSize) == 0) {
                        TRACE(Trace::Information, ("Created SHM buffer using /dev/shm: fd=%d, path=%s", 
                                                 _shmFd, _tempFilePath.c_str()));
                        return true;
                    } else {
                        close(_shmFd);
                        unlink(_tempFilePath.c_str());
                        _shmFd = -1;
                        _tempFilePath.clear();
                    }
                }
            }
            return false;
        }

        bool TryTempFileShm()
        {
            // Fallback to regular temp file
            char tempTemplate[] = "/tmp/test_pixel_buffer_XXXXXX";
            _shmFd = mkstemp(tempTemplate);
            
            if (_shmFd != -1) {
                _tempFilePath = std::string(tempTemplate);
                
                if (ftruncate(_shmFd, _shmSize) == 0) {
                    TRACE(Trace::Information, ("Created SHM buffer using temp file: fd=%d, path=%s", 
                                             _shmFd, _tempFilePath.c_str()));
                    return true;
                } else {
                    close(_shmFd);
                    unlink(_tempFilePath.c_str());
                    _shmFd = -1;
                    _tempFilePath.clear();
                }
            }
            return false;
        }

        void CopyDataToSharedMemory()
        {
            // Map the shared memory for writing
            void* mappedMemory = mmap(nullptr, _shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, _shmFd, 0);
            if (mappedMemory == MAP_FAILED) {
                TRACE(Trace::Error, ("mmap failed for test buffer: %s", strerror(errno)));
                return;
            }

            // Copy the pixel data into shared memory
            std::memcpy(mappedMemory, _source.data.data(), _shmSize);

            // Unmap - we don't need write access anymore  
            if (munmap(mappedMemory, _shmSize) != 0) {
                TRACE(Trace::Warning, ("munmap failed: %s", strerror(errno)));
            }

            TRACE(Trace::Information, ("Copied %zu bytes to test SHM buffer: %dx%d", 
                                     _shmSize, _source.width, _source.height));
        }

        void CleanupSharedMemoryBuffer()
        {
            if (_shmFd != -1) {
                close(_shmFd);
                _shmFd = -1;
            }

            // Remove temp file if we created one
            if (!_tempFilePath.empty()) {
                unlink(_tempFilePath.c_str());
                _tempFilePath.clear();
            }
        }

    private:
        const Texture::PixelData& _source;
        Iterator _planes;
        int _shmFd;
        size_t _shmSize;
        std::string _tempFilePath;
    }; // class PixelBuffer
} // namespace Thunder
} // namespace Compositor