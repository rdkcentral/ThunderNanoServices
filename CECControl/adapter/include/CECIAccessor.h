#pragma once

#include <CECIAdapterAccessor.h>
#include <CECTypes.h>

#include <string>

namespace WPEFramework {
namespace CEC {
    class Adapter;

    struct EXTERNAL IAccessor {
        static IAccessor* Instance();

        virtual uint32_t Announce(const std::string& id, const std::string& config) = 0;
        virtual uint32_t Revoke(const std::string& id) = 0;

        // TODO: Keep Adapter objects intern... and give IAdapterAccessor* instead
        virtual Adapter GetAdapter(const string& id, const role_t role) = 0;
        //virtual IAdapterAccessor* GetAdapter(const string& id, const role_t role) = 0;

        virtual ~IAccessor() = default;
    }; // struct IAccessor
} // namespace CEC
} // namespace WPEFramework