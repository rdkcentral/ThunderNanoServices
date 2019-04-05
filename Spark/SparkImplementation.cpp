#include "Module.h"
#include <interfaces/IMemory.h>
#include <interfaces/IBrowser.h>

#include <fstream>
#include <pxFont.h>
#include <pxContext.h>
#include <pxEventLoop.h>
#include <pxScene2d.h>
#include <pxWindow.h>
#include <pxTimer.h>
#include <rtPathUtils.h>
#include <rtScript.h>
#include <rtUrlUtils.h>

#include <pxUtil.h>
#include <rtSettings.h>

pxContext context;
extern rtScript script;

#define ENTERSCENELOCK() rtWrapperSceneUpdateEnter();
#define EXITSCENELOCK()  rtWrapperSceneUpdateExit();

namespace WPEFramework {
namespace Plugin {

    class SparkImplementation : public Exchange::IBrowser, public PluginHost::IStateControl {
    private:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            Config()
                : Core::JSON::Container()
                , Url()
                , Width(1280)
                , Height(720)
                , AnimationFPS(60)
            {
                Add(_T("url"), &Url);
                Add(_T("width"), &Width);
                Add(_T("height"), &Height);
                Add(_T("animationfps"), &AnimationFPS);
            }
            ~Config() {}

        public:
            Core::JSON::String Url;
            Core::JSON::DecUInt32 Width;
            Core::JSON::DecUInt32 Height;
            Core::JSON::DecUInt8 AnimationFPS;
        };

        class SceneWindow : public Core::Thread, public pxWindow, public pxIViewContainer {
        private:
            SceneWindow(const SceneWindow&) = delete;
            SceneWindow& operator=(const SceneWindow&) = delete;

        public:
            SceneWindow()
                : Core::Thread(Core::Thread::DefaultStackSize(), _T("Spark"))
                , _eventLoop()
                , _view(nullptr)
                , _closed(false)
                , _initialized(false)
                , _width(~0)
                , _height(~0)
                , _animationFPS(~0)
                , _scene(nullptr)
                , _sceneContainer(nullptr)
            {
            }
            virtual ~SceneWindow()
            {
                Stop();
                Quit();

                Wait(Thread::STOPPED | Thread::BLOCKED, Core::infinite);

                if (_view != nullptr) {
                    //context.term();

                    // pxScene.cpp:104:12: warning: deleting object of abstract class type
                    // ‘pxIView’ which has non-virtual destructor will cause undefined
                    // behaviour [-Wdelete-non-virtual-dtor]
                    static_cast<pxIView*>(_view)->setViewContainer(nullptr);
                    static_cast<pxIView*>(_view)->Release(); //FIXME: Could not able to delete it, that means it is still in use.
                    _view = nullptr;
                }
                if (_sceneContainer != nullptr) {
                    delete _sceneContainer;
                    _sceneContainer = nullptr;
                }
                if (_scene != nullptr) {
                    delete _scene;
                    _scene = nullptr;
                }
            }

            uint32_t Configure(PluginHost::IShell* service) {
                uint32_t result = Core::ERROR_NONE;

                static constexpr TCHAR basePermissions[] = _T("sparkpermissions.conf");
                static constexpr TCHAR baseConfig[] = _T(".sparkSettings.json");
                static constexpr TCHAR baseFPS[] = _T(".sparkFps");

                Config config;
                config.FromString(service->ConfigLine());

                _url = config.Url.Value();
                _width = config.Width.Value();
                _height = config.Height.Value();
                _animationFPS = config.AnimationFPS.Value();

                // Check if there is a persistent config file, that will overrule the one that is in the ROM.
                string permissionsFile (service->PersistentPath() + basePermissions);
                string configFile (service->PersistentPath() + baseConfig);
                string fpsFile (service->PersistentPath() + baseFPS);

                if (Core::File(permissionsFile).Exists() != true) {
                    permissionsFile = service->DataPath() + basePermissions;
                }
                if (Core::File(configFile).Exists() != true) {
                    configFile = service->DataPath() + baseConfig;
                }
                if (Core::File(fpsFile).Exists() != true) {
                    fpsFile = service->DataPath() + baseFPS;
                }

                if (Core::File(permissionsFile).Exists() == true) {
                    Core::SystemInfo::SetEnvironment(_T("SPARK_PERMISSIONS_ENABLED"), _T("true"));
                    Core::SystemInfo::SetEnvironment(_T("SPARK_PERMISSIONS_CONFIG"), permissionsFile);
                }
                if (Core::File(configFile).Exists() == true) {
                    rtSettings::instance()->loadFromFile(rtString(configFile.c_str()));

                    rtValue screenWidth, screenHeight;
                    if (RT_OK == rtSettings::instance()->value("screenWidth", screenWidth)) {
                        _width = screenWidth.toInt32();
                    }

                    if (RT_OK == rtSettings::instance()->value("screenHeight", screenHeight)) {
                        _height = screenHeight.toInt32();
                    }
                }

                if (Core::File(fpsFile).Exists() == true) {
                    std::fstream fs(fpsFile, std::fstream::in);
                    uint32_t val = 0;
                    fs >> val;
                    if (val > 0) {
                        _animationFPS = val;
                    }
                }
                Core::SystemInfo::SetEnvironment(_T("NODE_PATH"), service->DataPath());
                Core::SystemInfo::SetEnvironment(_T("PXSCENE_PATH"), service->DataPath());

                return result;
            }

