#pragma once

#include "Module.h"

#ifndef __WINDOWS__
#include <syslog.h>
#endif

namespace WPEFramework {
namespace Plugin {

    class TraceOutput : public Trace::ITraceMedia {
    private:
        TraceOutput(const TraceOutput&) = delete;
        TraceOutput& operator=(const TraceOutput&) = delete;

    public:
        TraceOutput(const bool syslogging)
            : _syslogging(syslogging)
        {
        }
        virtual ~TraceOutput()
        {
        }

    public:
        virtual void Output(const char fileName[], const uint32_t lineNumber, const char className[], const Trace::ITrace* information)
        {
            // Time to printf...
            string time(Core::Time::Now().ToRFC1123(true));

#ifndef __WINDOWS__
            if (_syslogging == true) {
                syslog(LOG_NOTICE, "[%s]:[%s:%d] %s: %s\n", time.c_str(), Core::FileNameOnly(fileName), lineNumber, information->Category(), information->Data());
            } else
#endif
            {
                printf("[%s]:[%s:%d] %s: %s\n", time.c_str(), Core::FileNameOnly(fileName), lineNumber, information->Category(), information->Data());
            }
        }

    private:
        bool _syslogging;
    };
}
}
