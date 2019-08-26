#pragma once

#include "Module.h"

namespace WPEFramework {

struct INotifier {
    virtual ~INotifier() {}
    virtual void NotifyDownloadStatus(const uint32_t status) = 0;
};

namespace PluginHost {

    class DownloadEngine : public Web::ClientTransferType<Core::SocketStream, Web::SignedFileBodyType<Crypto::SHA256>> {
    private:
        typedef Web::ClientTransferType<Core::SocketStream, Web::SignedFileBodyType<Crypto::SHA256>> BaseClass;

        DownloadEngine() = delete;
        DownloadEngine(const DownloadEngine&) = delete;
        DownloadEngine& operator=(const DownloadEngine&) = delete;

    public:
        DownloadEngine(INotifier* notifier, const string& downloadStorage)
            : BaseClass(false, Core::NodeId(_T("0.0.0.0")), Core::NodeId(), 1024, 1024)
            , _notifier(notifier)
            , _storage(downloadStorage.c_str(), false)
        {
        }
        virtual ~DownloadEngine()
        {
        }

    public:
        uint32_t Start(const string& locator, const string& destination, const string& hash)
        {
            Core::URL url(locator);
            uint32_t result = (url.IsValid() == true ? Core::ERROR_INPROGRESS : Core::ERROR_INCORRECT_URL);

            if (result == Core::ERROR_INPROGRESS) {

                _adminLock.Lock();

                if (_storage.IsOpen() == false) {

                    result = Core::ERROR_OPENING_FAILED;

                    if (_storage.Create() == true) {

                        result = Core::ERROR_NONE;

                        _hash = hash;

                        BaseClass::Download(url, _storage);
                    }
                }

                _adminLock.Unlock();
            }

            return (result);
        }

    private:
        virtual void Transfered(const uint32_t result, const Web::SignedFileBodyType<Crypto::SHA256>& destination) override
        {
            // Do your file magic stuff here, validate the hash and if it validates oke, move the file to the destination
            // location. Depending on these actions, modify the result and notify the parent.
            // For now we move on :-)
            // TODO: Implement above sequence
            uint32_t status = result;
            if (status == Core::ERROR_NONE) {
                if (_hash.empty() != true) {
                    uint8_t hashHex[Crypto::HASH_SHA256];
                    if (HashStringToBytes(_hash, hashHex) == true) {

                        const uint8_t* downloadedHash = destination.SerializedHashValue();
                        if (downloadedHash != nullptr) {
                            for (uint16_t i = 0; i < Crypto::HASH_SHA256; i++) {
                                if (downloadedHash[i] != hashHex[i]) {
                                    status = Core::ERROR_INCORRECT_HASH;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            if (_notifier != nullptr) {
                _notifier->NotifyDownloadStatus(status);
            }

            _adminLock.Lock();
            _storage.Close();
            _adminLock.Unlock();
        }

        virtual bool Setup(const Core::URL& remote) override
        {
            bool result = false;

            if (remote.Host().IsSet() == true) {
                uint16_t portNumber(remote.Port().IsSet() ? remote.Port().Value() : 80);

                BaseClass::Link().RemoteNode(Core::NodeId(remote.Host().Value().Text().c_str(), portNumber));

                result = true;
            }
            return (result);
        }

        inline bool HashStringToBytes(const std::string& hash, uint8_t (&hashHex)[Crypto::HASH_SHA256])
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


    private:
        string _hash;
        Core::CriticalSection _adminLock;
        INotifier* _notifier;
        Core::File _storage;
    };
}
}
