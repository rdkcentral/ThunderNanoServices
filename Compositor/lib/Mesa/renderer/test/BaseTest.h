#include <IOutput.h>
#include <IBuffer.h>
#include <IRenderer.h>

using namespace Thunder;

namespace {
class BaseTest {

    class Sink : public Compositor::IOutput::ICallback {
    public:
        Sink(const Sink&) = delete;
        Sink& operator=(const Sink&) = delete;
        Sink() = delete;

        Sink(BaseTest& parent)
            : _parent(parent)
        {
        }

        virtual ~Sink() = default;

        virtual void Presented(const Compositor::IOutput* output, const uint64_t sequence, const uint64_t time) override
        {
            std::cerr << "Presented!" << std::endl;
            _parent.HandleVSync(output, sequence, time);
        }

        virtual void Terminated(const Compositor::IOutput* output) override
        {
            TRACE_GLOBAL(Thunder::Trace::Information, ("Output terminated, exiting application"));

            _parent.Stop();
            _parent.RequestExit();
            _parent.HandleVSync(output, 0, 0);
        }

    private:
        BaseTest& _parent;
    };

public:
    BaseTest() = delete;
    BaseTest(const BaseTest&) = delete;
    BaseTest& operator=(const BaseTest&) = delete;

    BaseTest(const std::string& connectorId, const std::string& renderId, const uint16_t mFramePerSecond)
        : _renderer()
        , _connector()
        , _period(mFramePerSecond > 0 ? std::chrono::microseconds(1000000000) / mFramePerSecond : std::chrono::microseconds(16667))
        , _running(false)
        , _render()
        , _renderFd(::open(renderId.c_str(), O_RDWR))
        , _sink(*this)
        , _rendering()
        , _vsync()
        , _ppts(Core::Time::Now().Ticks())
        , _fps(0.0f)
        , _sequence(0)
        , _exitMutex()
        , _exitSignal()
        , _exitRequested(false)
    {
        _renderer = Compositor::IRenderer::Instance(_renderFd);
        ASSERT(_renderer.IsValid());

        _connector = Compositor::CreateBuffer(
            connectorId, 1920, 1080, mFramePerSecond,
            Compositor::PixelFormat::Default(),
            _renderer, &_sink);

        ASSERT(_connector.IsValid());
    }

    virtual ~BaseTest()
    {
        Stop();

        _renderer.Release();
        _connector.Release();

        ::close(_renderFd);
    }

    void Start()
    {
        bool expected = false;

        if (_running.compare_exchange_strong(expected, true)) {
            TRACE(Trace::Information, ("Starting BaseTest"));
            _render = std::thread(&BaseTest::Render, this);
        }
    }

    void Stop()
    {
        bool expected = true;

        if (_running.compare_exchange_strong(expected, false)) {
            TRACE(Trace::Information, ("Stopping BaseTest"));
            _render.join();
        }
    }

    bool IsRunning() const
    {
        return _running.load();
    }

    void RequestExit()
    {
        TRACE(Trace::Information, ("Exit requested via output termination"));
        std::lock_guard<std::mutex> lock(_exitMutex);
        _exitRequested = true;
        _exitSignal.notify_one();
    }

    bool ShouldExit() const
    {
        return _exitRequested.load();
    }

    float GetFPS() const
    {
        return _fps.load();
    }

private:
    void Render()
    {
        while (_running.load() && !ShouldExit()) {
            const auto next = _period - NewFrame();
            std::this_thread::sleep_for((next.count() > 0) ? next : std::chrono::microseconds(0));
        }
    }

    void HandleVSync(const Compositor::IOutput* output VARIABLE_IS_NOT_USED, const uint64_t sequence, uint64_t pts /*usec from epoch*/)
    {
        if (_ppts != 0) { // Skip first frame
            uint64_t frameDelta = pts - _ppts;

            if (frameDelta > 0) { // Prevent division by zero
                _fps = 1000000.0f / frameDelta; // Convert microseconds to fps
            } else {
                _fps = 0.0f; // Same timestamp or time went backwards
            }
        } else {
            _fps = 0.0f; // First frame
        }

        _sequence = sequence;
        _ppts = pts;
        _vsync.notify_all();
    }

protected:
    virtual std::chrono::microseconds NewFrame() = 0;

    void WaitForVSync(uint32_t timeoutMs)
    {
        std::unique_lock<std::mutex> lock(_rendering);

        if (timeoutMs == Core::infinite) {
            _vsync.wait(lock);
        } else {
            _vsync.wait_for(lock, std::chrono::milliseconds(timeoutMs));
        }

        // TRACE(Trace::Information, ("Connector running at %.2f fps", _fps));
    }

    Core::ProxyType<Compositor::IRenderer> Renderer() const
    {
        return _renderer;
    }

    Core::ProxyType<Compositor::IOutput> Connector() const
    {
        return _connector;
    }

    const std::chrono::microseconds& Period() const
    {
        return _period;
    }

private:
    Core::ProxyType<Compositor::IRenderer> _renderer;
    Core::ProxyType<Compositor::IOutput> _connector;
    const std::chrono::microseconds _period;
    std::atomic<bool> _running;
    std::thread _render;
    int _renderFd;
    Sink _sink;
    std::mutex _rendering;
    std::condition_variable _vsync;
    uint64_t _ppts;
    std::atomic<float> _fps;
    uint64_t _sequence;
    std::mutex _exitMutex;
    std::condition_variable _exitSignal;
    std::atomic<bool> _exitRequested;
}; // BaseTest
} // namespace