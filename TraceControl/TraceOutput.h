#ifndef __TRACEOUTPUT_H
#define __TRACEOUTPUT_H

#include "Module.h"

#ifndef __WIN32__
#include <syslog.h>
#endif

namespace WPEFramework {
namespace Plugin {

    class TraceOutput : public Trace::ITraceMedia {
    private:
    TraceOutput(const TraceOutput&) = delete;
    TraceOutput& operator= (const TraceOutput&) = delete;

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

#ifndef __WIN32__
        if (_syslogging == true) {
            syslog (LOG_NOTICE, "[%s]:[%s:%d]:[%s] %s: %s\n",time.c_str(), Core::FileNameOnly(fileName), lineNumber, Core::ClassNameOnly(className).Data(), information->Category(), information->Data());
        } else
#endif
        {
            printf ("[%s]:[%s:%d]:[%s] %s: %s\n",time.c_str(), Core::FileNameOnly(fileName), lineNumber, Core::ClassNameOnly(className).Data(), information->Category(), information->Data());
        }
    }

    private:
    bool _syslogging;
    };
}
}

#endif // __TRACEOUTPUT_H
