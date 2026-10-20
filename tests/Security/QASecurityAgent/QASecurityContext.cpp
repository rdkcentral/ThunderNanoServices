#include "QASecurityContext.h"

namespace Thunder {
namespace Plugin {

QASecurityContext::QASecurityContext(
    const string& token,
    const Policy& policy)
    : _token(token)
    , _policy(policy)
    , _pathCalls(0)
    , _httpCalls(0)
    , _jsonRpcCalls(0)
{
}

bool QASecurityContext::Allowed(const string& path VARIABLE_IS_NOT_USED) const
{
    ++_pathCalls;

    return (_policy.path == Decision::ALLOW);
}

bool QASecurityContext::Allowed(
    const Web::Request& request VARIABLE_IS_NOT_USED) const
{
    ++_httpCalls;

    return (_policy.http == Decision::ALLOW);
}

bool QASecurityContext::Allowed(
    const Core::JSONRPC::Message& message VARIABLE_IS_NOT_USED) const
{
    ++_jsonRpcCalls;

    return (_policy.jsonrpc == Decision::ALLOW);
}

string QASecurityContext::Token() const
{
    return _token;
}

uint32_t QASecurityContext::PathCalls() const
{
    return _pathCalls.load();
}

uint32_t QASecurityContext::HttpCalls() const
{
    return _httpCalls.load();
}

uint32_t QASecurityContext::JsonRpcCalls() const
{
    return _jsonRpcCalls.load();
}

} // namespace Plugin
} // namespace Thunder
