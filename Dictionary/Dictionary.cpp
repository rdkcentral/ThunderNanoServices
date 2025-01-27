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
 
#include "Dictionary.h"

namespace Thunder {

ENUM_CONVERSION_BEGIN(Plugin::Dictionary::enumType)

    { Plugin::Dictionary::enumType::VOLATILE, _TXT("volatile") },
    { Plugin::Dictionary::enumType::PERSISTENT, _TXT("persistent") },

ENUM_CONVERSION_END(Plugin::Dictionary::enumType)

namespace Plugin {

    namespace {

        static Metadata<Dictionary> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

    static bool IsValidName(const string& value)
    {
        return ((value.empty() == false) && (value.find_first_of(Exchange::IDictionary::namespaceDelimiter, 0) == static_cast<size_t>(~0)));
    }

    bool Dictionary::CreateInternalDictionary(const string& currentSpace, const NameSpace& current)
    {
        bool correctStructure(true);
        Core::JSON::ArrayType<NameSpace::Entry>::ConstIterator keyIndex(current.Dictionary.Elements());
        Core::JSON::ArrayType<NameSpace>::ConstIterator spaceIndex(current.Spaces.Elements());
        std::list<RuntimeEntry>* currentList = NULL;

        // Fill in the keys from this name space...
        while ((correctStructure == true) && (keyIndex.Next() == true)) {
            const string& key(keyIndex.Current().Key.Value());

            correctStructure = IsValidName(key);

            if (correctStructure == true) {
                if (currentList == NULL) {
                    currentList = &(_dictionary[currentSpace]);

                    ASSERT(currentList != NULL);
                }

                currentList->push_back(RuntimeEntry(key, keyIndex.Current().Value.Value(), keyIndex.Current().Type.Value()));
            }
        }

        while ((correctStructure == true) && (spaceIndex.Next() == true)) {
            string nameSpace(spaceIndex.Current().Name.Value());
            correctStructure = IsValidName(nameSpace);
            correctStructure = correctStructure && CreateInternalDictionary(currentSpace + nameSpace, spaceIndex.Current());
        }

        return (correctStructure);
    }

    void Dictionary::CreateExternalDictionary(const string& currentSpace, NameSpace& current) const
    {
        Core::TextFragment requiredSpace(currentSpace);
        DictionaryMap::const_iterator index(_dictionary.begin());

        while (index != _dictionary.end()) {
            // Vallidate if the given path does include this namespace..
            if ((currentSpace.empty() == true) || (requiredSpace.EqualText(index->first.c_str(), 0, requiredSpace.Length(), true) == true)) {
                // Seems like we need to report this space, build it up
                NameSpace& blockToFill(current[index->first]);

                // No we got the namespace bloc, fill in the keys..
                const std::list<RuntimeEntry>& keyList(index->second);
                std::list<RuntimeEntry>::const_iterator keyIndex(keyList.begin());

                while (keyIndex != keyList.end()) {
                    NameSpace::Entry& entry(blockToFill.Dictionary.Add(NameSpace::Entry()));
                    entry.Key = keyIndex->Key();
                    entry.Value = keyIndex->Value();

                    if (keyIndex->Type() != entry.Type.Default()) {
                        entry.Type = keyIndex->Type();
                    }

                    keyIndex++;
                }
            }
            index++;
        }
    }

    /* virtual */ const string Dictionary::Initialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED )
    {
        /*
        _config.FromString(service->ConfigLine());

        Core::File dictionaryFile(service->PersistentPath() + _config.Storage.Value());

        if (dictionaryFile.Open(true) == true) {
            NameSpace dictionary;
            Core::OptionalType<Core::JSON::Error> error;
            dictionary.IElement::FromFile(dictionaryFile, error);
            if (error.IsSet() == true) {
                SYSLOG(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
            }
            CreateInternalDictionary(Delimiter(), dictionary);
        }
        */
        Exchange::JDictionary::Register(*this, this);

        // On succes return a name as a Callsign to be used in the URL, after the "service"prefix
        return (_T(""));
    }

    /* virtual */ void Dictionary::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED )
    {

        Exchange::JDictionary::Unregister(*this);
        /*
        Core::File dictionaryFile(service->PersistentPath() + _config.Storage.Value());

        if (dictionaryFile.Create() == true) {
            NameSpace dictionary;
            CreateExternalDictionary(EMPTY_STRING, dictionary);
            if (dictionary.IElement::ToFile(dictionaryFile) == false) {
                SYSLOG(Logging::Shutdown, (_T("Error occured while trying to save dictionary data to file!")));
            }
        }
        */
    }

