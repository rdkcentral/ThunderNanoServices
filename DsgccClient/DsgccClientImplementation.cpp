#include "DsgccClientImplementation.h"
#include <iomanip>
#include <sstream>
#include <iostream>
#include <fstream>

#define CLIENTPORTKEEPALIVE 50
#define MAXLINE 4096

extern "C" {
#include "refsw/BcmSharedMemory.h"
#include "refsw/dsgccClientCallback_rpcServer.h"
#include "refsw/dsgccClientRegistration_rpcClient.h"

void dsgcc_ClientNotification(struct dsgccClientNotification* clientNotification);
extern char* TunnelStatusTypeName(unsigned int value);
}

namespace WPEFramework {
namespace Plugin {

    DsgccClientImplementation::DsgccClientImplementation()
        : _service(nullptr)
        , _state(IDsgccClient::Unknown)
        , _observers()
        , _siThread(this)
        , _caThread(_config)
        , _callback(nullptr)
        //, _dsgCallback()
    {
    }

    void DsgccClientImplementation::Callback(IDsgccClient::INotification* callback)
    {
        if (callback != nullptr) {
            ASSERT(_callback == nullptr);   // Never registered but got Unregister
            _callback = callback;
            _callback->AddRef();
        } else {
            if(_callback != nullptr) {
                _callback->Release();
            }
            _callback = nullptr;
        }
    }

    uint32_t DsgccClientImplementation::Configure(PluginHost::IShell* service)
    {
        uint32_t result = 0;
        ASSERT(service != nullptr);

        _service = service;
        _config.FromString(service->ConfigLine());

        _siThread.Run();
        if (_config.DsgCaId)
            _caThread.Run();

        //_dsgCallback.Run();

        return (result);
    }

    void DsgccClientImplementation::DsgccClientSet(const string& str)
    {
        this->str = str;
    }

    string DsgccClientImplementation::GetChannels() const
    {
        return (_siThread.getChannels());
    }

    string DsgccClientImplementation::State() const
    {
        return Core::EnumerateType<IDsgccClient::state>(_state).Data() ;
    }

    void DsgccClientImplementation::Restart()
    {
        if (_siThread.State() != Core::Thread::RUNNING) {
            TRACE_L1("%s: Restarting SI Thread", __FUNCTION__);
            _siThread.Run();
        } else {
            TRACE_L1("%s: SI Thread already running", __FUNCTION__);
        }
    }

    DsgccClientImplementation::SiThread::SiThread(DsgccClientImplementation* parent)
        : Core::Thread(Core::Thread::DefaultStackSize(), _T("DsgSiThread"))
        , _parent(parent)
        , _config(parent->_config)
        , _isRunning(false)
        , _isInitialized(false)
    {
    }

    void DsgccClientImplementation::SiThread::Setup()
    {
        int retVal;

        CLIENT* handle = NULL;
        struct dsgClientRegInfo *regInfo = &regInfoData;

        if (!_isInitialized) {
            bzero((char *) regInfo, sizeof(struct dsgClientRegInfo));
            regInfo->clientPort = _config.DsgPort;
            regInfo->idType = _config.DsgType;
            regInfo->clientId = _config.DsgId;

            sharedMemoryId = BcmSharedMemoryCreate(regInfo->clientPort, 1);
            handle = clnt_create("127.0.0.1", DSGREGISTRAR, DSGREGISTRARVERS, "udp");
            if (handle) {
                regInfo->handle = (int)handle;

                TRACE_L1("registerdsgclient port=%d type=%d id=%d", regInfo->clientPort, regInfo->idType, regInfo->clientId);
                regInfo->idType |= 0x00010000;    // strip IP/UDP headers
                retVal = registerdsgclient(regInfo);

                if (retVal == DSGCC_SUCCESS) {
                    TRACE_L1("Tunnel request is registered successfully and tunnel is pending.");
                }

                retVal = dsgcc_KeepAliveClient(&regInfoData);
                if (retVal & DSGCC_CLIENT_REGISTERED) {
                    // Get tunnel status = MSB 16 bits
                    retVal >>= 16;
                    TRACE_L1("tunnel status %s", TunnelStatusTypeName(retVal));
                }
                _isInitialized = true;
            } else {
                TRACE_L1("Could not contact DSG Registrar");
            }
        }
    }

    DsgccClientImplementation::SiThread::~SiThread()
    {
        int rc = dsgcc_UnregisterClient(&regInfoData);
        TRACE_L1("Unregistering DsgCC client. rc=%d", rc);
        BcmSharedMemoryDelete(sharedMemoryId);
    }

