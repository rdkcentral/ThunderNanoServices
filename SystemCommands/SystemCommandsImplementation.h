/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#pragma once

#include "Module.h"

#include <linux/usbdevice_fs.h>

namespace WPEFramework {
namespace Plugin {

    class SystemCommandsImplementation {
    public:
        SystemCommandsImplementation() = default;
        ~SystemCommandsImplementation() = default;

        uint32_t USBReset(const std::string& device)
        {
            uint32_t result = Core::ERROR_NONE;
            int fd = open(device.c_str(), O_WRONLY);
            if (fd < 0) {
                TRACE(Trace::Error, (_T("Opening of %s failed."), device.c_str()));
                result = Core::ERROR_GENERAL;
            } else {
                int rc = ioctl(fd, USBDEVFS_RESET, 0);
                if (rc < 0) {
                    TRACE(Trace::Error, (_T("ioctl(USBDEVFS_RESET) failed with %d. Errno: %d"), rc, errno));
                    result = Core::ERROR_GENERAL;
                }
                close(fd);
            }

            return result;
        }

    private:
        SystemCommandsImplementation(const SystemCommandsImplementation&) = delete;
        SystemCommandsImplementation& operator=(const SystemCommandsImplementation&) = delete;
    };

} // namespace Plugin
} // namespace WPEFramework
