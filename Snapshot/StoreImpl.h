#pragma once

#include "Module.h"
#include <interfaces/ICapture.h>

namespace WPEFramework {
namespace Plugin {

    class StoreImpl : public Exchange::ICapture::IStore {
    private:
        StoreImpl() = delete;
        StoreImpl(const StoreImpl&) = delete;
        StoreImpl& operator=(const StoreImpl&) = delete;

    public:
        StoreImpl(Core::BinairySemaphore& inProgress, const string& path)
            : _file(FileBodyExtended::Instance(inProgress, path))
        {
        }

        virtual ~StoreImpl()
        {
        }

        bool R8_G8_B8_A8(const unsigned char* buffer, const unsigned int width, const unsigned int height) override;

        operator Core::ProxyType<Web::IBody>()
        {
            return Core::ProxyType<Web::IBody>(*_file);
        }

        bool IsValid() const
        {
            return (_file.IsValid());
        }

        uint64_t Size() const
        {
            if (IsValid() == true) {
               return (*_file).Core::File::Size();
            }
            return 0;
        }

        void Serialize(uint8_t stream[], const uint32_t maxLength) const
        {
            if (IsValid() == true) {
                (*_file).Core::File::Read(stream, maxLength);
            }
        }

        class FileBodyExtended : public Web::FileBody {
        private:
            FileBodyExtended() = delete;
            FileBodyExtended(const FileBodyExtended&) = delete;
            FileBodyExtended& operator=(const FileBodyExtended&) = delete;

        protected:
            FileBodyExtended(Core::BinairySemaphore* semLock, const string& path)
                : Web::FileBody(path, false)
                , _semLock(*semLock)
            {
                // Already exist, due to unexpected termination!
                if (true == Core::File::Exists()) {
                    // Close and remove file
                    Core::File::Destroy();
                }

                Core::File::Create();
            }

        public:
            virtual ~FileBodyExtended()
            {
                Core::File::Destroy();

                // Signal, It is ready for new capture
                _semLock.Unlock();
            }

            static Core::ProxyType<FileBodyExtended> Instance(Core::BinairySemaphore& semLock, const string& fileName)
            {
                Core::ProxyType<FileBodyExtended> result;

                if (semLock.Lock(0) == Core::ERROR_NONE) {
                    // We got the lock, forward it to the filebody
                    result = Core::ProxyType<FileBodyExtended>::Create(&semLock, fileName);
                }

                return (result);
            }

        private:
            Core::BinairySemaphore& _semLock;
        };

    private:
        Core::ProxyType<FileBodyExtended> _file;
    };

} // namespace Plugin
}
