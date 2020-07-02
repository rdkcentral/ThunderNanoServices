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

#pragma once

#include "Module.h"

namespace WPEFramework {

struct INotifier {
    virtual ~INotifier() = default;
    virtual void NotifyStatus(const uint32_t status) = 0;
    virtual void NotifyProgress(const uint8_t percentage) = 0;
};

namespace PluginHost {

    // Here we might potentially start using Web::SignedFileBodyType<Crypto::HMAC256>>

    class DownloadEngine : public Web::ClientTransferType<Core::SocketStream, Web::SignedFileBodyType<Crypto::SHA256>> {
    private:
        typedef Web::ClientTransferType<Core::SocketStream, Web::SignedFileBodyType<Crypto::SHA256>> BaseClass;

    public:
        DownloadEngine() = delete;
        DownloadEngine(const DownloadEngine&) = delete;
        DownloadEngine& operator=(const DownloadEngine&) = delete;

        DownloadEngine(INotifier* notifier, const string& hashKey, const uint16_t interval)
            : BaseClass(false, Core::NodeId(_T("0.0.0.0")), Core::NodeId(), 1024, ((64 * 1024) - 1))
            , _adminLock()
            , _notifier(notifier)
            , _storage()
            , _interval(interval * 1000)
            , _checkHash(false)
            , _activity(*this)
        {
            // If we are going for HMAC, here we could set our secret...
            memset(_HMAC, 0, Crypto::HASH_SHA256);
            // BaseClass::Hash().Key(hashKey);
        }
        ~DownloadEngine() override
        {
            _adminLock.Lock();
            _activity.Revoke();
            _adminLock.Unlock();
        }

    public:
        uint32_t Start(const string& locator, const string& destination, const string& hashValue)
        {
            Core::URL url(locator);
            uint32_t result = (url.IsValid() == true ? Core::ERROR_INPROGRESS : Core::ERROR_INCORRECT_URL);


            _adminLock.Lock();

            // But I guess for the firmware control, we are getting the
            // HMAC, not via an HTTP header but Through REST API, we need
            // to remeber what it should be. Lets safe the HMAC for
            // validation, if it is transferred here....
            if (hashValue.empty() == false) {
                if (HashStringToBytes(hashValue, _HMAC) == true) {
                    _checkHash = true;
                } else {
                    result = Core::ERROR_INCORRECT_HASH;
                }
            }

            if ((result == Core::ERROR_INPROGRESS) && (_storage.IsOpen() == false)) {

                result = Core::ERROR_OPENING_FAILED;
                _storage = destination;

                // The create truncates the file (if it exists), to 0.
                if (_storage.Create() == true) {

                    result = BaseClass::Download(url, _storage);

                    if (((result == Core::ERROR_NONE) || (result == Core::ERROR_INPROGRESS)) && (_interval != 0)) {
                        _activity.Revoke();
                        _activity.Schedule(Core::Time::Now().Add(_interval));
                    }
                }
            }
            _adminLock.Unlock();

            return (result);
        }

    private:
        void Transfered(const uint32_t result, const Web::SignedFileBodyType<Crypto::SHA256>& destination) override
        {
            uint32_t status = result;

            // Let's see if the calculated HMAC is what we expected....
            if ((status == Core::ERROR_NONE) && (_checkHash == true) &&
                (::memcmp(const_cast<Web::SignedFileBodyType<Crypto::SHA256>&>(destination).Hash().Result(), _HMAC, Crypto::HASH_SHA256) != 0)) {

                status = Core::ERROR_UNAUTHENTICATED;
            }
            _storage.Close();

            _adminLock.Lock();

            if (_notifier != nullptr) {
                _notifier->NotifyStatus(status);
            }

            if (status != Core::ERROR_NONE) {
                _storage.Destroy();
            }

            _adminLock.Unlock();
        }

        bool Setup(const Core::URL& remote) override
        {
            bool result = false;

            if (remote.Host().IsSet() == true) {
                uint16_t portNumber(remote.Port().IsSet() ? remote.Port().Value() : 80);

                BaseClass::Link().RemoteNode(Core::NodeId(remote.Host().Value().c_str(), portNumber));

                result = true;
            }
            return (result);
        }

        inline bool HashStringToBytes(const string& hash, uint8_t hashHex[Crypto::HASH_SHA256])
        {
            bool status = true;

            for (uint8_t i = 0; i < Crypto::HASH_SHA256; i++) {
                char highNibble = hash.c_str()[i * 2];
                char lowNibble = hash.c_str()[(i * 2) + 1];
                if (isxdigit(highNibble) && isxdigit(lowNibble)) {
                    std::string byteStr = hash.substr(i * 2, 2);
                    hashHex[i] = static_cast<uint8_t>(strtol(byteStr.c_str(), nullptr, 16));
                }
                else {
                    status = false;
                    break;
                }
            }
	    return status;
        }

        friend Core::ThreadPool::JobType<DownloadEngine&>;
        void Dispatch()
        {
            _adminLock.Lock();

            if (_notifier != nullptr) {

                uint8_t percentage = static_cast<uint8_t>((static_cast<float>(BaseClass::Transferred()) * 100)/static_cast<float>(BaseClass::FileSize()));
                if (percentage) {
                    _notifier->NotifyProgress(percentage);
                }
                if (percentage < 100) {
                    _activity.Schedule(Core::Time::Now().Add(_interval));
                }
            }

            _adminLock.Unlock();
        }

    private:
        Core::CriticalSection _adminLock;
        INotifier* _notifier;
        Core::File _storage;

        uint32_t _interval;
        bool _checkHash;
        uint8_t _HMAC[Crypto::HASH_SHA256];
        Core::WorkerPool::JobType<DownloadEngine&> _activity;
    };
}
}