            void InitScene ()
            {
                ENTERSCENELOCK()
                if (!_initialized) {
                    script.init();

                    pxWindow::init(0, 0, _width, _height);

                    context.init();
                    TCHAR buffer[64];

                    setAnimationFPS(_animationFPS);

                    sprintf(buffer, "Spark: %s", PX_SCENE_VERSION);
                    setTitle(buffer);

                    _scene = new pxScene2d();
                    _sceneContainer = new pxSceneContainer(_scene);

                    _initialized = true;
                }
                ENTERSCENELOCK()
            }

            string GetURL() const {
                return (_url);
            }

            void SetURL(const string& url)
            {
                _url = url;

                if (_initialized == true) { // Ensure the sceneWindow is properly initialized before setting URL

                    if (url.empty() == true)
                    {
                        ENTERSCENELOCK()

                        if (_view != nullptr) {
                            delete _view;
                            _view = nullptr;
                        }

                        Quit();

                        EXITSCENELOCK()

                    } else {
                        TCHAR prefix[] = _T("shell.js?url=");
                        TCHAR buffer[MAX_URL_SIZE + sizeof(prefix)];
                        Core::URL analyser(url.c_str());
                        uint16_t length = 0;

                        strncpy(buffer, prefix, sizeof(prefix));
                        if ((analyser.IsValid() == true) && ((analyser.Type() == Core::URL::SCHEME_HTTP) || (analyser.Type() == Core::URL::SCHEME_HTTPS))) {
                            length = Core::URL::Encode(
                                         url.c_str(),
                                         static_cast<uint16_t>(url.length()),
                                         &(buffer[sizeof(prefix)]),
                                         sizeof(buffer) - sizeof(prefix));

                        } else {
                            length = std::min(url.length(), sizeof(buffer) - sizeof(prefix));

                            strncat(buffer, url.c_str(), length);
                        }

                        if (length >= (sizeof(buffer) - sizeof(prefix))) {

                            SYSLOG(Trace::Warning, (_T("URL size greater than 8000 bytes, so resetting url to browser.js")));
                            ::strcat(buffer, _T("browser.js"));
                        } else {
                            ::strncat(buffer, url.c_str(), length);
                        }

                        ENTERSCENELOCK()

                        _closed = false;

                        if (_view != nullptr) {
                            static_cast<pxIView*>(_view)->setViewContainer(nullptr);
                            static_cast<pxIView*>(_view)->Release(); //FIXME: Could not able to delete it, that means it is still in use.
                        }

                        _view = new pxScriptView(buffer,"javascript/node/v8", _sceneContainer);

                        ASSERT (_view != nullptr);

                        static_cast<pxIView*>(_view)->setViewContainer(this);
                        static_cast<pxIView*>(_view)->onSize(_width, _height);

                        EXITSCENELOCK()
                        Run();
                    }
                }
            }

            inline pxScene2d* GetScene()
            {
                ENTERSCENELOCK()
                rtValue args;
                args.setString("scene");
                rtValue scene;
                pxScriptView::getScene(1, &args, &scene, static_cast<void*>(_view));
                pxScene2d* pxScene = (pxScene2d*)(scene.toObject().getPtr());
                EXITSCENELOCK()
                return pxScene;
            }

            void Quit()
            { 
                Block();
                _eventLoop.exit();
            }

