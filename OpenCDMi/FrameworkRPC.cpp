/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <regex>
#include <string>
#include <vector>

#include "Module.h"

// Get in the definitions required for access to the sepcific
// DRM engines.
#include <interfaces/IDRM.h>

// Get in the definitions required for access to the OCDM
// counter part living in the applications
#include <ocdm/DataExchange.h>
#include <ocdm/IOCDM.h>

#include <interfaces/IContentDecryption.h>

#include "CENCParser.h"

#include <ocdm/open_cdm.h>

extern "C" {

typedef ::CDMi::ISystemFactory* (*GetDRMSystemFunction)();
}

namespace WPEFramework {

namespace Plugin {

    static void TrimWs(const std::string& str, size_t& start, size_t& end)
    {
        while(std::isspace(str[start]) && start < end) {
            ++start;
        }

        while(std::isspace(str[end - 1]) && end - 1 > 0) {
         --end;
        }
    }

    static std::vector<std::string> Tokenize(const std::string& str, char token)
    {
        std::vector<std::string> tokens;
        size_t startPos = 0;
        size_t endPos = 0;
        do {
            endPos = str.find(token, startPos);
            if (endPos != std::string::npos) {
                size_t end = endPos;
                TrimWs(str, startPos, end);
                tokens.emplace_back(std::string(&str[startPos], end - startPos));
                startPos = endPos + 1;
            }
        } while (endPos != std::string::npos && startPos < str.length());
        size_t end = str.size();
        TrimWs(str, startPos, end);
        tokens.emplace_back(std::string(&str[startPos], end - startPos));
        return tokens;
    }

    static void ParseContentType(const std::string& contentType, std::string& mimeType, std::vector<std::string>& codecsList) {
        codecsList.clear();
        if (contentType.empty() == false) {
            std::smatch matches;
            const size_t kCaptureGroupsNumber = 4;
            std::regex expr("\\s*([a-zA-Z0-9\\-\\+]+/[a-zA-Z0-9\\-\\+]+)\\s*(;\\s*codecs\\*?\\s*=\\s*\"?([a-zA-Z0-9,\\s\\+\\-\\.']+)\"?\\s*)?");
            bool matched = std::regex_match(contentType, matches, expr, std::regex_constants::match_default);

            if (matched && matches.size() == kCaptureGroupsNumber) {
                ASSERT(matches[0] == contentType);
                mimeType = matches[1];
                if (matches[2].str().empty() == false) {
                    std::vector<std::string> codecs = Tokenize(matches[3], ',');
                    codecsList.swap(codecs);
                }
            }
        }
    }

    static const TCHAR BufferFileName[] = _T("ocdmbuffer.");

    class OCDMImplementation : public Exchange::IContentDecryption {
    private:
        OCDMImplementation(const OCDMImplementation&) = delete;
        OCDMImplementation& operator=(const OCDMImplementation&) = delete;

        struct SystemFactory {
            std::string Name;
            CDMi::ISystemFactory* Factory;
        };

        class ExternalAccess : public RPC::Communicator {
        private:
            ExternalAccess() = delete;
            ExternalAccess(const ExternalAccess&) = delete;
            ExternalAccess& operator=(const ExternalAccess&) = delete;

        public:
            ExternalAccess(
                const Core::NodeId& source, 
                ::OCDM::IAccessorOCDM* parentInterface, 
                const Core::ProxyType<RPC::InvokeServer> & engine)
                : RPC::Communicator(source, _T(""), Core::ProxyType<Core::IIPCServer>(engine))
                , _parentInterface(parentInterface)
            {
                engine->Announcements(Announcement());
                Open(Core::infinite);
            }
            ~ExternalAccess()
            {
                Close(Core::infinite);
            }

        private:
            virtual void* Aquire(const string& className, const uint32_t interfaceId, const uint32_t versionId)
            {
                void* result = nullptr;

                // Currently we only support version 1 of the IRPCLink :-)
                if (((versionId == 1) || (versionId == static_cast<uint32_t>(~0))) && ((interfaceId == ::OCDM::IAccessorOCDM::ID) || (interfaceId == Core::IUnknown::ID))) {
                    // Reference count our parent
                    _parentInterface->AddRef();
                    TRACE(Trace::Information, ("OCDM interface aquired => %p", this));
                    // Allright, respond with the interface.
                    result = _parentInterface;
                }
                return (result);
            }

        private:
            ::OCDM::IAccessorOCDM* _parentInterface;
        };

        class AccessorOCDM : public ::OCDM::IAccessorOCDM {
        private:
            AccessorOCDM() = delete;
            AccessorOCDM(const AccessorOCDM&) = delete;
            AccessorOCDM& operator=(const AccessorOCDM&) = delete;

            class BufferAdministrator {
            private:
                BufferAdministrator() = delete;
                BufferAdministrator(const BufferAdministrator&) = delete;
                BufferAdministrator& operator=(const BufferAdministrator&) = delete;

            public:
                BufferAdministrator(const string pathName)
                    : _adminLock()
                    , _basePath(Core::Directory::Normalize(pathName))
                    , _occupation(0)
                {
                }
                ~BufferAdministrator()
                {
                }

            public:
                bool AquireBuffer(string& locator)
                {
                    uint8_t index = 0;

                    locator.clear();

                    _adminLock.Lock();

                    while ((index < (sizeof(_occupation) * 8)) && ((_occupation & (1 << index)) != 0)) {
                        index++;
                    }

                    // We do not expect more that 4 buffers to be allocated concurrently,
                    // so more than X would be dramatic
                    ASSERT(index < (sizeof(_occupation) * 8));

                    if (index < (sizeof(_occupation) * 8)) {
                        _occupation |= (1 << index);
                        locator = _basePath + BufferFileName + Core::NumberType<uint8_t>(index).Text();
                    }

                    _adminLock.Unlock();

                    return (locator.empty() == false);
                }
                bool ReleaseBuffer(const string& locator)
                {

                    bool released = false;

                    if (locator.compare(0, _basePath.length(), _basePath) == 0) {
                        string actualFile(locator.substr(_basePath.length()));
                        uint8_t baseLength((sizeof(BufferFileName) / sizeof(TCHAR)) - 1);

                        if (actualFile.compare(0, baseLength, BufferFileName) == 0) {
                            // Than the last part is the number..
                            uint8_t number(Core::NumberType<uint8_t>(&(actualFile.c_str()[baseLength]), static_cast<uint32_t>(actualFile.length() - baseLength)).Value());

                            if (number <= (sizeof(_occupation) * 8)) {
                                _adminLock.Lock();

                                if ((_occupation & (1 << number)) != 0) {
                                    _occupation ^= (1 << number);
                                    released = true;
                                } else {
                                    // Freeing a buffer that is already free sounds dangerous !!!
                                    ASSERT(false);
                                }

                                _adminLock.Unlock();
                            }
                        }
                    }
                    return (released);
                }

