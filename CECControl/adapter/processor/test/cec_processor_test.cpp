#ifndef MODULE_NAME
#define MODULE_NAME CECProcessorTest
#endif

#include <CECOperationFrame.h>
#include <CECProcessor.h>

#include <localtracer/localtracer.h>
#include <messaging/messaging.h>

#include <chrono>
#include <core/core.h>
#include <stdarg.h>
#include <thread>
#include <time.h>

using namespace Thunder::CEC;

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

int main(int /*argc*/, const char* argv[])
{
    {
        Messaging::LocalTracer& tracer = Messaging::LocalTracer::Open();
        Messaging::ConsolePrinter printer(false);

        tracer.Callback(&printer);
        // tracer.EnableMessage("CECMessageProcessor", "Information", true);
        // tracer.EnableMessage("CECMessageProcessor", "Error", true);
        // tracer.EnableMessage("CECProcessorTest", "Information", true);
        // tracer.EnableMessage("CECProcessorTest", "Error", true);

        TRACE(Trace::Information, ("%s - build %s", argv[0], __TIMESTAMP__));

        // these messages need to be awnsered with no feature abort.
        std::list<opcode_type> mms = {
            GET_CEC_VERSION,
            GET_MENU_LANGUAGE,
            GIVE_DEVICE_VENDOR_ID,
            GIVE_OSD_NAME,
            GIVE_PHYSICAL_ADDR,
            GIVE_DEVICE_POWER_STATUS,
            STANDBY,
        };

        std::list<opcode_type>::const_iterator ci;

        for (ci = mms.cbegin(); ci != mms.cend(); ci++) {
            OperationFrame operation;
            bool broadcast;

            operation.OpCode(*ci);

            string sdata;
            Core::ToHexString(operation.Data(), operation.Size(), sdata);

            Processor::Instance().Process(operation, broadcast);

            TRACE(Trace::Information, ("OpCode 0x%02x [%d] %s. raw data: \'%s\'", *ci, broadcast ? "Broadcast" : "Addressed", (operation.OpCode() != FEATURE_ABORT) ? "OK" : "NOK", sdata.c_str()));
        }

        tracer.Close();

        Thunder::Core::Singleton::Dispose();
    }

    return 0;
}