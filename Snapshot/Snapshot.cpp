
#include "Snapshot.h"

#include <png.h>

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(Snapshot, 1, 0);

    class StoreImpl: public Exchange::ICapture::IStore {
    private:
        StoreImpl() = delete;
        StoreImpl(const StoreImpl&) = delete;
        StoreImpl& operator=(const StoreImpl&) = delete;

    public:
        StoreImpl (Core::BinairySemaphore& inProgress, const string& path)
                : _file(FileBodyExtended::Instance(inProgress, path))
        {
        }

        virtual ~StoreImpl()
        {
        }

        virtual bool R8_G8_B8_A8(const unsigned char *buffer, const unsigned int width, const unsigned int height)
        {

            png_structp pngPointer = nullptr;
            bool result = false;

            pngPointer = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (pngPointer == nullptr) {

                return result;
            }

            png_infop infoPointer = nullptr;
            infoPointer = png_create_info_struct(pngPointer);
            if (infoPointer == nullptr) {

                png_destroy_write_struct(&pngPointer, &infoPointer);
                return result;
            }

            // Set up error handling.
            if (setjmp(png_jmpbuf(pngPointer))) {

                png_destroy_write_struct(&pngPointer, &infoPointer);
                return result;
            }

            // Set image attributes.
            int depth = 8;
            png_set_IHDR(pngPointer,
                         infoPointer,
                         width,
                         height,
                         depth,
                         PNG_COLOR_TYPE_RGB,
                         PNG_INTERLACE_NONE,
                         PNG_COMPRESSION_TYPE_DEFAULT,
                         PNG_FILTER_TYPE_DEFAULT);

            // Initialize rows of PNG.
            png_byte** rowLines = static_cast<png_byte**>(png_malloc(pngPointer, height * sizeof(png_byte*)));
            const int pixelSize = 4; // RGBA
            for (unsigned int i = 0; i < height; ++i) {

                png_byte* rowLine = static_cast<png_byte*>(png_malloc(pngPointer, sizeof(png_byte) * width * pixelSize));
                const uint8_t *rowSource = buffer + (sizeof(png_byte) * i * width * pixelSize);
                rowLines[i] = rowLine;
                for (unsigned int j = 0; j < width*pixelSize; j+=pixelSize) {
                    *rowLine++ = rowSource[j+2]; // Red
                    *rowLine++ = rowSource[j+1]; // Green
                    *rowLine++ = rowSource[j+0]; // Blue
                    // ignore alpha
                }
            }

            // Duplicate file descriptor and create File stream based on it.
            FILE* filePointer = static_cast<FILE*>(*_file);
            if (nullptr != filePointer) {
                // Write the image data to "file".
                png_init_io(pngPointer, filePointer);
                png_set_rows(pngPointer, infoPointer, rowLines);
                png_write_png(pngPointer, infoPointer, PNG_TRANSFORM_IDENTITY, nullptr);
                // All went well.
                result = true;
            }

            // Close stream to flush and release allocated buffers
            fclose(filePointer);

            for (unsigned int i = 0; i < height; i++) {
                png_free(pngPointer, rowLines[i]);
            }

            png_free(pngPointer, rowLines);
            png_destroy_write_struct(&pngPointer, &infoPointer);

            return result;
        }

        operator Core::ProxyType<Web::IBody>()
        {

            return Core::ProxyType<Web::IBody>(*_file);
        }

        bool IsValid()
        {
            return (_file.IsValid());
        }

        class FileBodyExtended : public Web::FileBody {
        private:
            FileBodyExtended() = delete;
            FileBodyExtended(const FileBodyExtended&) = delete;
            FileBodyExtended& operator=(const FileBodyExtended&) = delete;

        protected:
            FileBodyExtended(Core::BinairySemaphore *semLock, const string& path)
                    : Web::FileBody(path, false)
                    , _semLock(*semLock)
            {
                // Already exist, due to unexpected termination!
                if (true == Core::File::Exists())
                {
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

        public:
            static Core::ProxyType<FileBodyExtended> Instance(Core::BinairySemaphore& semLock, const string& fileName)
            {
                Core::ProxyType<FileBodyExtended> result;

                if (semLock.Lock(0) == Core::ERROR_NONE)
                {
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

    /* virtual */ const string Snapshot::Initialize(PluginHost::IShell* service)
    {
        string result;

        // Capture PNG file name
        ASSERT(service->PersistentPath() != _T(""));
        ASSERT(_device == nullptr);

        Core::Directory directory(service->PersistentPath().c_str());
        if (directory.CreatePath()) {
           _fileName = service->PersistentPath() + string("Capture.png");
        }
        else {
            _fileName = string("/tmp/Capture.png");
        }

        // Setup skip URL for right offset.
        _skipURL = service->WebPrefix().length();

        // Get producer
        _device = Exchange::ICapture::Instance();

        if (_device != nullptr) {
            TRACE_L1(_T("Capture device: %s"), _device->Name());
        }
        else {
            result = string("No capture device is registered");
        }

        return (result);
    }

    /* virtual */ void Snapshot::Deinitialize(PluginHost::IShell* service)
    {

        ASSERT(_device != nullptr);

        if (_device != nullptr) {
            _device->Release();
            _device = nullptr;
        }
    }

    /* virtual */ string Snapshot::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ void Snapshot::Inbound(Web::Request& /* request */)
    {
    }

    /* virtual */ Core::ProxyType<Web::Response> Snapshot::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        // Proxy object for response type.
        Core::ProxyType<Web::Response> response(PluginHost::Factories::Instance().Response());

        // Default is not allowed.
        response->Message = _T("Method not allowed");
        response->ErrorCode = Web::STATUS_METHOD_NOT_ALLOWED;

        // Decode request path.
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

        // Get first plugin verb.
        index.Next();

        // Move to next segment if it exists.
        if (request.Verb == Web::Request::HTTP_GET) {

            if (false == index.Next()) {

                response->Message = _T("Plugin is up and running");
                response->ErrorCode = Web::STATUS_OK;
            }
            else if ( (index.Current() == "Capture") ) {

                StoreImpl file(_inProgress, _fileName);

                // _inProgress event is signalled, capture screen
                if (file.IsValid() == true) {

                    if (_device->Capture(file)) {

                        // Attach to response.
                        response->ContentType = Web::MIMETypes::MIME_IMAGE_PNG;
                        response->Body(static_cast<Core::ProxyType<Web::IBody>>(file));
                        response->Message = string(_device->Name());
                        response->ErrorCode = Web::STATUS_ACCEPTED;
                    } else {
                        response->Message = _T("Could not create a capture on ") + string(_device->Name());
                        response->ErrorCode = Web::STATUS_PRECONDITION_FAILED;
                    }
                } else {
                    response->Message = _T("Plugin is already in progress");
                    response->ErrorCode = Web::STATUS_PRECONDITION_FAILED;
                }
            }
        }

        return (response);
    }

}
}
