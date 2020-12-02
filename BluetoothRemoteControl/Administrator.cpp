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

class Key {
public:
    Key() = delete;
    Key(const Key&) = delete;
    Key& operator= (const Key&) = delete;

    Key(const TCHAR name[], const Exchange::IVoiceProducer::IProfile::codec codec) 
        : _name(name)
        , _codec(codec) {
    }
    ~Key() = default;

public:
    bool operator< (const Key& RHS) const {
        return ((_codec == RHS._codec) ? _name < RHS._name : _codec < RHS._codec);
    }
    bool operator== (const Key& RHS) const {
        return ((_codec == RHS._codec) && (_name == RHS._name));
    }
    bool operator!= (const Key& RHS) const {
        return (!operator==(RHS));
    }
  
private:
    const string _name;
    Exchange::IVoiceProducer::IProfile::codec _codec;
};

static std::map<Key, IDecoder::IFactory*> _factories;

/* static */ IDecoder* IDecoder::Instance(const TCHAR name[], Exchange::IVoiceProducer::IProfile::codec codec, const string& configuration)
{
    IDecoder* result = nullptr;
    std::map<Key, IDecoder::IFactory*>::iterator index = _factories.find(Key(name, codec));
    if (index != _factories.end()) {
        result = index->second->Factory(configuration);
    }
    return (result);
}

/* static */ void IDecoder::Announce(const TCHAR name[], Exchange::IVoiceProducer::IProfile::codec codec, IDecoder::IFactory* factory)
{
    // uses pair's piecewise constructor
    _factories.emplace(std::piecewise_construct,
              std::forward_as_tuple(name, codec),
              std::forward_as_tuple(factory));
}

} } // namespace
