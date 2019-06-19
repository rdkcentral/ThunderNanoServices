#ifndef IdentityProvider_IdentityProvider_H
#define IdentityProvider_IdentityProvider_H

#include "Module.h"

namespace WPEFramework {
namespace Plugin {
    class IdentityProvider : public PluginHost::ISubSystem::IIdentifier {
    public:
        IdentityProvider();
        
        virtual ~IdentityProvider(){
            if (_identifier != nullptr) {
                delete (_identifier);
            }
        };

        IdentityProvider(const IdentityProvider&) = delete;
        IdentityProvider& operator=(const IdentityProvider&) = delete;

        BEGIN_INTERFACE_MAP(IdentityProvider)
            INTERFACE_ENTRY(PluginHost::ISubSystem::IIdentifier)
        END_INTERFACE_MAP

        virtual uint8_t Identifier(const uint8_t length, uint8_t buffer[]) const override{
            uint8_t result = 0;

            if (_identifier != nullptr) {
                result = _identifier[0];
                ::memcpy(buffer, &(_identifier[1]), (result > length ? length : result));
            }

            return (result);
        }         
    private:
        uint8_t* _identifier;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // IdentityProvider_IdentityProvider_H