            private:
                Core::CriticalSection _adminLock;
                string _basePath;
                uint16_t _occupation;
            };

            // IMediaKeys defines the MediaKeys interface.
            class SessionImplementation : public ::OCDM::ISession, public ::OCDM::ISessionExt {
            private:
                SessionImplementation() = delete;
                SessionImplementation(const SessionImplementation&) = delete;
                SessionImplementation& operator=(const SessionImplementation&) = delete;

                class DataExchange : public ::OCDM::DataExchange, public Core::Thread {
                private:
                    DataExchange() = delete;
                    DataExchange(const DataExchange&) = delete;
                    DataExchange& operator=(const DataExchange&) = delete;

                public:
                    DataExchange(CDMi::IMediaKeySession* mediaKeys, const string& name, const uint32_t defaultSize)
                        : ::OCDM::DataExchange(name, defaultSize)
                        , Core::Thread(Core::Thread::DefaultStackSize(), _T("DRMSessionThread"))
                        , _mediaKeys(mediaKeys)
                        , _mediaKeysExt(dynamic_cast<CDMi::IMediaKeySessionExt*>(mediaKeys))
                        , _sessionKey(nullptr)
                        , _sessionKeyLength(0)
                    {
                        Core::Thread::Run();
                        TRACE_L1("Constructing buffer server side: %p - %s", this, name.c_str());
                    }
                    ~DataExchange()
                    {
                        TRACE_L1("Destructing buffer server side: %p - %s", this, ::OCDM::DataExchange::Name().c_str());
                        // Make sure the thread reaches a HALT.. We are done.
                        Core::Thread::Stop();

                        // If the thread is waiting for a semaphore, fake a signal :-)
                        Produced();

                        Core::Thread::Wait(Core::Thread::STOPPED, Core::infinite);
                    }

                private:
                    virtual uint32_t Worker() override
                    {

                        while (IsRunning() == true) {

                            uint32_t clearContentSize = 0;
                            uint8_t* clearContent = nullptr;

                            RequestConsume(Core::infinite);

                            if (IsRunning() == true) {
                                uint8_t keyIdLength = 0;
                                const uint8_t* keyIdData = KeyId(keyIdLength);

                                int cr = _mediaKeys->Decrypt(
                                    _sessionKey,
                                    _sessionKeyLength,
                                    nullptr, //subsamples
                                    0, //number of subsamples
                                    IVKey(),
                                    IVKeyLength(),
                                    Buffer(),
                                    BytesWritten(),
                                    &clearContentSize,
                                    &clearContent,
                                    keyIdLength,
                                    keyIdData,
                                    InitWithLast15());
                                if ((cr == 0) && (clearContentSize != 0)) {
                                    if (clearContentSize != BytesWritten()) {
                                        TRACE_L1("Returned clear sample size (%d) differs from encrypted buffer size (%d)", clearContentSize, BytesWritten());
                                        Size(clearContentSize);
                                    }

                                    // Adjust the buffer on our sied (this process) on what we will write back
                                    SetBuffer(0, clearContentSize, clearContent);
                                }

                                // Store the status we have for the other side.
                                Status(static_cast<uint32_t>(cr));

                                // Whatever the result, we are done with the buffer..
                                Consumed();
                            }
                        }

                        return (Core::infinite);
                    }

                private:
                    CDMi::IMediaKeySession* _mediaKeys;
                    CDMi::IMediaKeySessionExt* _mediaKeysExt;
                    uint8_t* _sessionKey;
                    uint32_t _sessionKeyLength;
                };

                // IMediaKeys defines the MediaKeys interface.
                class Sink : public CDMi::IMediaKeySessionCallback {
                private:
                    Sink() = delete;
                    Sink(const Sink&) = delete;
                    Sink& operator=(const Sink&) = delete;

                public:
                    Sink(SessionImplementation* parent, ::OCDM::ISession::ICallback* callback)
                        : _parent(*parent)
                        , _callback(callback)
                    {
                        _callback->AddRef();
                    }
                    Sink(SessionImplementation* parent)
                        : _parent(*parent)
                        , _callback(nullptr)
                    {
                    }
                    virtual ~Sink()
                    {
                        if (_callback != nullptr) {
                            Revoke(_callback);
                        }
                    }

                public:
                    // Event fired when a key message is successfully created.
                    virtual void OnKeyMessage(
                        const uint8_t* f_pbKeyMessage, //__in_bcount(f_cbKeyMessage)
                        uint32_t f_cbKeyMessage, //__in
                        const char* f_pszUrl) override
                    {
                        TRACE(Trace::Information, ("OnKeyMessage(%s)", f_pszUrl));
                        if (_callback != nullptr) {
                            std::string url(f_pszUrl, strlen(f_pszUrl));
                            _callback->OnKeyMessage(f_pbKeyMessage, f_cbKeyMessage, url);
                        }
                    }

                    // Event fired when MediaKeySession encounters an error.
                    virtual void OnError(int16_t f_nError, CDMi::CDMi_RESULT f_crSysError, const char* errorMessage) override
                    {
                        TRACE(Trace::Information, ("OnKeyError(%d,%s)", f_nError, errorMessage));
                        if (_callback != nullptr) {
                            std::string message(errorMessage, strlen(errorMessage));
                            _callback->OnError(f_nError, (::OCDM::OCDM_RESULT)f_crSysError, message);
                        }
                    }

