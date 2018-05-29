#ifndef PROJECT_WAYLAND_H
#define PROJECT_WAYLAND_H

extern "C" {
namespace WPEFramework {
    namespace Implementation {
        struct IServer {
            virtual void SetInput(const char name[]) = 0;

            virtual ~IServer(){};
        };

        IServer* Create(const string& configuration);
    }
}
}
#endif //PROJECT_WAYLAND_H
