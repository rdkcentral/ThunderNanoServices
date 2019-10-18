#pragma once

#include "Module.h"

#include <libdropbear.h>

namespace WPEFramework {
namespace Plugin {

    class DropbearServerImplementation {
    public:
        DropbearServerImplementation() = default;
        ~DropbearServerImplementation() = default;

        uint32_t StartService(const std::string& argv)
        {
            uint32_t result = Core::ERROR_NONE;

	    // TODO: Currently these options are static and need to pass required parameters from JSON-RPC
	    char* argv1[] = {
        		"DropbearServer",
		        "-R",
		        "-p",
		        "8099"
	    };

            TRACE(Trace::Information, (_T("Starting Dropbear Service with %s port."), argv.c_str()));
	    activate_dropbear(4, argv1);

            return result;
        }

        uint32_t StopService()
        {
            uint32_t result = Core::ERROR_NONE;

            TRACE(Trace::Information, (_T("Stoping Dropbear Service")));
	    deactivate_dropbear();

            return result;
        }

    private:
        DropbearServerImplementation(const DropbearServerImplementation&) = delete;
        DropbearServerImplementation& operator=(const DropbearServerImplementation&) = delete;
    };

} // namespace Plugin
} // namespace WPEFramework
