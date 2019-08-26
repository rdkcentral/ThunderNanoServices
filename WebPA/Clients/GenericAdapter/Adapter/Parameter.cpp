#include "Parameter.h"

namespace WPEFramework {
namespace WebPA {

Parameter::Parameter(Handler* handler, WalDB* walDB)
    : _walDB(walDB)
    , _handler(handler)
    , _adminLock()
{
}

Parameter::~Parameter()
{
}
const void Parameter::Values(const std::vector<std::string>& parameterNames, std::map<std::vector<Data>, WebPAStatus>& parametersList) const
{
    for (auto& name: parameterNames) {
        std::vector<Data> parameters;

        WebPAStatus ret = Values(name, parameters);
        parametersList.insert(std::make_pair(parameters, ret));
        if ((ret == WEBPA_SUCCESS) && (parameters.size() > 0)) {
            TRACE(Trace::Information, (_T( "Parameter Name: %s return: %d"), name.c_str(), parameters.size()));
        } else {
            TRACE(Trace::Information, (_T( "Parameter Name: %s return no value, so keeping empty values to get the status")));
        }
    }
}

WebPAStatus Parameter::Values(const std::vector<Data>& parameters, std::vector<WebPAStatus>& status)
{
    WebPAStatus ret = WEBPA_SUCCESS;
    for (uint16_t i = 0; i < parameters.size(); ++ i) {

        ret = Values(parameters[i]);
        status[i] = ret;

    }
    return ret;
}

const WebPAStatus Parameter::Values(const std::string& parameterName, std::vector<Data>& parameters) const
{
    WebPAStatus status = WEBPA_FAILURE; // Overall get status

    int32_t dbHandle = _walDB->DBHandle();

    if (dbHandle) {
        if (Utils::IsWildCardParam(parameterName)) { // It is a wildcard Param
            /* Translate wildcard to list of parameters */
            std::map<uint32_t, std::pair<std::string, std::string>> dbParamters;
            DBStatus dbRet = _walDB->Parameters(parameterName, dbParamters);
            if (dbRet == DB_SUCCESS && dbParamters.size() > 0) {
                for (auto&  dbParamter:  dbParamters) {
                    Variant value(Utils::ConvertToParamType(dbParamter.second.second));
                    Data param(dbParamter.second.first, value);

                    _adminLock.Lock();
                    WebPAStatus ret = Utils::ConvertFaultCodeToWPAStatus((static_cast<const Handler&>(*_handler)).Parameter(param));
                    _adminLock.Unlock();

                    // Fill Only if we can able to get Proper value
                    if (WEBPA_SUCCESS == ret) {
                        parameters.push_back(param);
                        status = WEBPA_SUCCESS; //Set status as success, if there is atleast one parameter
                    }
                } // End of Wild card for loop
            } else {
                TRACE(Trace::Error, (_T( " Wild card Param list is empty")));
                status = WEBPA_FAILURE;
            }
            dbParamters.clear();

        } else { // Not a wildcard Parameter Lets fill it
            TRACE(Trace::Information, (_T( "Get Request for a Non-WildCard Parameter")));
            std::string dataType;

            if (_walDB->IsValidParameter (parameterName, dataType)) {
                TRACE(Trace::Information, (_T( "Valid Parameter..! ")));
                Variant value(Utils::ConvertToParamType(dataType));
                Data param(parameterName, value);

                // Convert param.paramType to ParamVal.type
                TRACE(Trace::Information, (_T( " Values parameterType is %d"), dataType));
                _adminLock.Lock();
                status = Utils::ConvertFaultCodeToWPAStatus((static_cast<const Handler&>(*_handler)).Parameter(param));
                _adminLock.Unlock();
                if (WEBPA_SUCCESS == status) {
                    parameters.push_back(param);
                } else {
                    TRACE(Trace::Error, (_T( "Failed Get Param Values From Handler: for Param Name :-  %s"), parameterName));
                }
            } else {
                TRACE(Trace::Error, (_T( "Invalid Parameter Name  :-  %s"), parameterName));
                status = WEBPA_ERR_INVALID_PARAMETER_NAME;
            }
        }
    } else {
        TRACE(Trace::Error, (_T( "Data base Handle is not Initialized %s"), parameterName));
    }
    return status;
}

WebPAStatus Parameter::Values(const Data& parameter)
{
    WebPAStatus ret = WEBPA_FAILURE;

    int32_t dbHandle = _walDB->DBHandle();

    if (dbHandle) {

        std::string dataType;
        if (_walDB->IsValidParameter(parameter.Name(), dataType)) {
            if (Utils::ConvertToParamType(dataType) == parameter.Value().Type()) {

                _adminLock.Lock();
                ret = Utils::ConvertFaultCodeToWPAStatus(_handler->Parameter(parameter));
                _adminLock.Unlock();
                TRACE(Trace::Information, (_T("handler::Parameter %d"), ret));
            } else {
                ret = WEBPA_ERR_INVALID_PARAMETER_TYPE;
            }
        } else {
            TRACE(Trace::Error, (_T(" Invalid Parameter name %s"), parameter.Name()));
            ret = WEBPA_ERR_INVALID_PARAMETER_NAME;
        }
    }
    return ret;
}

} // WebPA
} // WPEFramework
