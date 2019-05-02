#ifndef __DSGCCCLIENTIMPLEMENTATION_H
#define __DSGCCCLIENTIMPLEMENTATION_H

#include "Module.h"
#include <interfaces/IMemory.h>

#include <refsw/dsgcc_client_api.h>
#include "DsgParser.h"

namespace WPEFramework {
namespace Plugin {

    class DsgccClientImplementation
        : public Exchange::IDsgccClient
    {
    private:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
            {
                Add(_T("dsgPort"), &DsgPort);
                Add(_T("dsgType"), &DsgType);
                Add(_T("dsgId"), &DsgId);
                Add(_T("dsgCaType"), &DsgCaType);
                Add(_T("dsgCaId"), &DsgCaId);
                Add(_T("vctId"), &VctId);
                Add(_T("dsgSiHeaderSize"), &DsgSiHeaderSize);
                Add(_T("dsgCaHeaderSize"), &DsgCaHeaderSize);
                Add(_T("dsgCacheFile"), &DsgCacheFile);

            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt16 DsgPort;
            Core::JSON::DecUInt16 DsgType;
            Core::JSON::DecUInt16 DsgId;
            Core::JSON::DecUInt16 DsgCaType;
            Core::JSON::DecUInt16 DsgCaId;
            Core::JSON::DecUInt16 VctId;
            Core::JSON::DecUInt16 DsgSiHeaderSize;
            Core::JSON::DecUInt16 DsgCaHeaderSize;
            Core::JSON::String DsgCacheFile;
        };

        class SiThread : public Core::Thread {

        public:
            SiThread(DsgccClientImplementation* parent);
            virtual ~SiThread();
            void Setup();

            string getChannels() const {
                return _channels;
            }

            void Dispose()
            {
                TRACE_L1("SiThread::%s: Done!!! ", __FUNCTION__);
            }
        private:
            SiThread(const SiThread&) = delete;
            SiThread& operator=(const SiThread&) = delete;
            uint32_t Worker() override;
            void LoadFromCache();
            void SaveToCache();

        private:
            DsgccClientImplementation* _parent;
            DsgccClientImplementation::Config& _config;
            bool _isRunning;
            bool _isInitialized;
            string _channels;
            struct dsgClientRegInfo regInfoData;
            int sharedMemoryId;
        };

        class CaThread : public Core::Thread {
        private:
            CaThread(const CaThread&) = delete;
            CaThread& operator=(const CaThread&) = delete;

        public:
            CaThread(DsgccClientImplementation::Config& config)
                : Core::Thread(Core::Thread::DefaultStackSize(), _T("DsgCaThread"))
                , _config(config)
                , _isRunning(true)
                , _isInitialized(false) {
            }

            virtual ~CaThread()
            {
            }

        public:
            void Dispose()
            {
                TRACE_L1("CaThread::%s: Done!!! ", __FUNCTION__);
            }

        private:
            DsgccClientImplementation::Config& _config;
            bool _isRunning;
            bool _isInitialized;
            string _channels;

            uint32_t Worker() override;
        };

        class ClientCallbackService : public Core::Thread {
        private:
            ClientCallbackService(const ClientCallbackService&) = delete;
            ClientCallbackService& operator=(const ClientCallbackService&) = delete;

        public:
            ClientCallbackService()
                : Core::Thread(Core::Thread::DefaultStackSize(), _T("DsgClientCallbackService"))
            {
            }
            virtual ~ClientCallbackService()
            {
            }

        public:
            void Dispose()
            {
                TRACE_L1("%s: Done!!! ", __FUNCTION__);
            }

        private:
            uint32_t Worker() override;
        };

    private:
        DsgccClientImplementation(const DsgccClientImplementation&) = delete;
        DsgccClientImplementation& operator=(const DsgccClientImplementation&) = delete;

    public:
        DsgccClientImplementation();
        virtual ~DsgccClientImplementation()
        {
        }

        void Callback(IDsgccClient::INotification* callback);
        uint32_t Configure(PluginHost::IShell* service);
        void DsgccClientSet(const string& str);
        string GetChannels() const;
        string State() const;
        void Restart();

        void StateChange(IDsgccClient::state state)
        {
            _state = state;
            if (_callback) {
                _callback->StateChange(_state);
            }
        }

        BEGIN_INTERFACE_MAP(DsgccClientImplementation)
        INTERFACE_ENTRY(Exchange::IDsgccClient)
        END_INTERFACE_MAP

    private:
        PluginHost::IShell* _service;
        IDsgccClient::state _state;
        std::list<PluginHost::IStateControl::INotification*> _observers;
        Config _config;
        SiThread _siThread;
        CaThread _caThread;
        IDsgccClient::INotification* _callback;
        ClientCallbackService _dsgCallback;
        string str;
    };

    SERVICE_REGISTRATION(DsgccClientImplementation, 1, 0);

} // namespace Plugin
} // namespace WPEFramework

#endif // __DSGCCCLIENTIMPLEMENTATION_H
