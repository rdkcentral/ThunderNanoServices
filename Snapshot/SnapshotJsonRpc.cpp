
#include "Module.h"
#include "Snapshot.h"
#include "StoreImpl.h"
#include <interfaces/json/JsonData_Snapshot.h>

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::Snapshot;

    // Registration
    //

    void Snapshot::RegisterAll()
    {
        Register<void,CaptureResultData>(_T("capture"), &Snapshot::endpoint_capture, this);
    }

    void Snapshot::UnregisterAll()
    {
        Unregister(_T("capture"));
    }

    // API implementation
    //

    // Method: capture - Takes a screen capture
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Failed to perform the screen capture
    //  - ERROR_INPROGRESS: Capture already in progress
    uint32_t Snapshot::endpoint_capture(CaptureResultData& response)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;
        StoreImpl file(_inProgress, _fileName);

        if (file.IsValid() == true) {
            if (_device->Capture(file) == true) {
                if (file.Size() > 0) {
                    const uint64_t size = file.Size();
                    const uint64_t encodedSize = ((4 * size / 3) + 3) & ~3;
                    uint8_t *buffer = new uint8_t[base64size];
                    if (buffer != nullptr) {
                        string image;
                        file.Serialize(buffer, size);
                        Core::ToString(buffer, encodedSize, true, image);
                        response.Image = image;
                        response.Device = _device->Name();
                        result = Core::ERROR_NONE;
                        delete[] buffer;
                    }
                }
            }
        } else {
            result = Core::ERROR_INPROGRESS;
        }

        return result;
    }

} // namespace Plugin

}