            void Hidden (const bool hidden) 
            {
                // JRJR TODO Why aren't these necessary for glut... pxCore bug
                setVisibility(!hidden);
            }

            uint8_t AnimationFPS() const
            {
                return (_animationFPS);
            }

        protected:
            virtual void invalidateRect(pxRect* r) override
            {
                pxWindow::invalidateRect(r);
            }

            virtual void* getInterface(const char* /* name */) override
            {
                return nullptr;
            }

            virtual void onCreate() override 
            {
            }

            virtual void onCloseRequest() override
            {
                ENTERSCENELOCK();
                if ((_view != nullptr) && (_closed == false)) {
                    _closed = true;

                    static_cast<pxIView*>(_view)->onCloseRequest();

                    pxFontManager::clearAllFonts();
                    script.pump();
                    script.collectGarbage();
                }
                EXITSCENELOCK();
            }

            virtual void onClose() override 
            {
            }

            virtual void onAnimationTimer() override
            {
                ENTERSCENELOCK();
                if ((_view != nullptr) && (!_closed)) {
                    static_cast<pxIView*>(_view)->onUpdate(pxSeconds());
                }
                EXITSCENELOCK();
                script.pump();
            }

            virtual void onSize(int32_t w, int32_t h) override
            {
                ENTERSCENELOCK();
                if (_view != nullptr) {
                    static_cast<pxIView*>(_view)->onSize(w, h);
                }
                EXITSCENELOCK();
            }

            virtual void onMouseDown(int32_t x, int32_t y, uint32_t flags) override
            {
                ENTERSCENELOCK();
                if (_view != nullptr) {
                    static_cast<pxIView*>(_view)->onMouseDown(x, y, flags);
                }
                EXITSCENELOCK();
            }
            virtual void onMouseUp(int32_t x, int32_t y, uint32_t flags) override
            {
                ENTERSCENELOCK();
                if (_view != nullptr) {
                    static_cast<pxIView*>(_view)->onMouseUp(x, y, flags);
                }
                EXITSCENELOCK();
            }

            virtual void onMouseLeave() override
            {
                ENTERSCENELOCK();
                if (_view != nullptr) {
                    static_cast<pxIView*>(_view)->onMouseLeave();
                }
                EXITSCENELOCK();
            }

            virtual void onMouseMove(int32_t x, int32_t y) override
            {
                ENTERSCENELOCK();
                if (_view != nullptr) {
                    static_cast<pxIView*>(_view)->onMouseMove(x, y);
                }
                EXITSCENELOCK();
            }

            virtual void onKeyDown(uint32_t keycode, uint32_t flags) override
            {
                ENTERSCENELOCK();
                if (_view != nullptr) {
                    static_cast<pxIView*>(_view)->onKeyDown(keycode, flags);
                }
                EXITSCENELOCK();
            }

            virtual void onKeyUp(uint32_t keycode, uint32_t flags) override
            {
                ENTERSCENELOCK();
                if (_view != nullptr) {
                    static_cast<pxIView*>(_view)->onKeyUp(keycode, flags);
                }
                EXITSCENELOCK();
            }

            virtual void onChar(uint32_t c) override
            {
                ENTERSCENELOCK();
                if (_view != nullptr) {
                    static_cast<pxIView*>(_view)->onChar(c);
                }
                EXITSCENELOCK();
            }

            virtual void onDraw(pxSurfaceNative) override
            {
                ENTERSCENELOCK();
                if (_view != nullptr) {
                    static_cast<pxIView*>(_view)->onDraw();
                }
                EXITSCENELOCK();
            }

            virtual void onMouseEnter() override
            {
                ENTERSCENELOCK();
                if (_view != nullptr) {
                    static_cast<pxIView*>(_view)->onMouseEnter();
                }
                EXITSCENELOCK();
            }

            virtual void onFocus() override
            {
                ENTERSCENELOCK();
                if (_view != nullptr) {
                    static_cast<pxIView*>(_view)->onFocus();
                }
                EXITSCENELOCK();
            }

            virtual void onBlur() override
            {
                ENTERSCENELOCK();
                if (_view != nullptr) {
                    static_cast<pxIView*>(_view)->onBlur();
                }
                EXITSCENELOCK();
            }

            virtual void onScrollWheel(float dx, float dy) override
            {
                ENTERSCENELOCK();
                if (_view != nullptr) {
                    static_cast<pxIView*>(_view)->onScrollWheel(dx, dy);
                }
                EXITSCENELOCK();
            }

