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
        Property<MeasurementData>(_T("measurement"), &PerformanceMonitor::get_measurement, nullptr, this);
    }

    void PerformanceMonitor::UnregisterAll()
    {
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

