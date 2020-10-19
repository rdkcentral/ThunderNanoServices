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

#include <interfaces/IVoiceHandler.h>

namespace WPEFramework {

namespace Decoders {

    struct IDecoder {
        virtual ~IDecoder() {}

        struct IFactory {
            virtual ~IFactory() {}
            virtual IDecoder* Factory(const string& configuration) = 0;
        };

        static void Announce(const TCHAR name[], Exchange::IVoiceProducer::IProfile::codec codec, IFactory* factory);
        static IDecoder* Instance(const TCHAR name[], Exchange::IVoiceProducer::IProfile::codec codec, const string& configuration);

        virtual uint32_t Frames() const = 0;
        virtual uint32_t Dropped() const = 0;

        virtual void Reset() = 0;
        virtual uint16_t Decode (const uint16_t lengthIn, const uint8_t dataIn[], const uint16_t lengthOut, uint8_t dataOut[]) = 0;
    };

    template<typename DECODER>
    class DecoderFactory : public IDecoder::IFactory {
    public:
        DecoderFactory(const DecoderFactory<DECODER>&) = delete;
        DecoderFactory<DECODER>& operator&(const DecoderFactory<DECODER>&) = delete;
        DecoderFactory() {

            IDecoder::Announce(DECODER::Name, DECODER::DecoderType, this);
        }

        virtual ~DecoderFactory() {
        }

        virtual IDecoder* Factory(const string& configuration) override
        {
            return (new DECODER(configuration));
        }
    };

} } // namespace WPEFramework::Decoders
