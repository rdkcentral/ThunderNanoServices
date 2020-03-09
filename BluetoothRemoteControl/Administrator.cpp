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

#include "Administrator.h"

namespace WPEFramework {

namespace Decoders {

static std::map<Exchange::IVoiceProducer::IProfile::codec, IDecoder::IFactory*> _factories;

/* static */ IDecoder* IDecoder::Instance(Exchange::IVoiceProducer::IProfile::codec codec, const string& configuration)
{
    IDecoder* result = nullptr;
    std::map<Exchange::IVoiceProducer::IProfile::codec, IDecoder::IFactory*>::iterator index = _factories.find(codec);
    if (index != _factories.end()) {
        result = index->second->Factory(configuration);
    }
    return (result);
}

/* static */ void IDecoder::Announce(Exchange::IVoiceProducer::IProfile::codec codec, IDecoder::IFactory* factory)
{
    _factories.insert(std::pair<Exchange::IVoiceProducer::IProfile::codec, IDecoder::IFactory*>(codec, factory));
}

} } // namespace