            virtual void onDragDrop(int32_t x, int32_t y, int32_t type, const char* dropped) override
            {
                ENTERSCENELOCK();
                if (_view != nullptr) {
                    static_cast<pxIView*>(_view)->onDragDrop(x, y, type, dropped);
                }
                EXITSCENELOCK();
            }

        private:
            virtual bool Initialize()
            {
                InitScene();

                SetURL(_url);

                return true;
           }
            virtual uint32_t Worker()
            {
                _eventLoop.run();
                return (Core::infinite);
            }


        private:
            pxEventLoop _eventLoop;
            pxScriptView* _view;
            bool _closed;
            bool _initialized;
            uint32_t _width;
            uint32_t _height;
            uint8_t _animationFPS;
            string _url;

            pxScene2d* _scene;
            pxSceneContainer* _sceneContainer;
        };

        SparkImplementation(const SparkImplementation&) = delete;
        SparkImplementation& operator=(const SparkImplementation&) = delete;

    public:
        SparkImplementation()
            : _adminLock()
            , _window()
            , _state(PluginHost::IStateControl::UNINITIALIZED)
            , _sparkClients()
            , _stateControlClients()
        {
        }

        virtual ~SparkImplementation()
        {
        }

        virtual uint32_t Configure(PluginHost::IShell* service)
        {
            uint32_t result = _window.Configure(service);

            _window.Init();

            _state = PluginHost::IStateControl::RESUMED;

            return (result);
        }
        virtual void SetURL(const string& URL) override {
            _window.SetURL(URL);
        }
        virtual string GetURL() const override {
            return (_window.GetURL());
        }
        virtual uint32_t GetFPS() const override {
            return (_window.AnimationFPS());
        }
        virtual void Hide(const bool hidden) {
            _window.Hidden(hidden);
        }
        virtual void Register(Exchange::IBrowser::INotification* sink)
        {
            _adminLock.Lock();

            // Make sure a sink is not registered multiple times.
            ASSERT(std::find(_sparkClients.begin(), _sparkClients.end(), sink) == _sparkClients.end());

            _sparkClients.push_back(sink);
            sink->AddRef();

            _adminLock.Unlock();
        }

        virtual void Unregister(Exchange::IBrowser::INotification* sink)
        {
            _adminLock.Lock();

            std::list<Exchange::IBrowser::INotification*>::iterator index(
                std::find(_sparkClients.begin(), _sparkClients.end(), sink));

            // Make sure you do not unregister something you did not register !!!
            ASSERT(index != _sparkClients.end());

            if (index != _sparkClients.end()) {
                (*index)->Release();
                _sparkClients.erase(index);
            }

            _adminLock.Unlock();
        }

        virtual void Register(PluginHost::IStateControl::INotification* sink)
        {
            _adminLock.Lock();

            // Make sure a sink is not registered multiple times.
            ASSERT(std::find(_stateControlClients.begin(), _stateControlClients.end(), sink) == _stateControlClients.end());

            _stateControlClients.push_back(sink);
            sink->AddRef();

            _adminLock.Unlock();
        }

        virtual void Unregister(PluginHost::IStateControl::INotification* sink)
        {
            _adminLock.Lock();

            std::list<PluginHost::IStateControl::INotification*>::iterator index(std::find(_stateControlClients.begin(), _stateControlClients.end(), sink));

            // Make sure you do not unregister something you did not register !!!
            ASSERT(index != _stateControlClients.end());

            if (index != _stateControlClients.end()) {
                (*index)->Release();
                _stateControlClients.erase(index);
            }

            _adminLock.Unlock();
        }

        virtual PluginHost::IStateControl::state State() const
        {
            return (_state);
        }

