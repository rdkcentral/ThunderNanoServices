#pragma once

#include "Module.h"

namespace WPEFramework {

namespace Decoders {

    struct IDecoder {
        virtual ~IDecoder() {}

        struct IFactory {
            virtual ~IFactory() {}
            virtual IDecoder* Factory(const string& configuration) = 0;
        };

        static void Announce(Exchange::IVoiceProducer::codec codec, IFactory* factory);
        static IDecoder* Instance(Exchange::IVoiceProducer::codec codec, const string& configuration);

        virtual uint32_t Frames() const = 0;
        virtual uint32_t Dropped() const = 0;

        virtual void Reset() = 0;
        virtuak uint16_t Decode (const uint16_t lengthIn, const uint8_t dataIn, const uint16_t lengthOut, uint8_t dataOut) = 0;
    };

    template<typename DECODER>
    class Factory : public IDecoder::IFactory {
    public:
        Factory(const Factory<DECODER>&) = delete;
        Factory<DECODER>& operator&(const FACTORY<DECODER>&) = delete;
        Factory() {
            IDecoder::Announce(DECODER::DecoderType, this);
        }
        virtual ~Factory() {
        }
        virtual IDecoder* Factory(const string& configuration) override
        {
            return (new DECODER(configuration));
        }
    };

} } // namespace WPEFramework::Decoders