                    //Event fired on key status update
                    virtual void OnKeyStatusUpdate(const char* keyMessage, const uint8_t buffer[], const uint8_t length) override
                    {
                        ASSERT (buffer != nullptr);

                        ::OCDM::ISession::KeyStatus key;
                        CommonEncryptionData::KeyId keyId(CommonEncryptionData::COMMON, buffer, length);

                        TRACE(Trace::Information, ("OnKeyStatusUpdate(%s)", keyMessage));

                        if (::strcmp(keyMessage, "KeyUsable") == 0)
                            key = ::OCDM::ISession::Usable;
                        else if (::strcmp(keyMessage, "KeyReleased") == 0)
                            key = ::OCDM::ISession::Released;
                        else if (::strcmp(keyMessage, "KeyExpired") == 0)
                            key = ::OCDM::ISession::Expired;
                        else if (::strcmp(keyMessage, "KeyOutputRestricted") == 0)
                            key = ::OCDM::ISession::OutputRestricted;
                        else if (::strcmp(keyMessage, "KeyOutputDownscaled") == 0)
                            key = ::OCDM::ISession::OutputDownscaled;
                        else if (::strcmp(keyMessage, "SEC_RESULT_HW_FAILURE") == 0)
                            key = ::OCDM::ISession::HWError;
                        else if (::strcmp(keyMessage, "KeyOutputRestrictedHDCP22") == 0)
                            key = ::OCDM::ISession::OutputRestrictedHDCP22;
                        else
                            key = ::OCDM::ISession::InternalError;

                        const CommonEncryptionData::KeyId* updated = _parent._cencData.UpdateKeyStatus(key, keyId);

                        ASSERT (updated != nullptr);

                        if (_callback != nullptr) {
                            _callback->OnKeyStatusUpdate(updated->Id(), updated->Length(), key);
                        }
                    }
                    void Revoke(::OCDM::ISession::ICallback* callback)
                    {
                        if ((_callback != nullptr) && (_callback == callback)) {
                            _callback->Release();
                            _callback = nullptr;
                        } else {
                            TRACE_L1("Additional request for Revoking the callback!! %p", callback);
                        }
                    }

                    void OnKeyStatusesUpdated() const override
                    {
                        _callback->OnKeyStatusesUpdated();
                    }

                private:
                    SessionImplementation& _parent;
                    ::OCDM::ISession::ICallback* _callback;
                };

            public:
                #ifdef __WINDOWS__
                #pragma warning(disable : 4355)
                #endif
                SessionImplementation(
                    AccessorOCDM* parent,
                    const std::string keySystem,
                    CDMi::IMediaKeySession* mediaKeySession,
                    ::OCDM::ISession::ICallback* callback,
                    const string bufferName,
                    const uint32_t defaultSize,
                    const CommonEncryptionData* sessionData)
                    : _parent(*parent)
                    , _refCount(1)
                    , _keySystem(keySystem)
                    , _sessionId(mediaKeySession->GetSessionId())
                    , _mediaKeySession(mediaKeySession)
                    , _mediaKeySessionExt(dynamic_cast<CDMi::IMediaKeySessionExt*>(mediaKeySession))
                    , _sink(this, callback)
                    , _buffer(new DataExchange(mediaKeySession, bufferName, defaultSize))
                    , _cencData(*sessionData)
                {
                    ASSERT(parent != nullptr);
                    ASSERT(sessionData != nullptr);
                    ASSERT(_mediaKeySession != nullptr);

                    _mediaKeySession->Run(&_sink);
                    TRACE(Trace::Information, ("Server::Session::Session(%s,%s,%s) => %p", _keySystem.c_str(), _sessionId.c_str(), bufferName.c_str(), this));
                    TRACE_L1("Constructed the Session Server side: %p", this);
                }

                SessionImplementation(
                    AccessorOCDM* parent,
                    const std::string keySystem,
                    CDMi::IMediaKeySessionExt* mediaKeySession,
                    ::OCDM::ISession::ICallback* callback,
                    const string bufferName,
                    const uint32_t defaultSize,
                    const CommonEncryptionData* sessionData)
                    : _parent(*parent)
                    , _refCount(1)
                    , _keySystem(keySystem)
                    , _sessionId("")
                    , _mediaKeySession(dynamic_cast<CDMi::IMediaKeySession*>(mediaKeySession))
                    , _mediaKeySessionExt(mediaKeySession)
                    , _sink(this, callback)
                    , _buffer(new DataExchange(dynamic_cast<CDMi::IMediaKeySession*>(mediaKeySession), bufferName, defaultSize))
                    , _cencData(*sessionData)
                {
                    ASSERT(parent != nullptr);
                    ASSERT(sessionData != nullptr);
                    ASSERT(_mediaKeySession != nullptr);

                    // This constructor can only be used for extended OCDM sessions.
                    ASSERT(_mediaKeySessionExt != nullptr);

                    TRACE_L1("Constructed the Session Server side: %p", this);
                    _mediaKeySession->Run(&_sink);
                    TRACE_L1("Constructed the Session Server side: %p", this);
                }
                #ifdef __WINDOWS__
                #pragma warning(default : 4355)
                #endif

                virtual ~SessionImplementation()
                {

                    TRACE_L1("Destructing the Session Server side: %p", this);
                    // this needs to be done in a thread safe way. Leave it up to
                    // the parent to lock handing out new entries before we clear.
                    _parent.Remove(this, _keySystem, _mediaKeySession);

                    delete _buffer;

                    TRACE(Trace::Information, ("Server::Session::~Session(%s,%s) => %p", _keySystem.c_str(), _sessionId.c_str(), this));
                    TRACE_L1("Destructed the Session Server side: %p", this);
                }

            public:
                inline bool IsSupported(const CommonEncryptionData& keyIds, const string& keySystem) const
                {
                    return ((keySystem == _keySystem) && (_cencData.IsSupported(keyIds) == true));
                }
                inline bool HasKeyId(const OCDM::KeyId& keyId) const
                {
                    return (_cencData.HasKeyId(keyId));
                }
                virtual std::string SessionId() const override
                {
                    return (_sessionId);
                }

                virtual std::string Metadata() const override
                {
                    return _mediaKeySession->GetMetadata();
                }

                virtual ::OCDM::ISession::KeyStatus Status() const override
                {
                    return (_cencData.Status());
                }