    uint32_t DsgccClientImplementation::SiThread::Worker()
    {
        int retVal;
        char msg[MAXLINE];
        int len;
        DsgParser _parser(_config.VctId);
        _isRunning = true;

        Setup();
        LoadFromCache();
        while ( _isRunning ) {
            len = BcmSharedMemoryRead(sharedMemoryId, msg, 0);
            if (len > 0) {
                if (len >= _config.DsgSiHeaderSize) {
                    char *pBuf = &msg[_config.DsgSiHeaderSize];
                    len -= _config.DsgSiHeaderSize;

                    _parser.parse((unsigned char *) pBuf, len);
                    if (_parser.isDone())
                        _isRunning = false;
                }
            }

            if (len <= 0) {
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
        } // while

        string new_channels = _parser.getChannels();
        if (_channels.compare(new_channels) == 0) {
            TRACE_L1("No change in channel map");
        } else {
            _channels = new_channels;
            SaveToCache();
            _parent->StateChange(Exchange::IDsgccClient::Changed);
            TRACE_L1("Channel map has changed.  Size new=%d old=%d", new_channels.size(), _channels.size());
        }

        Suspend();
        TRACE_L1("Exiting %s state=%d", __PRETTY_FUNCTION__, Core::Thread::State());
        return (Core::infinite);
    }

    void DsgccClientImplementation::SiThread::LoadFromCache()
    {
        std::ifstream fs;
        fs.open (_config.DsgCacheFile.Value());
        if (fs.is_open()) {
            fs.seekg (0, fs.end);
            int fileSize = fs.tellg();
            fs.seekg (0, fs.beg);

            char* buffer = new char [fileSize];
            fs.read (buffer, fileSize);
            delete[] buffer;
            if (fs.gcount() == fileSize) {
                _channels = string (buffer, fileSize);
                _parent->StateChange(Exchange::IDsgccClient::Ready);
            } else {
                TRACE_L1("Failed to load full channel map. bytes read=%d fileSize=%d",  fs.gcount(), fileSize);
            }
            fs.close();
        }
    }

    void DsgccClientImplementation::SiThread::SaveToCache()
    {
        string filename = _config.DsgCacheFile.Value();
        if (filename.size()) {
            std::ofstream fs;
            fs.open (filename);
            if (fs.is_open()) {
                fs << _channels;
                fs.close();
            } else {
                TRACE_L1("Failed to save channels to %s", filename);
            }
        }
    }

    uint32_t DsgccClientImplementation::CaThread::Worker()
    {
        static struct dsgClientRegInfo regInfoData;

        struct dsgClientRegInfo *regInfo = &regInfoData;
        int retVal;
        char msg[MAXLINE];
        int len;
        int sharedMemoryId;

        CLIENT* handle = NULL;

        bzero((char *) regInfo, sizeof(struct dsgClientRegInfo));
        regInfo->clientPort = _config.DsgPort;
        regInfo->idType = _config.DsgCaType;
        regInfo->clientId = _config.DsgCaId;

        sharedMemoryId = BcmSharedMemoryCreate(regInfo->clientPort, 1);
        handle = clnt_create("127.0.0.1", DSGREGISTRAR, DSGREGISTRARVERS, "udp");
        if (handle == 0) {
            TRACE_L1("Could not contact DSG Registrar");
            return -1;
        }

        regInfo->handle = (int)handle;

        TRACE_L1("registerdsgclient port=%d type=%d id=%d", regInfo->clientPort, regInfo->idType, regInfo->clientId);
        //regInfo->idType |= 0x00010000;    // strip IP/UDP headers
        retVal = registerdsgclient(regInfo);

        if (retVal == DSGCC_SUCCESS) {
            TRACE_L1("Tunnel request is registered successfully and tunnel is pending.");
        }

        retVal = dsgcc_KeepAliveClient(&regInfoData);
        if (retVal & DSGCC_CLIENT_REGISTERED) {
            // Get tunnel status = MSB 16 bits
            retVal >>= 16;
            TRACE_L1("tunnel status %s", TunnelStatusTypeName(retVal));
        }

        while ( _isRunning ) {
            len = BcmSharedMemoryRead(sharedMemoryId, msg, 0);
            if (len > 0) {
                if (len >= _config.DsgCaHeaderSize) {
                    char *pBuf = &msg[_config.DsgCaHeaderSize];
                    len -= _config.DsgCaHeaderSize;
                    //TRACE_L1("CA Data: %p len=%d", pBuf, len);
                }
            }

            if (len <= 0) {
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
        } // while

        TRACE_L1("Unregistering DsgCC client.");
        retVal = dsgcc_UnregisterClient(regInfo);
        BcmSharedMemoryDelete(sharedMemoryId);

        TRACE_L1("Exiting %s state=%d", __PRETTY_FUNCTION__, Core::Thread::State());
        return (Core::infinite);
    }


    uint32_t DsgccClientImplementation::ClientCallbackService::Worker() {

        TRACE_L1("Starting ClientCallbackService::%s", __FUNCTION__);
        dsgClientCallbackSvcRun();
        return (Core::infinite);
    }

    void dsgcc_ClientNotification(struct dsgccClientNotification* clientNotification)
    {
        TRACE_L1("%s: NOTIFICATION from DSG-CC -> %s", __FUNCTION__, TunnelStatusTypeName(clientNotification->eventType));
    }

} // namespace Plugin
} // namespace WPEFramework
