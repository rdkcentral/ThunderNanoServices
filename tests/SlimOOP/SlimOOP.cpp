#include "SlimOOP.h"

namespace Thunder {
namespace Plugin {

    namespace {
        static Metadata<SlimOOP> metadata(1, 0, 0, {}, {}, {});
    }

    const string SlimOOP::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_implementation == nullptr);
        ASSERT(_connectionId == 0);
        string retval;

        _implementation = service->Root<Core::IUnknown>(_connectionId, 2000, _T("SlimOOPImplementation"));

        if (_implementation == nullptr) {
            retval = _T("Could not create SlimOOPImplementation out of process");
        }

        return retval;
    }

    void SlimOOP::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        RPC::IRemoteConnection* connection = service->RemoteConnection(_connectionId);

        if (_implementation != nullptr) {
            _implementation->Release();
            _implementation = nullptr;
        }
        if (connection != nullptr) {
            connection->Terminate();
            connection->Release();
        }
        _connectionId = 0;
    }

    string SlimOOP::Information() const
    {
        return string();
    }

}
}