                ::OCDM::ISession::KeyStatus Status(const uint8_t keyId[], const uint8_t length) const override
                {
                    return (_cencData.Status(CommonEncryptionData::KeyId(static_cast<CommonEncryptionData::systemType>(0), keyId, length)));
                }

                virtual std::string BufferId() const override
                {
                    return (_buffer->Name());
                }

                virtual std::string BufferIdExt() const override
                {
                    return (_buffer->Name());
                }

                // Loads the data stored for the specified session into the cdm object
                virtual ::OCDM::OCDM_RESULT Load() override
                {
                    TRACE(Trace::Information, ("Load()"));
                    return (::OCDM::OCDM_RESULT)(_mediaKeySession->Load());
                }

                // Process a key message response.
                virtual void Update(const uint8_t* keyMessage, const uint16_t keyLength) override
                {
                    TRACE(Trace::Information, ("Update(%d)", keyLength));
                    return (_mediaKeySession->Update(keyMessage, keyLength));
                }

                //Removes all license(s) and key(s) associated with the session
                virtual ::OCDM::OCDM_RESULT Remove() override
                {
                    TRACE(Trace::Information, ("Remove()"));
                    return (::OCDM::OCDM_RESULT)(_mediaKeySession->Remove());
                }

                //We are done with the Session, close what we can..
                virtual void Close() override
                {
                    TRACE(Trace::Information, ("Close()"));

                    _mediaKeySession->Close();
                }

                virtual void ResetOutputProtection() override {
                    TRACE_L1("ResetOutputProtection! %p", this);
                    _mediaKeySession->ResetOutputProtection();
                }

                virtual void Revoke(OCDM::ISession::ICallback* callback) override
                {
                    _sink.Revoke(callback);
                }

                virtual uint32_t SessionIdExt() const override
                {
                    return _mediaKeySessionExt->GetSessionIdExt();
                }

                virtual OCDM::OCDM_RESULT SetDrmHeader(const uint8_t drmHeader[], uint32_t drmHeaderLength) override
                {
                    return (OCDM::OCDM_RESULT)_mediaKeySessionExt->SetDrmHeader(drmHeader, drmHeaderLength);
                }

                virtual OCDM::OCDM_RESULT GetChallengeDataExt(uint8_t* challenge, uint32_t& challengeSize, uint32_t isLDL) override
                {
                    return (OCDM::OCDM_RESULT)_mediaKeySessionExt->GetChallengeDataExt(challenge, challengeSize, isLDL);
                }

                virtual OCDM::OCDM_RESULT CancelChallengeDataExt() override
                {
                    return (OCDM::OCDM_RESULT)_mediaKeySessionExt->CancelChallengeDataExt();
                }

                virtual OCDM::OCDM_RESULT StoreLicenseData(const uint8_t licenseData[], uint32_t licenseDataSize, unsigned char* secureStopId) override
                {
                    return (OCDM::OCDM_RESULT)_mediaKeySessionExt->StoreLicenseData(licenseData, licenseDataSize, secureStopId);
                }

                virtual OCDM::OCDM_RESULT SelectKeyId(const uint8_t keyLength, const uint8_t keyId[]) override
                {
                    return (OCDM::OCDM_RESULT)_mediaKeySessionExt->SelectKeyId(keyLength, keyId);
                }

                virtual OCDM::OCDM_RESULT CleanDecryptContext() override
                {
                    return (OCDM::OCDM_RESULT)_mediaKeySessionExt->CleanDecryptContext();
                }

                BEGIN_INTERFACE_MAP(Session)
                INTERFACE_ENTRY(::OCDM::ISession)
                INTERFACE_RELAY(::OCDM::ISessionExt, _mediaKeySessionExt)
                END_INTERFACE_MAP

            private:
            private:
                AccessorOCDM& _parent;
                mutable Core::CriticalSection _adminLock;
                mutable uint32_t _refCount;
                std::string _keySystem;
                std::string _sessionId;
                CDMi::IMediaKeySession* _mediaKeySession;
                CDMi::IMediaKeySessionExt* _mediaKeySessionExt;
                Core::Sink<Sink> _sink;
                DataExchange* _buffer;
                CommonEncryptionData _cencData;
            };

        public:
            AccessorOCDM(OCDMImplementation* parent, const string& name, const uint32_t defaultSize)
                : _parent(*parent)
                , _adminLock()
                , _administrator(name)
                , _defaultSize(defaultSize)
                , _sessionList()
            {
                ASSERT(parent != nullptr);
            }
            virtual ~AccessorOCDM()
            {
                TRACE_L1("Released the AccessorOCDM server side [%d]", __LINE__);
            }

        public:
            virtual bool IsTypeSupported(
                const std::string& keySystem,
                const std::string& mimeType) const override
            {

                return (_parent.IsTypeSupported(keySystem, mimeType) ? 0 : 1);
            }

            virtual OCDM::OCDM_RESULT Metadata(
                const std::string& keySystem,
                std::string& metadata) const override
            {
                OCDM::OCDM_RESULT result = OCDM::OCDM_KEYSYSTEM_NOT_SUPPORTED;
                metadata.clear();

                CDMi::IMediaKeys* system = _parent.KeySystem(keySystem);
                if (system != nullptr) {
                    metadata = system->GetMetadata();
                    result = OCDM::OCDM_SUCCESS;
                }

                return result;
            }

