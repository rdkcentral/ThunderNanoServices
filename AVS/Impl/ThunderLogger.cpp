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

#include "ThunderLogger.h"

#include "TraceCategories.h"

namespace Thunder {
namespace Plugin {

    // using namespace Thunder;
    using namespace alexaClientSDK::avsCommon::utils;
    using namespace alexaClientSDK::avsCommon::utils::logger;

    static const std::string CONFIG_KEY_DEFAULT_LOGGER = "thunderLogger";

    std::shared_ptr<Logger> ThunderLogger::instance()
    {
        static std::shared_ptr<Logger> singleThunderLogger = std::shared_ptr<ThunderLogger>(new ThunderLogger);
        return singleThunderLogger;
    }

    ThunderLogger::ThunderLogger()
        : Logger(Level::UNKNOWN)
    {
        init(configuration::ConfigurationNode::getRoot()[CONFIG_KEY_DEFAULT_LOGGER]);
    }

    void ThunderLogger::Log(const std::string& stringToPrint)
    {
        TRACE_GLOBAL(AVSClient, (stringToPrint.c_str()));
    }

    void ThunderLogger::Log(std::initializer_list<std::string> lines)
    {
        for (const auto& line : lines) {
            Log(line);
        }
    }

    void ThunderLogger::emit(
        Level level,
        std::chrono::system_clock::time_point time VARIABLE_IS_NOT_USED,
        const char* threadMoniker,
        const char* text)
    {

        std::stringstream ss;
        ss << "[" << threadMoniker << "] " << convertLevelToChar(level) << " " << text;
        TRACE(AVSSDK, (ss.str().c_str()));
    }

} // namespace Plugin
} // namespace Thunder

namespace alexaClientSDK {
namespace avsCommon {
    namespace utils {
        namespace logger {

            std::shared_ptr<alexaClientSDK::avsCommon::utils::logger::Logger> getThunderLogger()
            {
                return Thunder::Plugin::ThunderLogger::instance();
            }

        } // namespace logger
    } // namespace utils
} // namespace avsCommon
} // namespace alexaClientSDK
