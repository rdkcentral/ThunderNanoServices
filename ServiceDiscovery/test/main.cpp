#include "../DNS.h"
#include <core/core.h>

using namespace WPEFramework::Plugin;
using namespace WPEFramework;

int main(int argc, char* argv[])
{
    uint8_t DNSPackage[] = "\xba\x92\x01\x00\x00\x01\x00\x00\x00\x00\x00\x01\x02\x68\x61\x06"
                           "\x62\x79\x62\x72\x61\x6d\x03\x63\x6f\x6d\x00\x00\x1c\x00\x01\x00"
                           "\x00\x29\x05\xc0\x00\x00\x00\x00\x00";

    DNS::BaseFrame frame(DNSPackage, sizeof(DNSPackage));

    std::string loadedData;
    Core::ToHexString(frame.Data(), frame.Size(), loadedData);
    printf("Data[%d]: %s\n", frame.Size(), loadedData.c_str());

    ASSERT(sizeof(DNSPackage) == frame.Size());

    uint16_t id = frame.TransactionId();
    ASSERT(id == 0xba92);

    bool q = frame.IsQuery();
    ASSERT(q == true);

    bool tc = frame.IsTruncated();
    ASSERT(tc == false);

    bool aa = frame.IsAuthoritativeAnswer();
    ASSERT(aa == false);

    DNS::OperationCode oc = frame.Operation();
    ASSERT(oc == DNS::OperationCode::Query);

    DNS::ResponseCode rc = frame.Response();
    ASSERT(rc == DNS::ResponseCode::NoError);

    Core::Singleton::Dispose();

    return EXIT_SUCCESS;
}