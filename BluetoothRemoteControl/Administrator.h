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

        static void Announce(Exchange::IVoiceProducer::IProfile::codec codec, IFactory* factory);
        static IDecoder* Instance(Exchange::IVoiceProducer::IProfile::codec codec, const string& configuration);

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
            IDecoder::Announce(DECODER::DecoderType, this);
        }

        virtual ~DecoderFactory() {
        }

        virtual IDecoder* Factory(const string& configuration) override
        {
            return (new DECODER(configuration));
        }
    };

} } // namespace WPEFramework::Decoders
