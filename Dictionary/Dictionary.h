/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#ifndef __DICTIONARY_H
#define __DICTIONARY_H

#include "Module.h"
#include <interfaces/IDictionary.h>
#include <interfaces/json/JDictionary.h>

namespace Thunder {
namespace Plugin {

    class Dictionary : public PluginHost::IPlugin, public Exchange::IDictionary, public PluginHost::JSONRPC {
    public:
        enum enumType {
            VOLATILE,
            PERSISTENT
        };

    private:
        Dictionary(const Dictionary&) = delete;
        Dictionary& operator=(const Dictionary&) = delete;

        class RuntimeEntry {
        public:
            RuntimeEntry()
                : _key()
                , _value()
                , _type(VOLATILE)
                , _dirty(false)
            {
            }
            RuntimeEntry(const RuntimeEntry& copy)
                : _key(copy._key)
                , _value(copy._value)
                , _type(copy._type)
                , _dirty(false)
            {
            }
            RuntimeEntry(const string& key, const string& value, const enumType& type)
                : _key(key)
                , _value(value)
                , _type(type)
                , _dirty(false)
            {
            }
            ~RuntimeEntry()
            {
            }

            RuntimeEntry& operator=(const RuntimeEntry& RHS)
            {
                _key = RHS._key;
                _value = RHS._value;
                _type = RHS._type;

                return (*this);
            }

        public:
            inline const string& Key() const
            {
                return (_key);
            }
            inline const string& Value() const
            {
                return (_value);
            }
            inline void Value(const string& value)
            {
                _dirty = true;
                _value = value;
            }
            inline enumType Type() const
            {
                return (_type);
            }

        private:
            string _key;
            string _value;
            enumType _type;
            bool _dirty;
        };

        using KeyValueContainer = std::list<RuntimeEntry>;
        using DictionaryMap = std::unordered_map<string, KeyValueContainer>;
        using ObserverMap = std::list<std::pair<const string, struct Exchange::IDictionary::INotification*>>;

    public:
        class NameSpace : public Core::JSON::Container {
        public:
            class Entry : public Core::JSON::Container {
            public:
                Entry()
                    : Core::JSON::Container()
                    , Key()
                    , Value()
                    , Type(VOLATILE)
                {
                    Add(_T("key"), &Key);
                    Add(_T("value"), &Value);
                    Add(_T("type"), &Type);
                }
                Entry(const Entry& copy)
                    : Core::JSON::Container()
                    , Key(copy.Key)
                    , Value(copy.Value)
                    , Type(copy.Type)
                {
                    Add(_T("key"), &Key);
                    Add(_T("value"), &Value);
                    Add(_T("type"), &Type);
                }
                Entry(const string& key, const string& value, const enumType type)
                    : Core::JSON::Container()
                    , Key()
                    , Value()
                    , Type(VOLATILE)
                {
                    Add(_T("key"), &Key);
                    Add(_T("value"), &Value);
                    Add(_T("type"), &Type);

                    if (key.empty() == false) {
                        Key = key;
                    }

                    if (value.empty() == false) {
                        Value = value;
                    }

                    if (type != VOLATILE) {
                        Type = type;
                    }
                }
                ~Entry() override = default;

                Entry& operator=(const Entry& RHS)
                {
                    Key = RHS.Key;
                    Value = RHS.Value;
                    Type = RHS.Type;

                    return (*this);
                }

            public:
                Core::JSON::String Key;
                Core::JSON::String Value;
                Core::JSON::EnumType<enumType> Type;
            };

            NameSpace()
                : Core::JSON::Container()
                , Name()
                , Spaces()
                , Dictionary()
            {
                Add(_T("name"), &Name);
                Add(_T("spaces"), &Spaces);
                Add(_T("dictionary"), &Dictionary);
            }
            NameSpace(const string& nameSpace)
                : Core::JSON::Container()
                , Name(nameSpace)
                , Spaces()
                , Dictionary()
            {
                Add(_T("name"), &Name);
                Add(_T("spaces"), &Spaces);
                Add(_T("dictionary"), &Dictionary);
            }
            NameSpace(const NameSpace& copy)
                : Core::JSON::Container()
                , Name(copy.Name)
                , Spaces()
                , Dictionary()
            {
                Add(_T("name"), &Name);
                Add(_T("spaces"), &Spaces);
                Add(_T("dictionary"), &Dictionary);
            }
            ~NameSpace() override = default;