            // Create a MediaKeySession using the supplied init data and CDM data.
            virtual OCDM::OCDM_RESULT CreateSession(
                const std::string& keySystem,
                const int32_t licenseType,
                const std::string& initDataType,
                const uint8_t* initData,
                const uint16_t initDataLength,
                const uint8_t* CDMData,
                const uint16_t CDMDataLength,
                ::OCDM::ISession::ICallback* callback,
                std::string& sessionId,
                ::OCDM::ISession*& session) override
            {
                 CDMi::IMediaKeys *system = _parent.KeySystem(keySystem);

                 session = nullptr;
                 if (system != nullptr)
                 {
                     CDMi::IMediaKeySession *sessionInterface = nullptr;
                     CommonEncryptionData keyIds(initData, initDataLength);

                     // OKe we got a buffer machanism to transfer the raw data, now create
                     // the session.
                     if (system->CreateMediaKeySession(keySystem, licenseType, 
                                        initDataType.c_str(), initData, initDataLength, 
                                        CDMData, CDMDataLength, &sessionInterface) == 0)
                     {
                         if (sessionInterface != nullptr)
                         {
                             std::string bufferId;

                             // See if there is a buffer available we can use..
                             if (_administrator.AquireBuffer(bufferId) == true)
                             {

                                 SessionImplementation *newEntry = 
                                    Core::Service<SessionImplementation>::Create<SessionImplementation>(this,
                                                 keySystem, sessionInterface,
                                                 callback, bufferId, _defaultSize, &keyIds);

                                 session = newEntry;
                                 sessionId = newEntry->SessionId();

                                 _adminLock.Lock();

                                 _sessionList.push_front(newEntry);
                                
                                if(false == keyIds.IsEmpty())
                                {
                                    CommonEncryptionData::Iterator index(keyIds.Keys());
                                    while (index.Next() == true) {
                                        const CommonEncryptionData::KeyId& entry(index.Current());
                                        callback->OnKeyStatusUpdate( entry.Id(), entry.Length(), ::OCDM::ISession::StatusPending);
                                    }
                                }
                                _adminLock.Unlock();
                             } else {
                                 TRACE_L1("Could not allocate a buffer for session: %s", sessionId.c_str());

                                 // TODO: We need to drop the session somehow...
                             }
                         }
                     }
                 }

                 if (session == nullptr) {
                     TRACE_L1("Could not create a DRM session! [%d]", __LINE__);
                 }

                 return (session != nullptr ? ::OCDM::OCDM_RESULT::OCDM_SUCCESS : ::OCDM::OCDM_RESULT::OCDM_S_FALSE);
            }

            // Set Server Certificate
            virtual ::OCDM::OCDM_RESULT SetServerCertificate(
                const std::string& keySystem,
                const uint8_t* serverCertificate,
                const uint16_t serverCertificateLength) override
            {

                CDMi::IMediaKeys* system = _parent.KeySystem(keySystem);
                ::OCDM::OCDM_RESULT result = ::OCDM::OCDM_RESULT::OCDM_S_FALSE;

                if (system != nullptr) {
                    TRACE(Trace::Information, ("Set ServerCertificate()"));
                    result = static_cast<::OCDM::OCDM_RESULT>(system->SetServerCertificate(serverCertificate, serverCertificateLength));
                } else {
                    TRACE_L1("Could not set the Server Certificates for system: %s", keySystem.c_str());
                }
                return result;
            }

            virtual uint64_t GetDrmSystemTime(const std::string& keySystem) const override
            {
                CDMi::IMediaKeysExt* systemExt = dynamic_cast<CDMi::IMediaKeysExt*>(_parent.KeySystem(keySystem));
                if (systemExt) {
                    return (OCDM::OCDM_RESULT)systemExt->GetDrmSystemTime();
                }
                return ::OCDM::OCDM_RESULT::OCDM_S_FALSE;
            }

            std::string GetVersionExt(const std::string& keySystem) const override
            {
                CDMi::IMediaKeysExt* systemExt = dynamic_cast<CDMi::IMediaKeysExt*>(_parent.KeySystem(keySystem));
                if (systemExt) {
                    return systemExt->GetVersionExt();
                }
                return "";
            }

            uint32_t GetLdlSessionLimit(const std::string& keySystem) const override
            {
                CDMi::IMediaKeysExt* systemExt = dynamic_cast<CDMi::IMediaKeysExt*>(_parent.KeySystem(keySystem));
                if (systemExt) {
                    return systemExt->GetLdlSessionLimit();
                }
                return 0;
            }

            bool IsSecureStopEnabled(const std::string& keySystem) override
            {
                CDMi::IMediaKeysExt* systemExt = dynamic_cast<CDMi::IMediaKeysExt*>(_parent.KeySystem(keySystem));
                if (systemExt) {
                    return systemExt->IsSecureStopEnabled();
                }
                return false;
            }

            OCDM::OCDM_RESULT EnableSecureStop(const std::string& keySystem, bool enable) override
            {
                CDMi::IMediaKeysExt* systemExt = dynamic_cast<CDMi::IMediaKeysExt*>(_parent.KeySystem(keySystem));
                if (systemExt) {
                    return (OCDM::OCDM_RESULT)systemExt->EnableSecureStop(enable);
                }
                return ::OCDM::OCDM_RESULT::OCDM_S_FALSE;
            }

            uint32_t ResetSecureStops(const std::string& keySystem) override
            {
                CDMi::IMediaKeysExt* systemExt = dynamic_cast<CDMi::IMediaKeysExt*>(_parent.KeySystem(keySystem));
                if (systemExt) {
                    return systemExt->ResetSecureStops();
                }
                return 0;
            }

            OCDM::OCDM_RESULT GetSecureStopIds(
                const std::string& keySystem,
                unsigned char Ids[],
                uint16_t idsLength,
                uint32_t& count)
            {
                CDMi::IMediaKeysExt* systemExt = dynamic_cast<CDMi::IMediaKeysExt*>(_parent.KeySystem(keySystem));
                if (systemExt) {
                    return (OCDM::OCDM_RESULT)systemExt->GetSecureStopIds(Ids, idsLength, count);
                }
                return ::OCDM::OCDM_RESULT::OCDM_S_FALSE;
            }

            OCDM::OCDM_RESULT GetSecureStop(
                const std::string& keySystem,
                const unsigned char sessionID[],
                uint32_t sessionIDLength,
                unsigned char* rawData,
                uint16_t& rawSize)
            {
                CDMi::IMediaKeysExt* systemExt = dynamic_cast<CDMi::IMediaKeysExt*>(_parent.KeySystem(keySystem));
                if (systemExt) {
                    return (OCDM::OCDM_RESULT)systemExt->GetSecureStop(sessionID, sessionIDLength, rawData, rawSize);
                }
                return ::OCDM::OCDM_RESULT::OCDM_S_FALSE;
            }

