#ifndef __COMMANDER_H
#define __COMMANDER_H

#include "Module.h"
#include <interfaces/ICommand.h>

namespace WPEFramework {
namespace Plugin {

    class Commander : public PluginHost::IPlugin, public PluginHost::IWeb, public Exchange::ICommand::IRegistration {
    public:
        enum state {
            IDLE,
            LOADED,
            RUNNING,
            ABORTING
        };
        class Command : public Core::JSON::Container {
        public:
            Command()
                : Core::JSON::Container()
                , Item()
                , Label()
                , Parameters(false)
            {
                Add(_T("command"), &Item);
                Add(_T("label"), &Label);
                Add(_T("parameters"), &Parameters);
            }
            Command(const Command& copy)
                : Core::JSON::Container()
                , Item(copy.Item)
                , Label(copy.Label)
                , Parameters(copy.Parameters)
            {
                Add(_T("command"), &Item);
                Add(_T("label"), &Label);
                Add(_T("parameters"), &Parameters);
            }
            ~Command()
            {
            }

            Command& operator=(const Command& RHS)
            {
                Item = RHS.Item;
                Label = RHS.Label;
                Parameters = RHS.Parameters;

                return (*this);
            }

        public:
            Core::JSON::String Item;
            Core::JSON::String Label;
            Core::JSON::String Parameters;
        };

        class Data : public Core::JSON::Container {
        public:
            Data()
                : Core::JSON::Container()
            {
                Add(_T("sequencer"), &Sequencer);
                Add(_T("state"), &State);
                Add(_T("index"), &Index);
                Add(_T("label"), &Label);
                Add(_T("command"), &Command);
            }
            Data(const string& name, const state actualState, const uint32_t index, const string& label)
                : Core::JSON::Container()
            {
                Add(_T("sequencer"), &Sequencer);
                Add(_T("state"), &State);
                Add(_T("index"), &Index);
                Add(_T("label"), &Label);
                Add(_T("command"), &Command);

                Sequencer = name;
                State = actualState;
                Index = index;
                Label = label;
            }
            Data(const Data& copy)
                : Core::JSON::Container()
                , Sequencer(copy.Sequencer)
                , State(copy.State)
                , Index(copy.Index)
                , Label(copy.Label)
                , Command(copy.Command)
            {
                Add(_T("sequencer"), &Sequencer);
                Add(_T("state"), &State);
                Add(_T("index"), &Index);
                Add(_T("label"), &Label);
                Add(_T("Command"), &Command);
            }
            ~Data()
            {
            }

            Data& operator=(const Data& RHS)
            {
                Sequencer = RHS.Sequencer;
                State = RHS.State;
                Index = RHS.Index;
                Label = RHS.Label;
                Command = RHS.Command;

                return (*this);
            }

        public:
            Core::JSON::String Sequencer;
            Core::JSON::EnumType<state> State;
            Core::JSON::DecUInt32 Index;
            Core::JSON::String Label;
            Core::JSON::String Command;
        };

    private:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config& copy) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
            {
                Add(_T("sequencers"), &Sequencers);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::ArrayType<Core::JSON::String> Sequencers;
        };
        class Administrator {
        private:
            Administrator(const Administrator&);
            Administrator operator=(const Administrator&);

        public:
            typedef Core::IteratorMapType<std::map<const string, Exchange::ICommand::IFactory*>, Exchange::ICommand::IFactory*, const string, std::map<const string, Exchange::ICommand::IFactory*>::iterator> Iterator;

        public:
            Administrator()
                : _adminLock()
                , _factory()
            {
            }
            ~Administrator()
            {
            }