        virtual uint32_t Request(const PluginHost::IStateControl::command command)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _adminLock.Lock();
            if (_state == PluginHost::IStateControl::UNINITIALIZED) {
                // Seems we are passing state changes before we reached an operational Spark.
                // Just move the state to what we would like it to be :-)
                _state = (command == PluginHost::IStateControl::SUSPEND ? PluginHost::IStateControl::SUSPENDED : PluginHost::IStateControl::RESUMED);
                result = Core::ERROR_NONE;
            }
            else {
                switch (command) {
                case PluginHost::IStateControl::SUSPEND:
                    if (_state == PluginHost::IStateControl::RESUMED) {
                        bool status = false;
                        rtValue r;
                        pxScene2d* scene = _window.GetScene();
                        scene->suspend(r, status);
                        StateChangeCompleted(status, PluginHost::IStateControl::SUSPEND);
                        result = Core::ERROR_NONE;
                    }
                    break;
                case PluginHost::IStateControl::RESUME:
                    if (_state == PluginHost::IStateControl::SUSPENDED) {
                        bool status = false;
                        rtValue r;
                        pxScene2d* scene = _window.GetScene();
                        scene->resume(r, status);
                        StateChangeCompleted(status, PluginHost::IStateControl::RESUME);
                        result = Core::ERROR_NONE;
                    }
                    break;
                default:
                    break;
                }
            }

            _adminLock.Unlock();
            return result;
        }

        BEGIN_INTERFACE_MAP(SparkImplementation)
            INTERFACE_ENTRY(Exchange::IBrowser)
            INTERFACE_ENTRY(PluginHost::IStateControl)
        END_INTERFACE_MAP

    private:
        void StateChangeCompleted(bool success, const PluginHost::IStateControl::command request)
        {

            if (success) {
                switch (request) {
                case PluginHost::IStateControl::RESUME:

                    _adminLock.Lock();

                    if (_state != PluginHost::IStateControl::RESUMED) {
                        StateChange(PluginHost::IStateControl::RESUMED);
                    }

                    _adminLock.Unlock();
                    break;
                case PluginHost::IStateControl::SUSPEND:

                    _adminLock.Lock();

                    if (_state != PluginHost::IStateControl::SUSPENDED) {
                        StateChange(PluginHost::IStateControl::SUSPENDED);
                    }

                    _adminLock.Unlock();
                    break;
                default:
                    ASSERT(false);
                    break;
                }
            }
            else {

                StateChange(PluginHost::IStateControl::EXITED);
            }
        }

        void StateChange(const PluginHost::IStateControl::state newState)
        {
            _adminLock.Lock();

            _state = newState;
            std::list<PluginHost::IStateControl::INotification*>::iterator index(_stateControlClients.begin());

            while (index != _stateControlClients.end()) {
                (*index)->StateChange(newState);
                index++;
            }
            _adminLock.Unlock();
        }
    private:
        mutable Core::CriticalSection _adminLock;
        SceneWindow _window;
        PluginHost::IStateControl::state _state;
        std::list<Exchange::IBrowser::INotification*> _sparkClients;
        std::list<PluginHost::IStateControl::INotification*> _stateControlClients;
    };

    SERVICE_REGISTRATION(SparkImplementation, 1, 0);

} /* namespace Plugin */

namespace Spark {

    class MemoryObserverImpl : public Exchange::IMemory {
    private:
        MemoryObserverImpl();
        MemoryObserverImpl(const MemoryObserverImpl&);
        MemoryObserverImpl& operator=(const MemoryObserverImpl&);

    public:
        MemoryObserverImpl(const uint32_t id)
            : _main(id == 0 ? Core::ProcessInfo().Id() : id)
            , _observable(false)
        {
        }
        ~MemoryObserverImpl() {}

    public:
        virtual void Observe(const uint32_t pid)
        {
            if (pid == 0) {
                _observable = false;
            } else {
                _main = Core::ProcessInfo(pid);
                _observable = true;
            }
        }
        virtual uint64_t Resident() const
        {
            return (_observable == false ? 0 : _main.Resident());
        }
        virtual uint64_t Allocated() const
        {
            return (_observable == false ? 0 : _main.Allocated());
        }
        virtual uint64_t Shared() const
        {
            return (_observable == false ? 0 : _main.Shared());
        }
        virtual uint8_t Processes() const { return (IsOperational() ? 1 : 0); }

        virtual const bool IsOperational() const
        {
            return (_observable == false) || (_main.IsActive());
        }

        BEGIN_INTERFACE_MAP(MemoryObserverImpl)
        INTERFACE_ENTRY(Exchange::IMemory)
        END_INTERFACE_MAP

    private:
        Core::ProcessInfo _main;
        bool _observable;
    };

    Exchange::IMemory* MemoryObserver(const uint32_t PID)
    {
        return (Core::Service<MemoryObserverImpl>::Create<Exchange::IMemory>(PID));
    }
}
} // namespace
