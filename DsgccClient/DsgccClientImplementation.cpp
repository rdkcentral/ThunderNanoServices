#include <sstream>
#include <iomanip>
#include "DsgccClientImplementation.h"

#include <refsw/dsgcc_client_api.h>
#define CLIENTPORTKEEPALIVE 50
#define MAXLINE     4096

extern "C" {
#include "refsw/BcmSharedMemory.h"
#include "refsw/dsgccClientRegistration_rpcClient.h"
#include "refsw/dsgccClientCallback_rpcServer.h"

void dsgcc_ClientNotification(struct dsgccClientNotification *clientNotification);
extern char* TunnelStatusTypeName( unsigned int value );
}


namespace WPEFramework {
namespace Plugin {

    DsgccClientImplementation::DsgccClientImplementation()
        : _observers()
        , _worker(config)
        , _dsgCallback()
    {
    }

    uint32_t DsgccClientImplementation::Configure(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        config.FromString(service->ConfigLine());

        uint32_t result = 0;
        _worker.Run();
        //_dsgCallback.Run();

        return (result);
    }

    void DsgccClientImplementation::DsgccClientSet(const string& str)
    {
        this->str = str;
    }
    string DsgccClientImplementation::DsgccClientGet() const
    {
        return (str);
    }

    void DsgccClientImplementation::Activity::HexDump(const char* label, const std::string& msg, uint16_t charsPerLine)
    {
        std::stringstream ssHex, ss;
        for (int32_t i = 0; i < msg.length(); i++) {
            int byte = (uint8_t)msg.at(i);
            ssHex << std::setfill('0') << std::setw(2) << std::hex <<  byte << " ";
            ss << char((byte < ' ') ? '.' : byte);

            if (!((i+1) % charsPerLine)) {
                TRACE(Trace::Information, ("%s: %s %s\n", label, ssHex.str().c_str(), ss.str().c_str()));
                ss.str(std::string());
                ssHex.str(std::string());
            }
        }
        TRACE(Trace::Information, ("%s: %s %s\n", label, ssHex.str().c_str(), ss.str().c_str()));
    }

    uint32_t DsgccClientImplementation::Activity::Worker() {
        Setup(_config.DsgPort, _config.DsgType, _config.DsgId);
        return (Core::infinite);
    }

    void DsgccClientImplementation::Activity::Setup(unsigned int port, unsigned int dsgType, unsigned int dsgId)
    {
        static struct dsgClientRegInfo regInfoData;

        struct dsgClientRegInfo *regInfo = &regInfoData;
        int n;
        socklen_t       len;
        int retVal;
        char            mesg[MAXLINE];
        int sharedMemoryId;

        CLIENT *handle = NULL;

        bzero((char *) regInfo, sizeof(struct dsgClientRegInfo));
        _isRunning = true;

        regInfo->clientPort = port;

        sharedMemoryId = BcmSharedMemoryCreate(regInfo->clientPort, 1);
        TRACE_L1("sharedMemoryId=%x", sharedMemoryId);

        handle = clnt_create("127.0.0.1", DSGREGISTRAR, DSGREGISTRARVERS, "udp");
        if (handle == 0) {
          TRACE_L1("Could not contact DSG Registrar");
          return;
        }

        regInfo->handle = (int)handle;
        regInfo->idType = dsgType;
        regInfo->clientId = dsgId;
        regInfo->idType |= 0x00010000;    // strip IP/UDP headers

        TRACE_L1("registerdsgclient port=%d type=%d id=%d", regInfo->clientPort, regInfo->idType, regInfo->clientId);
        retVal = registerdsgclient(regInfo);

        if( retVal == DSGCC_SUCCESS ) {
            TRACE_L1("Tunnel request is registered successfully and tunnel is pending.");
        }

        retVal = dsgcc_KeepAliveClient(&regInfoData);
        if(retVal & DSGCC_CLIENT_REGISTERED) {
            // Get tunnel status = MSB 16 bits
            retVal >>= 16;
            TRACE_L1("tunnel status %s", TunnelStatusTypeName(retVal));
        }

        while ( _isRunning ) {
            n = BcmSharedMemoryRead(sharedMemoryId, mesg, 0);
            if (n > 0) {
                HexDump("DSG:", string(mesg,n));
                process((unsigned char *)mesg, n);
            }

            if (n <= 0) {
                if (CLIENTPORTKEEPALIVE) {
                    if (errno == EWOULDBLOCK) { // XXX:
                    retVal = dsgcc_KeepAliveClient(&regInfoData);
                        if(retVal & DSGCC_CLIENT_REGISTERED) {
                        retVal >>= 16;
                            TRACE_L1("tunnel status %s", TunnelStatusTypeName(retVal));
                    }
                  }
                }
            }
        }

        retVal = dsgcc_UnregisterClient(regInfo);
        BcmSharedMemoryDelete(sharedMemoryId);
    }

