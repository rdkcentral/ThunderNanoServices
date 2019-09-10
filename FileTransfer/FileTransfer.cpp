#include "FileTransfer.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(FileTransfer, 1, 0);

    const string FileTransfer::Initialize(PluginHost::IShell* service)
    {
        Config config;
        config.FromString(service->ConfigLine());

        _logOutput.SetDestination(config.Destination.Binding.Value(), config.Destination.Port.Value());
        _observer.Register(config.FilePath.Value(), &_logOutput, IN_CLOSE_WRITE, config.FullFile.Value());

        return string();
    }

    void FileTransfer::Deinitialize(PluginHost::IShell* service)
    {
        _observer.Unregister();
    }

    string FileTransfer::Information() const
    {
        return string();
    }
} // namespace Plugin
} // namespace WPEFramework
