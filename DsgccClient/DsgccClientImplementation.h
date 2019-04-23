#ifndef __DSGCCCLIENTIMPLEMENTATION_H
#define __DSGCCCLIENTIMPLEMENTATION_H

#include "Module.h"
#include <interfaces/IDsgccClient.h>
#include <interfaces/IMemory.h>

#include "DsgParser.h"

namespace WPEFramework {
namespace Plugin {

    class DsgccClientImplementation : public Exchange::IDsgccClient {
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
        };

        class SiThread : public Core::Thread {
        private:
            SiThread(const SiThread&) = delete;
            SiThread& operator=(const SiThread&) = delete;

        public:
            SiThread(DsgccClientImplementation::Config& config)
                : Core::Thread(Core::Thread::DefaultStackSize(), _T("DsgccClient"))
                , _config(config)
                , _isRunning(true) {
            }

            virtual ~SiThread()
            {
            }

            string getChannels() const {
                return _channels;
            }

        public:
            void Dispose()
            {
                TRACE_L1("SiThread::%s: Done!!! ", __FUNCTION__);
            }

        private:
            DsgccClientImplementation::Config& _config;
            bool _isRunning;
            string _channels;

            virtual uint32_t Worker() override;
            void Setup(unsigned int port, unsigned int dsgType, unsigned int dsgId);
            void HexDump(const char* label, const std::string& msg, uint16_t charsPerLine = 32);
        };

        class CaThread : public Core::Thread {
        private:
            CaThread(const CaThread&) = delete;
            CaThread& operator=(const CaThread&) = delete;

        public:
            CaThread(DsgccClientImplementation::Config& config)
                : Core::Thread(Core::Thread::DefaultStackSize(), _T("DsgccClient"))
                , _config(config)
                , _isRunning(true) {
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
            string _channels;

            virtual uint32_t Worker() override;
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

        uint32_t Configure(PluginHost::IShell* service);
        void DsgccClientSet(const string& str);
        string GetChannels() const;

        BEGIN_INTERFACE_MAP(DsgccClientImplementation)
        INTERFACE_ENTRY(Exchange::IDsgccClient)
        END_INTERFACE_MAP

    private:
        std::list<PluginHost::IStateControl::INotification*> _observers;
        Config _config;
        SiThread _siThread;
        CaThread _caThread;
        ClientCallbackService _dsgCallback;
        string str;
    };

    SERVICE_REGISTRATION(DsgccClientImplementation, 1, 0);

} // namespace Plugin
} // namespace WPEFramework

#endif // __DSGCCCLIENTIMPLEMENTATION_H
