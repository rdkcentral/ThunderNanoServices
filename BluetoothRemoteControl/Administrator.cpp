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
