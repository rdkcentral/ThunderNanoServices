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
        Property<MeasurementData>(_T("measurement"), &PerformanceMonitor::get_measurement, nullptr, this);
    }

    void PerformanceMonitor::UnregisterAll()
    {
        Unregister(_T("measurement"));
    }

    // API implementation
    //

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