    /* virtual */ string Dictionary::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ Core::hresult Dictionary::Get(const string& path, const string& key, string& value) const
    {
        ASSERT(key.empty() == false);
        ASSERT((path.size() <= 1 ) || (path.back() != Exchange::IDictionary::namespaceDelimiter ));

        Core::hresult result = Core::ERROR_UNKNOWN_KEY;

        _adminLock.Lock();

        DictionaryMap::const_iterator index = _dictionary.find((path.empty() == true ? Delimiter() : path));

        if (index != _dictionary.end()) {
            const KeyValueContainer& container(index->second);
            KeyValueContainer::const_iterator listIndex(container.begin());

            while ((listIndex != container.end()) && (listIndex->Key() != key)) {
                listIndex++;
            }

            if (listIndex != container.end()) {
                result = Core::ERROR_NONE;
                value = listIndex->Value();
            }
        }

        _adminLock.Unlock();

        return (result);
    }

    /* virtual */ Core::hresult Dictionary::PathEntries(const string& path, IDictionary::IPathIterator*& entries /* @out */) const
    {

        ASSERT((path.size() <= 1) || (path.back() != Exchange::IDictionary::namespaceDelimiter));

        std::list<Exchange::IDictionary::PathEntry> pathentries;

        if ((path.empty() == true) || ((path.size() == 1) && (path[0] == Exchange::IDictionary::namespaceDelimiter))) {
        
            _adminLock.Lock();

            DictionaryMap::const_iterator namespaces = _dictionary.cbegin();

            while (namespaces != _dictionary.cend()) {
                ASSERT(namespaces->first.size() >= 1);
                if (namespaces->first == Delimiter()) {
                    const KeyValueContainer& container(namespaces->second);
                    KeyValueContainer::const_iterator listIndex(container.begin());

                    while ((listIndex != container.end())) {
                        pathentries.emplace_back(Exchange::IDictionary::PathEntry{ listIndex->Key(), (listIndex->Type() == enumType::VOLATILE ? Exchange::IDictionary::Type::VOLATILE_KEY : Exchange::IDictionary::Type::PERSISTENT_KEY) });
                        listIndex++;
                    }
                } else {
                    string::size_type endpos = namespaces->first.find(Exchange::IDictionary::namespaceDelimiter, 0);
                    const string& name = (endpos == string::npos ? namespaces->first : namespaces->first.substr(0, endpos));
                    if (std::find_if(pathentries.cbegin(), pathentries.cend(), [&name](const Exchange::IDictionary::PathEntry& x) { return x.name == name; }) == pathentries.cend()) {
                        pathentries.emplace_back(Exchange::IDictionary::PathEntry{ name, Exchange::IDictionary::Type::NAMESPACE });
                    }
                }
                ++namespaces;
            };

            _adminLock.Unlock();

        } else {

            _adminLock.Lock();

            DictionaryMap::const_iterator namespaces = _dictionary.cbegin();

            while (namespaces != _dictionary.cend()) {

                string namespacepart = namespaces->first.substr(0, path.size());  // please note std::string start_with is only c++ 20 and upwards...

                if (namespacepart == path) {
                    if (path.size() == namespaces->first.size()) {
                        // if the whole name matches we must get all the keys for this namespace path
                        const KeyValueContainer& container(namespaces->second);
                        KeyValueContainer::const_iterator listIndex(container.begin());

                        while ((listIndex != container.end())) {
                            pathentries.push_back({ listIndex->Key(), (listIndex->Type() == enumType::VOLATILE ? Exchange::IDictionary::Type::VOLATILE_KEY : Exchange::IDictionary::Type::PERSISTENT_KEY) });
                            listIndex++;
                        };
                        // no break here to stop looping through the namespaces as next to the full namespace that has keys (what we found now) lets also support that there could be subnamespces as well at the same time (like for a disk folder structure 
                        // were you could have files in a folder and subfolders at the same time) so we keep looping to also find these
                    } else if ((namespaces->first.size() > path.size()) && (namespaces->first[path.size()] == Exchange::IDictionary::namespaceDelimiter)) {
                        // if the namespace path found starts with what we are looking for and has a delimiter next (otherwise by accident the 
                        // first part of the namespace is equal but different characters follow so it is not the one we are looking for) we must add the next namespace part to the result 
                        // (and continue looking for more)
                        ASSERT((namespaces->first.size() > path.size() + 1) && (namespaces->first[path.size() + 1] != Exchange::IDictionary::namespaceDelimiter)); // there must be at least one more character (as we do not allow the namespace stored to end with a delimiter) andn that character 

                        //see if we find another delimiter (could of course be more than one nested namespace after the one we are looking for
                        string::size_type endpos = namespaces->first.find(Exchange::IDictionary::namespaceDelimiter, path.size() + 1);
                        string subnamespace;
                        if (endpos == string::npos) {
                            // no delimiter rest of string is the last nested namespace
                            subnamespace = namespaces->first.substr(path.size() + 1); //note this means get all chars starting from this position
                        } else {
                            subnamespace = namespaces->first.substr(path.size() + 1, endpos - path.size() - 1);
                        }
                        if (std::find_if(pathentries.cbegin(), pathentries.cend(), [&subnamespace](const Exchange::IDictionary::PathEntry& x) { return x.name == subnamespace; }) == pathentries.cend()) {
                            pathentries.emplace_back(Exchange::IDictionary::PathEntry{ subnamespace, Exchange::IDictionary::Type::NAMESPACE });
                        }
                    }
                }

                ++namespaces;
            };

            _adminLock.Unlock();
        
        }

        using Implementation = RPC::IteratorType<Exchange::IDictionary::IPathIterator>;
        entries = Core::ServiceType<Implementation>::Create<Exchange::IDictionary::IPathIterator>(pathentries);

        return (Core::ERROR_NONE);
    }


