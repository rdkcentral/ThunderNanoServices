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

#pragma once

#include "Module.h"
#include <errno.h>

#include <linux/types.h>
#include <linux/watchdog.h>
#include <sys/ioctl.h>

namespace Thunder {
namespace Plugin {

    // API: https://www.kernel.org/doc/html/latest/watchdog/watchdog-api.html
    // inspriration:
    // https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/tools/testing/selftests/watchdog/watchdog-test.c
    // https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/samples/watchdog/watchdog-simple.c

    class Doggo : public PluginHost::IPlugin {
    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Device("/dev/watchdog")
                , Interval(30)
                , Margin(5)
            {
                Add(_T("device"), &Device);
                Add(_T("interval"), &Interval);
                Add(_T("margin"), &Margin);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Device;
            Core::JSON::DecUInt16 Interval; // in seconds
            Core::JSON::DecUInt8 Margin; // in seconds
        };

        class FeedingTime : public Core::IDispatchType<void> {
        private:
            FeedingTime() = delete;
            FeedingTime(const FeedingTime&) = delete;
            FeedingTime& operator=(const FeedingTime&) = delete;

        public:
            FeedingTime(Doggo* dog)
                : _dog(*dog)
            {
                ASSERT(parent != nullptr);
            }
            ~FeedingTime()
            {
            }

        public:
            virtual void Dispatch() override
            {
                _dog.Feed();
            }

        private:
            Doggo& _dog;
        };

        Doggo(const Doggo&) = delete;
        Doggo& operator=(const Doggo&) = delete;

        Doggo()
            : _watchdogFd(0)
            , _feedingTime(Core::ProxyType<FeedingTime>::Create(this))
        {
        }

        ~Doggo() override
        {
        }

        BEGIN_INTERFACE_MAP(Doggo)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    private:
        void Feed()
        {
            int ret;

            TRACE(Trace::Information, (_T("Feeding the watchdog...")));

            ret = ioctl(_watchdogFd, WDIOC_KEEPALIVE, 0);

            if (!ret) {
                Core::Time timestamp(Core::Time::Now());
                timestamp.Add(_interval * 1000);
                Core::IWorkerPool::Instance().Schedule(timestamp, _feedingTime);

                TRACE(Trace::Information, (_T("Schedule next feeding time in %ds"), _interval));
            } else {
                TRACE(Trace::Error, (_T("WDIOC_KEEPALIVE error '%s'\n"), strerror(errno)));
            }
        }

        /*
        // TODO: maybe nice to report but WDIOC_GETBOOTSTATUS is not 100% supported by all drivers
        void Reason(&string reason)
        {
            int ret, flags;

            ret = ioctl(_watchdogFd, WDIOC_GETBOOTSTATUS, &flags);

            if (!ret) {
                reason = (flags != 0) ? "Watchdog" : "Power-On-Reset";
            } else {
                reason = string();
                TRACE(Trace::Error, (_T("WDIOC_GETBOOTSTATUS error '%s'\n"), strerror(errno)));
            }
        }
        */

        void Kill()
        {
            const char v = 'V';
            write(_watchdogFd, &v, 1);
        }

    private:
        int _watchdogFd;
        uint16_t _interval; // in seconds
        Core::ProxyType<Core::IDispatchType<void>> _feedingTime;
    };
} // namespace Plugin
} // namespace Thunder
