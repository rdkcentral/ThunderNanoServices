 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include <AVS/AVSCommon/Utils/Logger/Logger.h>

#include <mutex>
#include <string>

namespace WPEFramework {
namespace Plugin {

    /**
     * Handles AVS SDK logs through Thunder Tracing
    */
    class ThunderLogger : public alexaClientSDK::avsCommon::utils::logger::Logger {
    public:
        ThunderLogger(const ThunderLogger&) = delete;
        ThunderLogger& operator=(const ThunderLogger&) = delete;

        static std::shared_ptr<alexaClientSDK::avsCommon::utils::logger::Logger> instance();

        static void Trace(const std::string& stringToPrint);
        static void PrettyTrace(const std::string& stringToPrint);
        static void PrettyTrace(std::initializer_list<std::string> lines);
        static void Log(const std::string& stringToPrint);
        static void Log(std::initializer_list<std::string> lines);
        void emit(alexaClientSDK::avsCommon::utils::logger::Level level, std::chrono::system_clock::time_point time,
            const char* threadMoniker, const char* text) override;

    private:
        ThunderLogger();
    };

} // namespace Plugin
} // namespace WPEFramework

namespace alexaClientSDK {
namespace avsCommon {
    namespace utils {
        namespace logger {

            std::shared_ptr<alexaClientSDK::avsCommon::utils::logger::Logger> getThunderLogger();

        } // namespace logger
    } // namespace utils
} // namespace avsCommon
} // namespace alexaClientSDK
