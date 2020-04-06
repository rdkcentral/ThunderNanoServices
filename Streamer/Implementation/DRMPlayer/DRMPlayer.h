#pragma once

#include "Administrator.h"
#include <atomic>
#include <gst/gst.h>
#include <vector>

namespace WPEFramework {
namespace Player {
    namespace Implementation {

        namespace {

            class DRMPlayer : public IPlayerPlatform, Core::Thread {
            public:
                DRMPlayer(const DRMPlayer&) = delete;
                DRMPlayer& operator=(const DRMPlayer&) = delete;

                DRMPlayer(const Exchange::IStream::streamtype streamType, const uint8_t index);

            public:
                struct PipelineData {
                    PipelineData()
                        : _playbin(nullptr)
                        , _mainLoop(nullptr)
                    {
                    }

                    GstElement* _playbin;
                    GMainLoop* _mainLoop;
                };

            public:
                // IPlayerPlatform methods
                // --------------------------------------------------------------
                uint32_t Setup() override;
                uint32_t Teardown() override;
                void Callback(ICallback* callback) override;
                string Metadata() const override;
                Exchange::IStream::streamtype Type() const override;
                Exchange::IStream::drmtype DRM() const override;
                Exchange::IStream::state State() const override;
                uint32_t Error() const override;
                uint8_t Index() const override;
                uint32_t Load(const string& uri) override;
                uint32_t AttachDecoder(const uint8_t) override;
                uint32_t DetachDecoder(const uint8_t) override;
                uint32_t Speed(const int32_t speed) override;
                int32_t Speed() const override;
                const std::vector<int32_t>& Speeds() const override;
                void Position(const uint64_t absoluteTime) override;
                uint64_t Position() const override;
                void TimeRange(uint64_t& begin, uint64_t& end) const override;
                const Rectangle& Window() const override;
                void Window(const Rectangle& rectangle) override;
                uint32_t Order() const override;
                void Order(const uint32_t order) override;
                const std::list<ElementaryStream>& Elements() const override;

                // Core::Thread method
                // ------------------------
                uint32_t Worker() override;

            private:
                // Gstreamer helper methods
                // ----------------------------------------------
                uint32_t SetupGstElements();
                uint32_t TeardownGstElements();
                void SendSeekEvent();
                GstEvent* CreateSeekEvent(const uint64_t position);

                // OCDM helper methods
                // -----------------------------------------------
                void initializeOcdm();

                uint8_t _index;
                int32_t _speed;
                std::vector<int32_t> _vecSpeeds;
                Exchange::IStream::streamtype _streamType;
                uint32_t _error;

                mutable Core::CriticalSection _adminLock;
                PipelineData _data;
                GstBus* _bus;

                // OpenCDMSystem* _system;
            };

            class Config : public Core::JSON::Container {
            public:
                Config(const Config&) = delete;
                Config& operator=(const Config&) = delete;

                Config()
                    : Core::JSON::Container()
                    , Speeds()
                {
                    Add(_T("speeds"), &Speeds);
                }

                Core::JSON::ArrayType<Core::JSON::DecSInt32> Speeds;
            } config;
        } // namespace

    } // namespace Implementation
} // namespace Player
}