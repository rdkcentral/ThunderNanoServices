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

#include "ThunderInputManager.h"
#include "ThunderVoiceHandler.h"
#include <interfaces/IAVSClient.h>

#if defined(KWD_PRYON)
#include <AVS/KWD/AbstractKeywordDetector.h>
#endif

#include <AVS/SampleApp/SampleApplication.h>

#include <vector>

namespace WPEFramework {
namespace Plugin {

    class AVSDevice
        : public Exchange::IAVSClient,
          private alexaClientSDK::sampleApp::SampleApplication {
    public:
        AVSDevice()
            : _service(nullptr)
            , m_thunderInputManager(nullptr)
            , m_thunderVoiceHandler(nullptr)
        {
        }

        AVSDevice(const AVSDevice&) = delete;
        AVSDevice& operator=(const AVSDevice&) = delete;
        virtual ~AVSDevice() = default;

    private:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Audiosource()
                , AlexaClientConfig()
                , LogLevel()
                , KWDModelsPath()
                , EnableKWD()
            {
                Add(_T("audiosource"), &Audiosource);
                Add(_T("alexaclientconfig"), &AlexaClientConfig);
                Add(_T("loglevel"), &LogLevel);
                Add(_T("kwdmodelspath"), &KWDModelsPath);
                Add(_T("enablekwd"), &EnableKWD);
            }

            ~Config() = default;

        public:
            Core::JSON::String Audiosource;
            Core::JSON::String AlexaClientConfig;
            Core::JSON::String LogLevel;
            Core::JSON::String KWDModelsPath;
            Core::JSON::Boolean EnableKWD;
        };

    public:
        bool Initialize(PluginHost::IShell* service, const string& configuration) override;
        bool Deinitialize() override;
        Exchange::IAVSController* Controller() override;
        void StateChange(PluginHost::IShell* audioSource) override;

        BEGIN_INTERFACE_MAP(AVSDevice)
        INTERFACE_ENTRY(Exchange::IAVSClient)
        END_INTERFACE_MAP

    private:
        bool Init(const std::string& audiosource, const bool enableKWD, const std::string& pathToInputFolder);
        bool InitSDKLogs(const string& logLevel);
        bool JsonConfigToStream(std::vector<std::shared_ptr<std::istream>>& streams, const std::string& configFile);

    private:
        PluginHost::IShell* _service;
        std::shared_ptr<ThunderInputManager> m_thunderInputManager;
        std::shared_ptr<ThunderVoiceHandler<alexaClientSDK::sampleApp::InteractionManager>> m_thunderVoiceHandler;
#if defined(KWD_PRYON)
        std::unique_ptr<alexaClientSDK::kwd::AbstractKeywordDetector> m_keywordDetector;
#endif
    };

}
}