        public:
            inline void Register(const string& className, Exchange::ICommand::IFactory* factory)
            {
                _adminLock.Lock();

                std::map<const string, Exchange::ICommand::IFactory*>::iterator index(_factory.find(className));

                ASSERT(index == _factory.end());

                if (index == _factory.end()) {
                    _factory.insert(std::pair<const string, Exchange::ICommand::IFactory*>(className, factory));
                }

                _adminLock.Unlock();
            }
            inline Exchange::ICommand::IFactory* Unregister(const string& className)
            {

                Exchange::ICommand::IFactory* result = nullptr;

                _adminLock.Lock();

                std::map<const string, Exchange::ICommand::IFactory*>::iterator index(_factory.find(className));

                ASSERT(index != _factory.end());

                if (index != _factory.end()) {
                    result = index->second;
                    _factory.erase(index);
                }

                _adminLock.Unlock();

                return (result);
            }
            Core::ProxyType<Exchange::ICommand> Create(const string& label, const string& className, const string& parameters)
            {
                Core::ProxyType<Exchange::ICommand> result;

                _adminLock.Lock();

                std::map<const string, Exchange::ICommand::IFactory*>::iterator index(_factory.find(className));

                ASSERT(index != _factory.end());

                if (index != _factory.end()) {
                    result = index->second->Create(label, parameters);
                }

                _adminLock.Unlock();

                return (result);
            }

            Iterator Commands()
            {
                return (Iterator(_factory));
            }

        private:
            Core::CriticalSection _adminLock;
            std::map<const string, Exchange::ICommand::IFactory*> _factory;
        };
        class Sequencer : public Core::IDispatchType<void> {
        private:
            Sequencer() = delete;
            Sequencer(const Sequencer& copy) = delete;
            Sequencer& operator=(const Sequencer&) = delete;

        public:
            Sequencer(const string& name, Administrator* commandFactory, PluginHost::IShell* service)
                : _commandFactory(commandFactory)
                , _adminLock()
                , _currentIndex(0)
                , _state(Commander::IDLE)
                , _name(name)
                , _service(service)
                , _sequenceList(5)
            {
                ASSERT (service != nullptr);

                if (_service != nullptr) {
                    _service->AddRef();
                }
            }
            ~Sequencer()
            {
                // Make sure we are not executing anything if we get destructed.
                Abort();

                if (_service != nullptr) {
                    _service->Release();
                }
            }

        public:
            inline const string& Name() const
            {
                return (_name);
            }
            inline bool IsActive() const
            {
                return ((_state != Commander::IDLE) && (_state != Commander::LOADED));
            }
            inline state State() const
            {
                return (_state);
            }
            inline uint32_t Index() const
            {

                uint32_t result = static_cast<uint32_t>(~0);

                _adminLock.Lock();

                if ((_state != Commander::IDLE) && (_state != Commander::LOADED)) {

                    result = _currentIndex;
                }

                _adminLock.Unlock();

                return (result);
            }
            inline string Label() const
            {

                string result;

                _adminLock.Lock();

                if ((_state != Commander::IDLE) && (_state != Commander::LOADED) && (_currentIndex < _sequenceList.Count())) {

                    result = _sequenceList[_currentIndex]->Label();
                }

                _adminLock.Unlock();

                return (result);
            }
            uint32_t Load(const Core::JSON::ArrayType<Command>& commandList)
            {

                _adminLock.Lock();

                // Only Load data if we are NOT active !!!
                ASSERT(IsActive() == false);

                if (IsActive() == false) {

                    ASSERT(_commandFactory != nullptr);

                    if (_sequenceList.Count() > 0) {
                        _sequenceList.Clear(0, _sequenceList.Count());
                    }

                    Core::JSON::ArrayType<Command>::ConstIterator index(commandList.Elements());

                    while (index.Next() == true) {

                        const string& label(index.Current().Label);
                        const string& className(index.Current().Item);
                        const string& parameters(index.Current().Parameters);

                        Core::ProxyType<Exchange::ICommand> newCommand(_commandFactory->Create(label, className, parameters));

                        if (newCommand.IsValid() == true) {
                            _sequenceList.Add(newCommand);
                        }
                    }

                    if (_sequenceList.Count() > 0) {
                        _state = Commander::LOADED;
                        _currentIndex = 0;
                    }
                }

                _adminLock.Unlock();

                return (_sequenceList.Count());
            }
            uint32_t Execute()
            {

                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _adminLock.Lock();

                if (_state == Commander::LOADED) {
                    result = Core::ERROR_NONE;
                    _state = Commander::RUNNING;
                }

                _adminLock.Unlock();

                return (result);
            }
            uint32_t Abort()
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _adminLock.Lock();

