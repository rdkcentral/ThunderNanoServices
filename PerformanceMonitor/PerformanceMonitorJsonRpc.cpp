#include "Module.h"
#include "PerformanceMonitor.h"
#include <interfaces/json/JsonData_PerformanceMonitor.h>

namespace WPEFramework {
namespace Plugin {

    using namespace JsonData::PerformanceMonitor;

    // Registration
    //

    void PerformanceMonitor::RegisterAll()
    {
        Register<void,void>(_T("clear"), &PerformanceMonitor::endpoint_clear, this);
        Register<BufferInfo,Core::JSON::DecUInt32>(_T("send"), &PerformanceMonitor::endpoint_send, this);
        Register<Core::JSON::DecUInt32,BufferInfo>(_T("receive"), &PerformanceMonitor::endpoint_receive, this);
        Register<BufferInfo,BufferInfo>(_T("exchange"), &PerformanceMonitor::endpoint_exchange, this);
        Property<MeasurementData>(_T("measurement"), &PerformanceMonitor::get_measurement, nullptr, this);
    }

    void PerformanceMonitor::UnregisterAll()
    {
        Unregister(_T("send"));
        Unregister(_T("receive"));
        Unregister(_T("exchange"));
        Unregister(_T("clear"));
        Unregister(_T("measurement"));
    }

    // API implementation
    //

    // Method: clear - Clear all performance data collected
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t PerformanceMonitor::endpoint_clear()
    {
        PluginHost::PerformanceAdministrator::Instance().Clear();
        return Core::ERROR_NONE;
    }

    // Method: send - Interface to test sending data
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t PerformanceMonitor::endpoint_send(const BufferInfo& params, Core::JSON::DecUInt32& response)
    {
        return Send(params, response);
    }

    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t PerformanceMonitor::endpoint_receive(const Core::JSON::DecUInt32& params, BufferInfo& response)
    {
        return Receive(params, response);
    }

    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t PerformanceMonitor::endpoint_exchange(const BufferInfo& params, BufferInfo& response)
    {
        return Exchange(params, response);
    }

    // Property: measurement - Retrieve the performance measurement against given package size
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t PerformanceMonitor::get_measurement(const string& index, MeasurementData& response) const
    {
        const uint32_t& packageSize = atoi(index.c_str());
        return RetrieveInfo(packageSize, response);
    }

} // namespace Plugin
}