            OCDM::OCDM_RESULT CommitSecureStop(
                const std::string& keySystem,
                const unsigned char sessionID[],
                uint32_t sessionIDLength,
                const unsigned char serverResponse[],
                uint32_t serverResponseLength)
            {
                CDMi::IMediaKeysExt* systemExt = dynamic_cast<CDMi::IMediaKeysExt*>(_parent.KeySystem(keySystem));
                if (systemExt) {
                    return (OCDM::OCDM_RESULT)systemExt->CommitSecureStop(sessionID, sessionIDLength, serverResponse, serverResponseLength);
                }
                return ::OCDM::OCDM_RESULT::OCDM_S_FALSE;
            }

            OCDM::OCDM_RESULT DeleteKeyStore(const std::string& keySystem) override
            {
                CDMi::IMediaKeysExt* systemExt = dynamic_cast<CDMi::IMediaKeysExt*>(_parent.KeySystem(keySystem));
                if (systemExt) {
                    return (OCDM::OCDM_RESULT)systemExt->DeleteKeyStore();
                }
                return ::OCDM::OCDM_RESULT::OCDM_S_FALSE;
            }

            OCDM::OCDM_RESULT DeleteSecureStore(const std::string& keySystem) override
            {
                CDMi::IMediaKeysExt* systemExt = dynamic_cast<CDMi::IMediaKeysExt*>(_parent.KeySystem(keySystem));
                if (systemExt) {
                    return (OCDM::OCDM_RESULT)systemExt->DeleteSecureStore();
                }
                return ::OCDM::OCDM_RESULT::OCDM_S_FALSE;
            }

            OCDM::OCDM_RESULT GetKeyStoreHash(
                const std::string& keySystem,
                uint8_t keyStoreHash[],
                uint32_t keyStoreHashLength) override
            {
                CDMi::IMediaKeysExt* systemExt = dynamic_cast<CDMi::IMediaKeysExt*>(_parent.KeySystem(keySystem));
                if (systemExt) {
                    return (OCDM::OCDM_RESULT)systemExt->GetSecureStoreHash(keyStoreHash, keyStoreHashLength);
                }
                return ::OCDM::OCDM_RESULT::OCDM_S_FALSE;
            }

            OCDM::OCDM_RESULT GetSecureStoreHash(
                const std::string& keySystem,
                uint8_t secureStoreHash[],
                uint32_t secureStoreHashLength) override
            {
                CDMi::IMediaKeysExt* systemExt = dynamic_cast<CDMi::IMediaKeysExt*>(_parent.KeySystem(keySystem));
                if (systemExt) {
                    return (OCDM::OCDM_RESULT)systemExt->GetSecureStoreHash(secureStoreHash, secureStoreHashLength);
                }
                return ::OCDM::OCDM_RESULT::OCDM_S_FALSE;
            }

            BEGIN_INTERFACE_MAP(AccessorOCDM)
            INTERFACE_ENTRY(::OCDM::IAccessorOCDM)
            END_INTERFACE_MAP

        private:
            ::OCDM::ISession* FindSession(const CommonEncryptionData& keyIds, const string& keySystem) const
            {
                ::OCDM::ISession* result = nullptr;

                std::list<SessionImplementation*>::const_iterator index(_sessionList.begin());

                while ((index != _sessionList.end()) && (result == nullptr)) {

                    if ((*index)->IsSupported(keyIds, keySystem) == true) {
                        result = *index;
                        result->AddRef();
                    } else {
                        index++;
                    }
                }
                return (result);
            }
            void Remove(SessionImplementation* session, const string& keySystem, CDMi::IMediaKeySession* mediaKeySession)
            {

                _adminLock.Lock();

                ASSERT(session != nullptr);

                if (mediaKeySession != nullptr) {

                    mediaKeySession->Run(nullptr);

                    CDMi::IMediaKeys* system = _parent.KeySystem(keySystem);
                    if (system != nullptr) {
                        system->DestroyMediaKeySession(mediaKeySession);
                    } else {
                        TRACE_L1("No system to handle session = %p\n", session);
                    }
                }

                if (session != nullptr) {

                    _administrator.ReleaseBuffer(session->BufferId());

                    std::list<SessionImplementation*>::iterator index(_sessionList.begin());

                    while ((index != _sessionList.end()) && (session != (*index))) {
                        index++;
                    }

                    ASSERT(index != _sessionList.end());

                    if (index != _sessionList.end()) {
                        const string sessionId(session->SessionId());
                        // Before we remove it here, release it.
                        _sessionList.erase(index);
                    }
                }

                _adminLock.Unlock();
            }

        private:
            OCDMImplementation& _parent;
            mutable Core::CriticalSection _adminLock;
            BufferAdministrator _administrator;
            uint32_t _defaultSize;
            std::list<SessionImplementation*> _sessionList;
        };

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            class Systems : public Core::JSON::Container {
            private:
                Systems& operator=(const Systems&);

            public:
                Systems()
                    : Core::JSON::Container()
                    , Name()
                    , Designators()
                    , Configuration()
                {
                    Add("name", &Name);
                    Add("designators", &Designators);
                    Add("configuration", &Configuration);
                }
                Systems(const Systems& copy)
                    : Core::JSON::Container()
                    , Name(copy.Name)
                    , Designators(copy.Designators)
                    , Configuration(copy.Configuration)
                {
                    Add("name", &Name);
                    Add("designators", &Designators);
                    Add("configuration", &Configuration);
                    Add("blacklistedcodecregexps", &BlacklistedCodecRegexps);
                    Add("blacklistedmediatyperegexps", &BlacklistedMediaTypeRegexps);
                }

                virtual ~Systems() = default;

            public:
                Core::JSON::String Name;
                Core::JSON::ArrayType<Core::JSON::String> Designators;
                Core::JSON::String Configuration;
                Core::JSON::ArrayType<Core::JSON::String> BlacklistedCodecRegexps;
                Core::JSON::ArrayType<Core::JSON::String> BlacklistedMediaTypeRegexps;
            };

        public:
            Config()
                : Core::JSON::Container()
                , Location()
                , Connector(_T("/tmp/ocdm"))
                , SharePath(_T("/tmp"))
                , ShareSize(8 * 1024)
                , KeySystems()
            {
                Add(_T("location"), &Location);
                Add(_T("connector"), &Connector);
                Add(_T("sharepath"), &SharePath);
                Add(_T("sharesize"), &ShareSize);
                Add(_T("systems"), &KeySystems);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Location;
            Core::JSON::String Connector;
            Core::JSON::String SharePath;
            Core::JSON::DecUInt32 ShareSize;
            Core::JSON::ArrayType<Systems> KeySystems;
        };

