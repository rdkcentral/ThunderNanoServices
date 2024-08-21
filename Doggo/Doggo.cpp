/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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

#include "Doggo.h"

namespace Thunder {
namespace Plugin {

    namespace {

        static Metadata<Doggo> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

    const string Doggo::Initialize(PluginHost::IShell* service)
    {
        ASSERT(_service == nullptr);

        string result;
        struct watchdog_info info;
        int ret;

        Config config;
        config.FromString(service->ConfigLine());

        _watchdogFd = open(config.Device.Value().c_str(), O_WRONLY);

        if (_watchdogFd == -1) {
            if (errno == ENOENT) {
                result = "Watchdog device (" + config.Device.Value() + ") not found.";
            } else if (errno == EACCES) {
                result = "Run watchdog as root";
            } else {
                result = "Watchdog device open failed" + string(strerror(errno));
            }
        } else {
            /* Validate that `_watchdogFd` is a watchdog device */
            ret = ioctl(_watchdogFd, WDIOC_GETSUPPORT, &info);

            if (!ret) {
                if (config.Interval.Value() > 0) {
                    int timeout(config.Interval.Value() + config.Margin.Value()); // Add some margin to the reset timeout.
                    ret = ioctl(_watchdogFd, WDIOC_SETTIMEOUT, &timeout);

                    Feed(); // Start feeding the dog.
                } else {
                    Kill();

                    close(_watchdogFd);

                    result = "Interval too small, disabled watchdog";
                }
            } else {
                result = "WDIOC_GETSUPPORT error: " + string(strerror(errno));

                close(_watchdogFd);
            }
        }

        return result;
    }

    void Doggo::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        Kill();

        Core::IWorkerPool::Instance().Revoke(_feedingTime);

        close(_watchdogFd);
    }

    string Doggo::Information() const
    {
        return string();
    }
} // namespace Plugin
} // namespace Thunder
