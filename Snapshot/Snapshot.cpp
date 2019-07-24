
#include "Snapshot.h"
#include "StoreImpl.h"
#include "Module.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(Snapshot, 1, 0);

    /* virtual */ const string Snapshot::Initialize(PluginHost::IShell* service)
    {
        string result;

        // Capture PNG file name
        ASSERT(service->PersistentPath() != _T(""));
        ASSERT(_device == nullptr);

        Core::Directory directory(service->PersistentPath().c_str());
        if (directory.CreatePath()) {
            _fileName = service->PersistentPath() + string("Capture.png");
        } else {
            _fileName = string("/tmp/Capture.png");
        }

        // Setup skip URL for right offset.
        _skipURL = service->WebPrefix().length();

        // Get producer
        _device = Exchange::ICapture::Instance();

        if (_device != nullptr) {
            TRACE_L1(_T("Capture device: %s"), _device->Name());
        } else {
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
            } else if ((index.Current() == "Capture")) {

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