    public:
        OCDMImplementation()
            : _entryPoint(nullptr)
            , _service(nullptr)
            , _compliant(false)
            , _systemToFactory()
            , _systemLibraries()
        {
            TRACE_L1("Constructing OCDMImplementation Service: %p", this);
        }
        virtual ~OCDMImplementation()
        {
            if (_service != nullptr) {
                delete _service;
            }

            if (_entryPoint != nullptr) {
                _entryPoint->Release();
            }

            _systemLibraries.clear();

            TRACE_L1("Destructed OCDMImplementation Service: %p", this);
        }

    public:
        uint32_t Initialize(PluginHost::IShell* service) override
        {
            uint32_t result = Core::ERROR_OPENING_FAILED;

            // On activation subscribe, on deactivation un-subscribe
            PluginHost::ISubSystem* subSystem = service->SubSystems();

            ASSERT(subSystem != nullptr);

            // Start loading the configured factories
            Config config;
            config.FromString(service->ConfigLine());

            const string locator(service->DataPath() + config.Location.Value());

            // Before we start loading the mapping of the Keys to the factories, load the factories :-)
            Core::Directory entry(locator.c_str(), _T("*.drm"));
            std::map<const string, SystemFactory> factories;

            while (entry.Next() == true) {
                Core::Library library(entry.Current().c_str());

                if (library.IsLoaded() == true) {
                    GetDRMSystemFunction handle = reinterpret_cast<GetDRMSystemFunction>(library.LoadFunction(_T("GetSystemFactory")));

                    if (handle != nullptr) {
                        CDMi::ISystemFactory* entry = handle();

                        if (handle != nullptr) {
                            SystemFactory element;
                            element.Name = Core::ClassNameOnly(entry->KeySystem()).Text();
                            element.Factory = entry;
                            _keySystems.push_back(element.Name);
                            factories.insert(std::pair<const string, SystemFactory>(element.Name, element));
                            _systemLibraries.push_back(library);
                        }
                    }
                } else {
                    SYSLOG(Logging::Startup, (_T("Could not load factory [%s], error [%s]"), Core::File::FileNameExtended(entry.Current()).c_str(), library.Error().c_str()));
                }
            }

            Core::JSON::ArrayType<Config::Systems>::ConstIterator index(static_cast<const Config&>(config).KeySystems.Elements());

            while (index.Next() == true) {

                const string system(index.Current().Name.Value());

                if ((system.empty() == false) && (index.Current().Designators.IsSet() == true)) {
                    Core::JSON::ArrayType<Core::JSON::String>::ConstIterator designators(static_cast<const Core::JSON::ArrayType<Core::JSON::String>&>(index.Current().Designators).Elements());

                    // Find a factory for the key system:
                    std::map<const string, SystemFactory>::iterator factory(factories.find(system));

                    while (designators.Next() == true) {
                        const string designator(designators.Current().Value());
                        if (designator.empty() == false) {
                            if (factory != factories.end()) {
                                _systemToFactory.insert(std::pair<const std::string, SystemFactory>(designator, factory->second));

                            } else {
                                SYSLOG(Logging::Startup, (_T("Required factory [%s], not found for [%s]"), system.c_str(), designator.c_str()));
                            }
                        }
                    }

                    //now handle the configuration
                    if (factory != factories.end()) {
                        const string configuration(index.Current().Configuration.Value());
                        factory->second.Factory->Initialize(service, configuration);
                    }
                }

                if ((system.empty() == false) && (index.Current().BlacklistedCodecRegexps.IsSet() == true)) {
                    FillBlacklist(_systemBlacklistedCodecRegexps, system, index.Current().BlacklistedCodecRegexps);
                }

                if ((system.empty() == false) && (index.Current().BlacklistedMediaTypeRegexps.IsSet() == true)) {
                    FillBlacklist(_systemBlacklistedMediaTypeRegexps, system, index.Current().BlacklistedMediaTypeRegexps);
                }
            }

            if (_systemToFactory.size() == 0) {
                SYSLOG(Logging::Startup, (_T("No DRM factories specified. OCDM can not service any DRM requests.")));
            }

            _entryPoint = Core::Service<AccessorOCDM>::Create<::OCDM::IAccessorOCDM>(this, config.SharePath.Value(), config.ShareSize.Value());
            Core::ProxyType<RPC::InvokeServer> server = Core::ProxyType<RPC::InvokeServer>::Create(&Core::IWorkerPool::Instance());
            _service = new ExternalAccess(Core::NodeId(config.Connector.Value().c_str()), _entryPoint, server);

            if (_service != nullptr) {

                if (_service->IsListening() == false) {
                    delete _service;
                    _entryPoint->Release();
                    _service = nullptr;
                    _entryPoint = nullptr;
                } else {
                    if (subSystem != nullptr) {

                        // Announce the port on which we are listening
                        Core::SystemInfo::SetEnvironment(_T("OPEN_CDM_SERVER"), config.Connector.Value(), true);

                        ASSERT(subSystem->IsActive(PluginHost::ISubSystem::DECRYPTION) == false);
                        subSystem->Set(PluginHost::ISubSystem::DECRYPTION, this);
                    }
                    if (_systemToFactory.size() == 0) {
                        SYSLOG(Logging::Startup, (string(_T("OCDM server has NO key systems registered!!!"))));
                    }
                }
            }
            return (result);
        }
        void Deinitialize(PluginHost::IShell* service) override {
            std::map<const string, SystemFactory>::iterator factory(_systemToFactory.begin());

            std::list<CDMi::ISystemFactory*> deinitialized;

            while (factory != _systemToFactory.end()) {
                std::list<CDMi::ISystemFactory*>::iterator index(std::find(deinitialized.begin(), deinitialized.end(), factory->second.Factory));

                if(index == deinitialized.end()){ 
                    TRACE_L1("Deinitializing factory(%p) for key system %s", factory->second.Factory, factory->second.Factory->KeySystem());
                    factory->second.Factory->Deinitialize(service);
                    deinitialized.push_back(factory->second.Factory);
                }
                
                factory++;
            }
        }
        virtual uint32_t Reset()
        {
            return (Core::ERROR_NONE);
        }
        virtual RPC::IStringIterator* Systems() const
        {
            return (Core::Service<RPC::StringIterator>::Create<RPC::IStringIterator>(_keySystems));
        }
        virtual RPC::IStringIterator* Designators(const string& keySystem) const
        {
            std::list<string> designators;
            LoadDesignators(keySystem, designators);
            return (Core::Service<RPC::StringIterator>::Create<RPC::IStringIterator>(designators));
        }
        virtual RPC::IStringIterator* Sessions(const string& keySystem) const
        {
            std::list<string> sessions;
            LoadSessions(keySystem, sessions);
            return (Core::Service<RPC::StringIterator>::Create<RPC::IStringIterator>(sessions));
        }

