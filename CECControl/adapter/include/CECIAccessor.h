#pragma once

#include <CECIAdapterAccessor.h>
#include <CECTypes.h>

#include <string>

namespace Thunder {
namespace CEC {
    class Adapter;

    struct EXTERNAL IAccessor {
        static IAccessor* Instance();

        virtual uint32_t Announce(const std::string& id, const std::string& config) = 0;
        virtual uint32_t Revoke(const std::string& id) = 0;

        // TODO: Keep Adapter objects intern... and give IAdapterAccessor* instead
        virtual Adapter GetAdapter(const string& id, const cec_adapter_role_t role) = 0;
        //virtual IAdapterAccessor* GetAdapter(const string& id, const cec_adapter_role_t role) = 0;

        virtual ~IAccessor() = default;
    }; // struct IAccessor
} // namespace CEC
} // namespace Thunder