    void Dictionary::NotifyForUpdate(const string& path, const string& key, const string& value) const
    {
        ObserverMap::const_iterator index(_observers.cbegin());

        while (index != _observers.cend()) {
            if (index->first == path) {
                index->second->Modified(path, key, value);
            }
            index++;
        }

        Exchange::JDictionary::Event::Modified(*this, path, key, value);
    }

    // Direct method to Set a value for a key in a certain namespace from the dictionary.
    // path and key MUST be filled.
    /* virtual */ Core::hresult Dictionary::Set(const string& path, const string& key, const string& value)
    {
        ASSERT((path.size() <= 1) || (path.back() != Exchange::IDictionary::namespaceDelimiter));

        // Direct method to Set a value for a key in a certain namespace from the dictionary.
        ASSERT(key.empty() == false);

        _adminLock.Lock();

        KeyValueContainer& container(_dictionary[(path.empty() == true ? Delimiter() : path)]);
        KeyValueContainer::iterator listIndex(container.begin());

        while ((listIndex != container.end()) && (listIndex->Key() != key)) {
            listIndex++;
        }

        if (listIndex == container.end()) {
            container.push_back(RuntimeEntry(key, value, VOLATILE));
            NotifyForUpdate(path, key, value);
        } else if (listIndex->Value() != value) {
            listIndex->Value(value);
            NotifyForUpdate(path, key, value);
        }

        _adminLock.Unlock();

        return (Core::ERROR_NONE);
    }

    /* virtual */ Core::hresult Dictionary::Register(const string& path, Exchange::IDictionary::INotification* sink)
    {
        ASSERT(sink != nullptr);

        ASSERT((path.size() <= 1) || (path.back() != Exchange::IDictionary::namespaceDelimiter));


        _adminLock.Lock();

#ifdef __DEBUG__
        ObserverMap::iterator index(_observers.begin());

        // DO NOT REGISTER THE SAME NOTIFICATION SINK ON THE SAME NAMESPACE MORE THAN ONCE. !!!!!!
        while (index != _observers.end()) {
            ASSERT((index->second != sink) || (path != index->first));

            index++;
        }
#endif

        sink->AddRef();
        _observers.push_back(std::pair<string, struct Exchange::IDictionary::INotification*>(path, sink));

        _adminLock.Unlock();

        return (Core::ERROR_NONE);
    }

    /* virtual */ Core::hresult Dictionary::Unregister(const string& path, const Exchange::IDictionary::INotification* sink)
    {
        ASSERT(sink != nullptr);

        ASSERT((path.size() <= 1) || (path.back() != Exchange::IDictionary::namespaceDelimiter));

        bool found = false;

        _adminLock.Lock();

        ObserverMap::iterator index(_observers.begin());

        while ((found == false) && (index != _observers.end())) {
            found = ((index->second == sink) && (path == index->first));
            if (found == false) {
                index++;
            }
        }

        if (index != _observers.end()) {
            index->second->Release();
            _observers.erase(index);
        }

        _adminLock.Unlock();

        return (Core::ERROR_NONE);
    }
}
}
