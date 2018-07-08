#ifndef __REMOTECONTROL_GREENPEAK__
#define __REMOTECONTROL_GREENPEAK__

#include "Module.h"
#include "interfaces/IKeyHandler.h"

namespace WPEFramework {
namespace Plugin {

    class GreenPeak : public Exchange::IKeyProducer {
    private:
        GreenPeak(const GreenPeak&);
        GreenPeak& operator=(const GreenPeak&);

        class Config : public Core::JSON::Container {
        private:
            Config (const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config ()
                : Core::JSON::Container()
                , CodeMask(static_cast<uint32_t>(~0))
            {
                Add(_T("codemask"), &CodeMask);
            }
            ~Config()
            {
            }

            Core::JSON::HexUInt32 CodeMask;
        };
        class Activity : public Core::Thread {
        private:
            Activity(const Activity&);
            Activity& operator=(const Activity&);

        public:
            Activity();
            virtual ~Activity();

        public:
            void Dispose();
            virtual uint32_t Worker();
        };

        class Info : public Core::JSON::Container {
        private:
            Info(const Info&);
            Info& operator=(const Info&);

        public:
            Info()
                : Core::JSON::Container()
                , Major(static_cast<uint16_t>(~0))
                , Minor(static_cast<uint16_t>(~0))
                , Revision(static_cast<uint16_t>(~0))
                , Patch(static_cast<uint16_t>(~0))
                , Change(static_cast<uint16_t>(~0))
            {
                Add(_T("major"), &Major);
                Add(_T("minor"), &Minor);
                Add(_T("revision"), &Revision);
                Add(_T("patch"), &Patch);
                Add(_T("change"), &Change);
            }
            ~Info()
            {
            }

        public:
            Core::JSON::DecUInt16 Major;
            Core::JSON::DecUInt16 Minor;
            Core::JSON::DecUInt16 Revision;
            Core::JSON::DecUInt16 Patch;
            Core::JSON::DecUInt16 Change;
        };

    public:
        GreenPeak();
        ~GreenPeak();

        BEGIN_INTERFACE_MAP(GreenPeak)
        INTERFACE_ENTRY(Exchange::IKeyProducer)
        END_INTERFACE_MAP

    public:
        virtual const TCHAR* Name() const
        {
            return (_resourceName.c_str());
        }
        virtual void Configure(const string& configure)
        {
            Config config; config.FromString(configure);
            _codeMask = config.CodeMask.Value();
        }


        virtual bool Pair();
        virtual bool Unpair(string bindingId);
        virtual uint32_t Callback(Exchange::IKeyHandler* callback);
        virtual uint32_t Error() const;
        virtual string MetaData() const;

        bool WaitForReady(const uint32_t time) const;

        // -------------------------------------------------------------------------------
        // The greenpeak library is C oriented. These methods should not be used by the
        // consumers of this class but are to link the greenpeak C works to the C++ world.
        // These methods should only be used by the GreenPeak C methods in the GreenPeak
        // implementation file.
        // -------------------------------------------------------------------------------
        void _Dispatch(const bool pressed, const uint16_t code, const uint16_t modifiers);
        static void Dispatch(const bool pressed, const uint16_t code, const uint16_t modifiers);

        void _Initialized(uint16_t major, uint16_t minor, uint16_t revision, uint16_t patch, const uint32_t change);
        static void Initialized(const uint16_t major, const uint16_t minor, const uint16_t revision, const uint16_t patch, const uint32_t change);

        static void Report(const string& text);

    private:
        mutable Core::CriticalSection _adminLock;
        mutable WPEFramework::Core::Event _readySignal;
        Exchange::IKeyHandler* _callback;
        Activity _worker;
        Info _info;
        uint32_t _error;
        bool _present;
        uint32_t _codeMask;
      /* static */ const string _resourceName;
    };
}
}

#endif // __REMOTECONTROL_GREENPEAK__
