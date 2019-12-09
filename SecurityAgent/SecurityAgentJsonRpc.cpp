
#include "Module.h"
#include "SecurityAgent.h"
#include <interfaces/json/JsonData_SecurityAgent.h>

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::SecurityAgent;

    // Registration
    //

    void SecurityAgent::RegisterAll()
    {
        Register<CreatetokenParamsData,CreatetokenResultInfo>(_T("createtoken"), &SecurityAgent::endpoint_createtoken, this);
        Register<CreatetokenResultInfo,ValidateResultData>(_T("validate"), &SecurityAgent::endpoint_validate, this);
    }

    void SecurityAgent::UnregisterAll()
    {
        Unregister(_T("validate"));
        Unregister(_T("createtoken"));
    }

    // API implementation
    //

    // Method: createtoken - Creates Token
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_GENERAL: Token creation failed
    uint32_t SecurityAgent::endpoint_createtoken(const CreatetokenParamsData& params, CreatetokenResultInfo& response)
    {
        uint32_t result = Core::ERROR_NONE;

        string token, payload;
        params.ToString(payload);

        if (CreateToken(static_cast<uint16_t>(payload.length()), reinterpret_cast<const uint8_t*>(payload.c_str()), token) == Core::ERROR_NONE) {
            response.Token = token;
        } else {
            result = Core::ERROR_GENERAL;
        }

        return result;
    }

    // Method: validate - Creates Token
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t SecurityAgent::endpoint_validate(const CreatetokenResultInfo& params, ValidateResultData& response)
    {
        uint32_t result = Core::ERROR_NONE;
        const string& token = params.Token.Value();
        response.Valid = false;

        Web::JSONWebToken webToken(Web::JSONWebToken::SHA256, sizeof(_secretKey), _secretKey);
        uint16_t load = webToken.PayloadLength(token);

        // Validate the token
        if (load != static_cast<uint16_t>(~0)) {
            // It is potentially a valid token, extract the payload.
            uint8_t* payload = reinterpret_cast<uint8_t*>(ALLOCA(load));

            load = webToken.Decode(token, load, payload);

             if (load != static_cast<uint16_t>(~0)) {
                response.Valid = true;
            }
        }

        return result;
    }

} // namespace Plugin

}

