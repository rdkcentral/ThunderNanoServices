#include "RequestSender.h"

namespace WPEFramework {
namespace Plugin {
    Core::ProxyPoolType<Web::Response> RequestSender::WebClient::_responseFactory(5);
}
}