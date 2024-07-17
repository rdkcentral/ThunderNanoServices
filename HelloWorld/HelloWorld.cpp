#include "HelloWorld.h"

namespace Thunder{
namespace Plugin{

    
    namespace {

        static Metadata<HelloWorld> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {subsystem::PLATFORM},
            // Terminations
            {},
            // Controls
            {}
        );
    }
    


const string HelloWorld::Initialize(PluginHost::IShell* service){

    std::string message;

    ASSERT(_service == nullptr);
    ASSERT(service != nullptr);
    //ASSERT(_impl == nullptr);

    _service = service;
    _service->AddRef();

    std::cout << "pls work" << std::endl;
    
    _impl = _service->Root<Exchange::IHelloWorld>(_connectionId, 2000, _T("HelloWorld"));
        if (_impl == nullptr) {
            message = _T("Couldn't create volume control instance");
        } else {
          //_impl->Register();
          Exchange::JHelloWorld::Register(*this, this);
        }
    //TRACE(TRACE::Initialisation, (_T("Initialising HelloWorld Plugin")));
    
   // Exchange::JHelloWorld::Register(*this, _implementation);
    
    return message;

}

void HelloWorld::Deinitialize(PluginHost::IShell* service){

    if(_service != nullptr){
        ASSERT(_service == service);
    }


    if( _impl != nullptr){
         Exchange::JHelloWorld::Unregister(*this);
        //_impl = nullptr;
    }

    std::cout << "Deinitialized" << std::endl;

    _service -> Release();
    
    _service = nullptr;
    

}

string HelloWorld::Information() const{
    return string();
};

uint32_t HelloWorld::PrintStuff(const string &randomWord) const {
    std::cout << randomWord << std::endl;
    return (Core::ERROR_NONE);
}


} // Plugin
} // Thunder