                if (_state == Commander::RUNNING) {
                    result = Core::ERROR_NONE;
                    _state = Commander::ABORTING;
                    _sequenceList[_currentIndex]->Abort();
                }

                _adminLock.Unlock();

                // Wait for the sequencer to reaach a safe positon..
                return (result);
            }

        private:
            virtual void Dispatch()
            {
                _adminLock.Lock();

                // See if we still need to take some "next steps"
                while ((_currentIndex < _sequenceList.Count()) && (_state == Commander::RUNNING)) {

                    Core::ProxyType<Exchange::ICommand> step(_sequenceList[_currentIndex]);

                    _adminLock.Unlock();

                    const string result = step->Execute(_service);

                    _adminLock.Lock();

                    if (result.empty() == true) {
                        _currentIndex++;
                    }
                    else {
                        uint32_t index = _currentIndex + 1;

                        // See if we have a forward label, as mentioned from the execute
                        while ((index < _sequenceList.Count()) && (_sequenceList[index]->Label() != result)) {
                            index++;
                        }

                        if (index < _sequenceList.Count()) {
                            // Seems like we found a next step, set it..
                            _currentIndex = index;
                        }
                        else {
                            // There are no steps before our current step, so no label found, just progress...
                            _currentIndex++;

                            // But let's check if there is a step before us (or we are ourselves :-), we might need to jump to..
                            index = _currentIndex;

                            // Check if we have a step with the given label prior to our current step..
                            while ((index > 0) && (_sequenceList[index - 1]->Label() != result)) {
                                index--;
                            }

                            if (index > 0) {
                                _currentIndex = (index - 1);
                            }
                        }
                    }
                }

                ASSERT((_state == Commander::RUNNING) || (_state == Commander::ABORTING));
                _state = IDLE;

                _sequenceList.Clear(0, _sequenceList.Count());

                _adminLock.Unlock();
            }

        private:
            Administrator* _commandFactory;
            mutable Core::CriticalSection _adminLock;
            uint32_t _currentIndex;
            state _state;
            string _name;
            PluginHost::IShell* _service;
            Core::ProxyList<Exchange::ICommand> _sequenceList;
        };

        Commander(const Commander&) = delete;
        Commander& operator=(const Commander&) = delete;

    public:
        Commander();
        virtual ~Commander();

        BEGIN_INTERFACE_MAP(Commander)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service);
        virtual void Deinitialize(PluginHost::IShell* service);
        virtual string Information() const;

        //	IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request);
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request);

        //	IExchange::ICommand::IRegistration methods
        // -------------------------------------------------------------------------------------------------------
        template <typename COMMAND>
        void Register()
        {
            const string name (Core::ClassNameOnly(typeid(COMMAND).name()).Data());
            _commandAdministrator.Register(name, new Exchange::Command::FactoryType<COMMAND>());
        }
        template <typename COMMAND>
        void Unregister()
        {
            Exchange::ICommand::IFactory* result = _commandAdministrator.Unregister(string(Core::ClassNameOnly(typeid(COMMAND).name()).Data()));

            if (result != nullptr) {
                delete result;
            }
        }
        virtual void Register(const string& className, Exchange::ICommand::IFactory* factory)
        {
            _commandAdministrator.Register(className, factory);
        }
        virtual Exchange::ICommand::IFactory* Unregister(const string& className)
        {
            return (_commandAdministrator.Unregister(className));
        }

    private:
        Commander::Data MetaData(const Commander::Sequencer& sequencer);

    private:
        uint8_t _skipURL;
        PluginHost::IShell* _service;
        Administrator _commandAdministrator;
        std::map<const string, Core::ProxyType<Sequencer> > _sequencers;
    };

}
} 

#endif // __COMMANDER_H
