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
 
#include "FileTransfer.h"

namespace WPEFramework {
namespace Plugin {

    namespace {

        static Metadata<FileTransfer> metadata(
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


    const string FileTransfer::Initialize(PluginHost::IShell* service)
    {
        Config config;
        config.FromString(service->ConfigLine());

        _logOutput.SetDestination(config.Destination.Binding.Value(), config.Destination.Port.Value());
        _observer.Register(config.FilePath.Value(), &_fileUpdate, config.FullFile.Value());

        return string();
    }

    void FileTransfer::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        _observer.Unregister();
    }

    string FileTransfer::Information() const
    {
        return string();
    }
} // namespace Plugin
} // namespace WPEFramework