    void DsgccClientImplementation::Activity::process(unsigned char *pBuf, ssize_t len)
    {
        int section_len;
        section_len = ((pBuf[1] & 0xf) << 8) | pBuf[2];

        switch (pBuf[0]) {
        case 0xc2:
            printf("Network Information Table \n");
            if (section_len < 8) {
              printf("short NIT, got %d bytes (expected >7)\n", section_len);
            } else if (1 == (pBuf[6] & 0xf)) {
               printf(" Network Information Table  : CDS\n"); //1 CDS  Carrier Definition Subtable
               //cdsDone = parse_cds(pBuf, section_len, &cds);
            } else if (2 == (pBuf[6] & 0xf)) {
               printf(" Network Information Table  : MMS\n"); //2 MMS  Modulation Mode Subtable
               //mmsDone = parse_mms(pBuf, section_len, &mms);
            } else {
               printf("NIT tbl_subtype=%s\n", (pBuf[6] & 0xf) ? "RSVD" : "INVLD");  //3-15 Reserved, 0 invalid
            }
            break;
        case 0xc3:
            printf(" Network text Table\n");
            //if(!nttDone)
            //  nttDone = parse_ntt(pBuf, section_len, &ntt);
            break;
        case 0xc4:
            printf(" Short-form Virtual Channel Table\n");
            //if(!svctDone)
            //  svctDone = parse_svct(pBuf, section_len, &vcm_list, vctid, vct_lookup_index);
            break;
        case 0xc5:
            printf(" System Time Table \n");
            if (section_len >= 10) {
                char buff[60];
            time_t stt = ((pBuf[5]<<24) | (pBuf[6]<<16) | (pBuf[7]<<8) | pBuf[8]) + 315964800;
            printf("Current Time : %s\n", ctime(&stt));
            struct tm * timeinfo;
            timeinfo =  localtime (&stt);
            strftime(buff, 60, "%Y-%m-%d %H:%M:%S", timeinfo);
            printf("System Time Table thinks it is %s\n", buff);

            char setdate[60];
            sprintf(setdate,"date -s '%s'", buff);
            system( setdate);
            printf("System Date Set Successfully\n");
            }
            break;
        default:
            printf("Unknown section 0x%x len=%d\n", pBuf[0], len);
            break;
        }

    }
    uint32_t DsgccClientImplementation::ClientCallbackService::Worker() {
        TRACE_L1("Starting ClientCallbackService::%s", __FUNCTION__);
        dsgClientCallbackSvcRun();
        return (Core::infinite);
    }

    void dsgcc_ClientNotification(struct dsgccClientNotification *clientNotification)
    {
       TRACE_L1("%s: NOTIFICATION from DSG-CC -> %s", __FUNCTION__, TunnelStatusTypeName(clientNotification->eventType));
    }

} // namespace Plugin
} // namespace WPEFramework


