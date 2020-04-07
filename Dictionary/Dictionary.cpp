/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Plugin::Dictionary::enumType)

    { Plugin::Dictionary::enumType::VOLATILE, _TXT("volatile") },
    { Plugin::Dictionary::enumType::PERSISTENT, _TXT("persistent") },
    { Plugin::Dictionary::enumType::CLOSURE, _TXT("closure") },

    ENUM_CONVERSION_END(Plugin::Dictionary::enumType);

namespace Plugin {

    SERVICE_REGISTRATION(Dictionary, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<Dictionary::NameSpace>> jsonBodyDataFactory(4);
    static Core::ProxyPoolType<Web::TextBody> textBodyDataFactory(4);

    static bool IsValidName(const string& value)
    {
        return ((value.empty() == false) && (value.find_first_of(Dictionary::NameSpaceDelimiter, 0) == static_cast<size_t>(~0)));
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
            correctStructure = correctStructure && CreateInternalDictionary(currentSpace + NameSpaceDelimiter + nameSpace, spaceIndex.Current());
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

    /* virtual */ const string Dictionary::Initialize(PluginHost::IShell* service)
    {
        _config.FromString(service->ConfigLine());

        Core::File dictionaryFile(service->PersistentPath() + _config.Storage.Value());

        if (dictionaryFile.Open(true) == true) {
            NameSpace dictionary;
            Core::OptionalType<Core::JSON::Error> error;
            dictionary.IElement::FromFile(dictionaryFile, error);
            if (error.IsSet() == true) {
                SYSLOG(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
            }
            CreateInternalDictionary(EMPTY_STRING, dictionary);
        }

        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());

        // On succes return a name as a Callsign to be used in the URL, after the "service"prefix
        return (_T(""));
    }

    /* virtual */ void Dictionary::Deinitialize(PluginHost::IShell* service)
    {

        Core::File dictionaryFile(service->PersistentPath() + _config.Storage.Value());

        if (dictionaryFile.Open(true) == true) {
            NameSpace dictionary;
            CreateExternalDictionary(EMPTY_STRING, dictionary);
            dictionary.IElement::ToFile(dictionaryFile);
        }
    }

    /* virtual */ string Dictionary::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ void Dictionary::Inbound(Web::Request& request)
    {
        // There are no requests coming in with a body that require a JSON body. So continue without action here.
        request.Body(Core::ProxyType<Web::IBody>(textBodyDataFactory.Element()));
    }

    // <GET> ../[namespace/]{Key}
    // <PUT> ../[namespace/]{Key}?Type=[persistent|volatile|closure]
    /* virtual */ Core::ProxyType<Web::Response> Dictionary::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());
        // <GET> ../[namespace/]{Key}
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length()) - _skipURL), false, '/');
        // <PUT> ../[namespace/]{Key}?Type=[persistent|volatile|closure]
        // If there is an entry, the first one will always be a '/', skip this one..
        index.Next();

        // By default, we assume everything works..
        result->ErrorCode = Web::STATUS_OK;
        result->Message = "OK";

        string nameSpace;
        string key = index.Current().Text();

        while (index.Next() == true) {
            if (nameSpace.empty() == true) {
                nameSpace += NameSpaceDelimiter;
            }
            nameSpace += key;
            key = index.Current().Text();
        }

        if (request.Verb == Web::Request::HTTP_GET) {
            string value;
            Core::ProxyType<Web::TextBody> valueBody(textBodyDataFactory.Element());

            Get(nameSpace, key, value);
            *valueBody = value;

            result->Body(Core::proxy_cast<Web::IBody>(valueBody));
            result->ErrorCode = Web::STATUS_OK;
            result->Message = _T("OK");
        } else if ((request.Verb == Web::Request::HTTP_POST) && (key.empty() == false) && (request.HasBody() == true)) {
            Dictionary::enumType keyType(Dictionary::enumType::VOLATILE);
            Core::ProxyType<const Web::TextBody> valueBody(request.Body<Web::TextBody>());
            const string value(valueBody.IsValid() == true ? string(*valueBody) : string());
            Core::TextSegmentIterator typeIterator(Core::TextSegmentIterator(Core::TextFragment(request.Query), true, '='));

            if ((typeIterator.Next() == true) && (typeIterator.Current() == _T("Type")) && (typeIterator.Next() == true)) {
                // Seems we have a type specifier
                keyType = Core::EnumerateType<Dictionary::enumType>(typeIterator.Current(), false).Value();
            }

            TRACE(Trace::Information, (_T("SetKey ( %s, %s, %s)"), key.c_str(), value.c_str(), Core::EnumerateType<Dictionary::enumType>(keyType).Data()));
            Set(nameSpace, key, value);

            result->ErrorCode = Web::STATUS_OK;
            result->Message = _T("OK");
        } else {
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = _T("Bad request.");
        }

        return (result);
    }

    /* virtual */ bool Dictionary::Get(const string& nameSpace, const string& key, string& value) const
    {
        bool result = false;

        _adminLock.Lock();

        DictionaryMap::const_iterator index(_dictionary.find(nameSpace));

        if (index != _dictionary.end()) {
            const std::list<RuntimeEntry>& container(index->second);
            std::list<RuntimeEntry>::const_iterator listIndex(container.begin());

            while ((listIndex != container.end()) && (listIndex->Key() != key)) {
                listIndex++;
            }

            if (listIndex != container.end()) {
                result = true;
                value = listIndex->Value();
            }
        }

        _adminLock.Unlock();

        return (result);
    }

    /* virtual */ Exchange::IDictionary::IIterator* Dictionary::Get(const string& nameSpace) const
    {

        static Core::ProxyPoolType<Dictionary::Iterator> iterators(4);

        Exchange::IDictionary::IIterator* result = nullptr;

        _adminLock.Lock();

        DictionaryMap::const_iterator index(_dictionary.find(nameSpace));

        if (index != _dictionary.end()) {
            Core::ProxyType<Iterator> entries(iterators.Element());

            entries->Load(InternalIterator(index->second));

            result = &(*entries);
            result->AddRef();
        }

        _adminLock.Unlock();

        return (result);
    }

    // Direct method to Set a value for a key in a certain namespace from the dictionary.
    // NameSpace and key MUST be filled.
    /* virtual */ bool Dictionary::Set(const string& nameSpace, const string& key, const string& value)
    {
        // Direct method to Set a value for a key in a certain namespace from the dictionary.
        bool result = false;

        _adminLock.Lock();

        std::list<RuntimeEntry>& container(_dictionary[nameSpace]);
        std::list<RuntimeEntry>::iterator listIndex(container.begin());

        while ((listIndex != container.end()) && (listIndex->Key() != key)) {
            listIndex++;
        }

        if (listIndex == container.end()) {
            result = true;
            container.push_back(RuntimeEntry(key, value, VOLATILE));
        } else if (listIndex->Value() != value) {
            result = true;
            listIndex->Value(value);
        }

        if (result == true) {
            ObserverMap::iterator index(_observers.begin());

            // Right, we updated send out the modification !!!
            while (index != _observers.end()) {
                if (index->first == nameSpace) {
                    index->second->Modified(nameSpace, key, value);
                }
                index++;
            }
        }

        _adminLock.Unlock();

        return (result);
    }

    /* virtual */ void Dictionary::Register(const string& nameSpace, struct Exchange::IDictionary::INotification* sink)
    {
        _adminLock.Lock();

#ifdef __DEBUG__
        ObserverMap::iterator index(_observers.begin());

        // DO NOT REGISTER THE SAME NOTIFICATION SINK ON THE SAME NAMESPACE MORE THAN ONCE. !!!!!!
        while (index != _observers.end()) {
            ASSERT((index->second != sink) || (nameSpace != index->first));

            index++;
        }
#endif

        _observers.push_back(std::pair<string, struct Exchange::IDictionary::INotification*>(nameSpace, sink));

        _adminLock.Unlock();
    }

    /* virtual */ void Dictionary::Unregister(const string& nameSpace, struct Exchange::IDictionary::INotification* sink)
    {
        bool found = false;

        _adminLock.Lock();

        ObserverMap::iterator index(_observers.begin());

        while ((found == false) && (index != _observers.end())) {
            found = ((index->second == sink) && (nameSpace == index->first));
            if (found == false) {
                index++;
            }
        }

        if (index != _observers.end()) {
            _observers.erase(index);
        }

        _adminLock.Unlock();
    }
}
}