    public:
        bool IsTypeSupported(const std::string& keySystem, const std::string& contentType)
        {

            // FIXME: The dead code below this statement has at least the following issues,
            //  - The MIME checking code is **very far** from compliant (no codec= support is one major issue, there are more)
            //  - If it's for HTML5 user agents, it's pointless because they do their own
            //    compliance checking for MIME types.
            //  - If it's for the out-of-app media engines, then interfaces that talk to
            //    those engines should be provided to check this properly.
            //  - Iterating the keys of a map and then strcmp'ing them isn't how maps are
            //    supposed to be used. The code below may as well have chosen a vector.
            //    map[string] is a much better way to do that, although when it's unlikely
            //    to ever have more than 3 keys, a map is pretty pointless as well.
            //  So given the above, what I've changed here is that now we just check if
            //  the key system is supported, because that's something we can easily do
            //  and that check belongs in our domain.
            bool result = (keySystem.empty() == false);

            if (result == true) {
                std::map<const std::string, SystemFactory>::iterator index(_systemToFactory.find(keySystem));

                if (index == _systemToFactory.end()) {
                    result = false;
                } else {
                    if (contentType.empty() == false) {
                        std::string mimeType;
                        std::vector<std::string> codecs;
                        ParseContentType(contentType, mimeType, codecs);
                        if (mimeType.empty() == false) {
                            Blacklist::iterator systemMediaTypeRegexps = _systemBlacklistedMediaTypeRegexps.find(index->second.Name);
                            if (systemMediaTypeRegexps != _systemBlacklistedMediaTypeRegexps.end()) {
                                for (auto& systemMediaTypeRegexp : systemMediaTypeRegexps->second) {
                                    if (std::regex_match(mimeType, std::regex(systemMediaTypeRegexp))) {
                                        TRACE(Trace::Information, ("%s mime type matches blacklisted %s regexp", mimeType.c_str(), systemMediaTypeRegexp.c_str()));
                                        result = false;
                                        break;
                                    }
                                }
                            }

                            if (result == true && codecs.size() > 0) {
                                Blacklist::iterator systemCodecRegexps = _systemBlacklistedCodecRegexps.find(index->second.Name);
                                if (systemCodecRegexps != _systemBlacklistedCodecRegexps.end()) {
                                    for (const std::string& codec : codecs) {
                                        for (const std::string& codecRegexp : systemCodecRegexps->second) {
                                            if (std::regex_match(codec, std::regex(codecRegexp))) {
                                                TRACE(Trace::Information, ("%s codec matches blacklisted %s regexp", codec.c_str(), codecRegexp.c_str()));
                                                result = false;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            TRACE(Trace::Information, ("IsTypeSupported(%s,%s) => %s", keySystem.c_str(), contentType.c_str(), result ? _T("True") : _T("False")));

            return result;
        }

        CDMi::IMediaKeys* KeySystem(const std::string& keySystem)
        {
            CDMi::IMediaKeys* result = nullptr;

            if (keySystem.empty() == false) {
                std::map<const std::string, SystemFactory>::iterator index(_systemToFactory.find(keySystem));

                if (_systemToFactory.end() != index) {
                    result = index->second.Factory->Instance();
                }
            }

            TRACE(Trace::Information, ("KeySystem(%s) => %p", keySystem.c_str(), result));
            return (result);
        }

    private:
        void LoadDesignators(const string& keySystem, std::list<string>& designators) const
        {
            std::map<const std::string, SystemFactory>::const_iterator index(_systemToFactory.begin());
            while (index != _systemToFactory.end()) {
                if (keySystem == index->second.Name) {
                    designators.push_back(index->first);
                }
                index++;
            }
        }
        void LoadSessions(const string& keySystem, std::list<string>& designators) const
        {
            std::map<const std::string, SystemFactory>::const_iterator index(_systemToFactory.begin());
            while (index != _systemToFactory.end()) {
                if (keySystem == index->second.Name) {
                    designators.push_back(index->first);
                }
                index++;
            }
        }

    private:
        // -------------------------------------------------------------------------------------------------------------
        // IDecryption methods
        // -------------------------------------------------------------------------------------------------------------
        BEGIN_INTERFACE_MAP(OCDMImplementation)
        INTERFACE_ENTRY(Exchange::IContentDecryption)
        END_INTERFACE_MAP

    private:
        using Blacklist = std::map<const std::string, std::vector<std::string>>;
        void FillBlacklist(Blacklist& blacklist, const std::string& system, const Core::JSON::ArrayType<Core::JSON::String>& list)
        {
            Core::JSON::ArrayType<Core::JSON::String>::ConstIterator iter(list.Elements());

            std::vector<std::string> elements;
            while (iter.Next() == true) {
                const string element(iter.Current().Value());
                if (element.empty() == false) {
                    elements.emplace_back(std::move(element));
                }
            }

            blacklist.insert(std::pair<const std::string, std::vector<std::string>>(system, elements));
        }

        ::OCDM::IAccessorOCDM* _entryPoint;
        ExternalAccess* _service;
        bool _compliant;
        std::map<const std::string, SystemFactory> _systemToFactory;
        Blacklist _systemBlacklistedCodecRegexps;
        Blacklist _systemBlacklistedMediaTypeRegexps;
        std::list<Core::Library> _systemLibraries;
        std::list<string> _keySystems;
    };

    SERVICE_REGISTRATION(OCDMImplementation, 1, 0);
}
} /* namespace WPEFramework::Plugin */
