#include "Administrator.h"

namespace WPEFramework {

namespace Decoders {

static std::map<Exchange::IVoiceProducer::codec, IDecoder::IFactory*> _factories;

/* static */ IDecoder* IDecoder::Instance(Exchange::IVoiceProducer::codec codec, const string& configuration)
{
    IDecoder* result = nullptr;
    std::map<Exchange::IVoiceProducer::codec, IDecoder::IFactory*>::iterator index = _factories.find(codec);
    if (index != _factories.end()) {
        result = index->second->Factory(configuration);
    }
    return (result);
}

/* static */ void IDecoder::Announce(Exchange::IVoiceProducer::codec codec, IDecoder::IFactory* factory)
{
    _factories.insert(std::pair<Exchange::IVoiceProducer::codec, IDecoder::IFactory*>(codec, factory));
}

} } // namespace
