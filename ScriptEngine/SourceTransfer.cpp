#include "Module.h"
#include "SourceTransfer.h"

namespace Thunder {
    namespace Plugin {

        /* static */ Core::ProxyPoolType<Web::Response> SourceTransfer::_responseFactory(1);

        uint32_t SourceTransfer::Download(const string& url) {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;
            _adminLock.Lock();

            if (_url.IsValid() == true) {
                _adminLock.Unlock();
            }
            else {
                result = Core::ERROR_INCORRECT_URL;
                _url = Core::URL(url.c_str());

                if (_url.IsValid() == false) {
                    _adminLock.Unlock();
                }
                else if (_url.Type() == Core::URL::SchemeType::SCHEME_FILE) {
                        
                    if ((_url.Path().IsSet() == false) || (Core::File(_url.Path().Value()).Exists() == false)) {
                        _adminLock.Unlock();
                        result = Core::ERROR_BAD_REQUEST;
                    }
                    else {
                        Core::File content(_url.Path().Value());

                        if (content.Open(true) == false) {
                            _adminLock.Unlock();
                            result = Core::ERROR_OPENING_FAILED;
                        }
                        else {
                            uint32_t size = static_cast<uint32_t>(content.Size());
                            _source.reserve(size);
                            while (size != 0) {
                                uint8_t buffer[1024];
                                uint32_t readBytes = content.Read(buffer, sizeof(buffer));
                                size -= readBytes;
                                _source.append(reinterpret_cast<const char*>(buffer), readBytes);
                            }
                            _adminLock.Unlock();

                            _callback->Transfered(url, Core::ERROR_NONE);
                            result = Core::ERROR_NONE;
                        }
                    }
                }
                else if (_url.Type() == Core::URL::SchemeType::SCHEME_HTTP) {
                    _downloader = Core::ProxyType<WebClient>::Create(*this, _url);
                    _adminLock.Unlock();
                    result = Core::ERROR_INPROGRESS;

                }
                else if (_url.Type() == Core::URL::SchemeType::SCHEME_HTTPS) {
                    _downloader = Core::ProxyType<SecureWebClient>::Create(*this, _url);
                    _adminLock.Unlock();

                    result = Core::ERROR_INPROGRESS;
                }
                else {
                    _url.Clear();
                    _adminLock.Unlock();
                    result = Core::ERROR_INVALID_DESIGNATOR;
                }
            }
            return (result);
        }
        uint32_t SourceTransfer::Abort() {

            _adminLock.Lock();

            if (_url.IsValid() == true) {
                if (_downloader.IsValid() == true) {
                    _downloader.Release();
                }
                _url.Clear();
            }
            _adminLock.Unlock();
            return (Core::ERROR_NONE);
        }
    }
}