        public:
            // Only the rootnamspace should be empty. Allnested namespace MUST have a namespace..
            Core::JSON::String Name;
            Core::JSON::ArrayType<NameSpace> Spaces;
            Core::JSON::ArrayType<Entry> Dictionary;

            // Create a namespace with the given name!
            NameSpace& operator[](const string& nameSpace)
            {
                NameSpace* current = this;

                if (nameSpace.empty() == false) {
                    Core::TextSegmentIterator iterator(Core::TextFragment(nameSpace), false, Exchange::IDictionary::namespaceDelimiter);

                    while (iterator.Next() == true) {
                        if (iterator.Current().Length() == 0) {
                            ASSERT(Name.Value().empty() == true);
                        } else {
                            // Find this namespace in Namespaces
                            Core::JSON::ArrayType<NameSpace>::Iterator newSpace(current->Spaces.Elements());

                            while ((newSpace.Next() == true) && (iterator.Current() != newSpace.Current().Name.Value())) {
                                // Intentinally left empty
                            }

                            if (newSpace.IsValid() == true) {
                                current = &(newSpace.Current());
                            } else {
                                const string nameSpace(iterator.Current().Text());

                                // Create the new space..
                                current = &(current->Spaces.Add(NameSpace(nameSpace)));
                                current->Name = nameSpace;
                            }
                        }
                    }
                }

                return (*current);
            }
        };
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Storage(_T("dictionary.json"))
                , LingerTime(10)
            { // Time in minutes.
                Add(_T("storage"), &Storage);
                Add(_T("lingertime"), &LingerTime);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Storage;
            Core::JSON::DecUInt16 LingerTime;
        };

    public:
        Dictionary()
            : _adminLock()
            , _config()
            , _dictionary()
        {
        }
        ~Dictionary() override = default;

        BEGIN_INTERFACE_MAP(Dictionary)
        INTERFACE_ENTRY(IPlugin)
        INTERFACE_ENTRY(Exchange::IDictionary)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        //  IPlugin methods
        // -------------------------------------------------------------------------------------------------------

        // First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
        // information and services for this particular plugin. The Service object contains configuration information that
        // can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
        // If there is an error, return a string describing the issue why the initialisation failed.
        // The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
        // The lifetime of the Service object is guaranteed till the deinitialize method is called.
        const string Initialize(PluginHost::IShell* service) override;

        // The plugin is unloaded from Thunder. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        void Deinitialize(PluginHost::IShell* service) override;

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        string Information() const override;

        //  IDictionary methods
        // -------------------------------------------------------------------------------------------------------
        // Direct method to Get a value from a key in a certain namespace from the dictionary.
        // key MUST be filled.
        Core::hresult Get(const string& path, const string& key, string& value /* @out */) const override;

        // Direct Method to get the entries for a certain namespace
        Core::hresult PathEntries(const string& path, IDictionary::IPathIterator*& entries /* @out */) const override;

        // Direct method to Set a value for a key in a certain namespace from the dictionary.
        // key MUST be filled.
        Core::hresult Set(const string& path, const string& key, const string& value) override;

        Core::hresult Register(const string& path, Exchange::IDictionary::INotification* sink) override;
        Core::hresult Unregister(const string& path, const Exchange::IDictionary::INotification* sink) override;

    private:
        bool CreateInternalDictionary(const string& currentSpace, const NameSpace& data);
        void CreateExternalDictionary(const string& currentSpace, NameSpace& data) const;
        inline void NotifyForUpdate(const string& path, const string& key, const string& value) const;

        const string& Delimiter() const {
            static string delimiter{ Exchange::IDictionary::namespaceDelimiter };
            return delimiter;
        }

    private:
        mutable Core::CriticalSection _adminLock;
        Config _config;
        DictionaryMap _dictionary;
        ObserverMap _observers;
    };
}
}

#endif // __DICTIONARY_H
