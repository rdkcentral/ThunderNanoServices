#pragma once

#include "Module.h"

#include <regex>

namespace WPEFramework {
namespace Plugin {

    //Allow -> Check first
    //			if Block then check for Block[] and block if present
    //			 else must be explicitly allowed

    class EXTERNAL AccessControlList {
    private:
        class EXTERNAL JSONACL : public Core::JSON::Container {
        public:
            class Config : public Core::JSON::Container {
            public:
                Config(const Config&) = delete;
                Config& operator=(const Config&) = delete;

                Config()
                {
                    Add(_T("allow"), &Allow);
                    Add(_T("block"), &Block);
                }
                virtual ~Config()
                {
                }

            public:
                Core::JSON::ArrayType<Core::JSON::String> Allow;
                Core::JSON::ArrayType<Core::JSON::String> Block;
            };
            class Role : public Core::JSON::Container {
            public:
                Role(const Role&) = delete;
                Role& operator=(const Role&) = delete;

                Role()
                    : Configuration()
                {
                    Add(_T("thunder"), &Configuration);
                }
                virtual ~Role()
                {
                }

            public:
                Config Configuration;
            };
            class Roles : public Core::JSON::Container {
            private:
                using RolesMap = std::map<string, Role>;

            public:
                using Iterator = Core::IteratorMapType<const RolesMap, const Role&, const string&, RolesMap::const_iterator>;

                Roles(const Roles&) = delete;
                Roles& operator=(const Roles&) = delete;

                Roles()
                    : _roles()
                {
                }
                virtual ~Roles()
                {
                }

                inline Iterator Elements() const
                {
                    return (Iterator(_roles));
                }

            private:
                virtual bool Request(const TCHAR label[])
                {
                    if (_roles.find(label) == _roles.end()) {
                        auto element = _roles.emplace(std::piecewise_construct,
                            std::forward_as_tuple(label),
                            std::forward_as_tuple());
                        Add(element.first->first.c_str(), &(element.first->second));
                    }
                    return (true);
                }

            private:
                RolesMap _roles;
            };
            class Group : public Core::JSON::Container {
            public:
                Group()
                    : URL()
                    , Role()
                {
                    Add(_T("url"), &URL);
                    Add(_T("role"), &Role);
                }
                Group(const Group& copy)
                    : URL()
                    , Role()
                {
                    Add(_T("url"), &URL);
                    Add(_T("role"), &Role);

                    URL = copy.URL;
                    Role = copy.Role;
                }
                virtual ~Group()
                {
                }

                Group& operator=(const Group& RHS)
                {
                    URL = RHS.URL;
                    Role = RHS.Role;

                    return (*this);
                }

            public:
                Core::JSON::String URL;
                Core::JSON::String Role;
            };

        public:
            JSONACL(const JSONACL&) = delete;
            JSONACL& operator=(const JSONACL&) = delete;
            JSONACL()
            {
                Add(_T("assign"), &Groups);
                Add(_T("roles"), &ACL);
            }
            virtual ~JSONACL()
            {
            }

        public:
            Core::JSON::ArrayType<Group> Groups;
            Roles ACL;
        };

    public:
        class Filter {
        public:
            Filter() = delete;
            Filter(const Filter&) = delete;
            Filter& operator=(const Filter&) = delete;

            Filter(const JSONACL::Config& filter)
            {
                Core::JSON::ArrayType<Core::JSON::String>::ConstIterator index(filter.Allow.Elements());
                while (index.Next() == true) {
                    _allow.emplace_back(index.Current().Value());
                }
                index = (filter.Block.Elements());
                while (index.Next() == true) {
                    _block.emplace_back(index.Current().Value());
                }
            }
            ~Filter()
            {
            }

        public:
            bool Allowed(const string& method) const
            {
                bool allowed = false;
                if (_allowSet) {
                    std::list<string>::const_iterator index(_allow.begin());
                    while ((index != _allow.end()) && (allowed == false)) {
                        allowed = strncmp(index->c_str(), method.c_str(), index->length()) == 0;
                        index++;
                    }
                } else {
                    allowed = true;
                    std::list<string>::const_iterator index(_block.begin());
                    while ((index != _block.end()) && (allowed == true)) {
                        allowed = strncmp(index->c_str(), method.c_str(), index->length()) != 0;
                        index++;
                    }
                }
                return (allowed);
            }

        private:
            bool _allowSet;
            std::list<string> _allow;
            std::list<string> _block;
        };

        using URLList = std::list<std::pair<string, Filter&>>;
        using Iterator = Core::IteratorType<const std::list<string>, const string&, std::list<string>::const_iterator>;

    public:
        AccessControlList(const AccessControlList&) = delete;
        AccessControlList& operator=(const AccessControlList&) = delete;

        AccessControlList()
            : _urlMap()
            , _filterMap()
            , _unusedRoles()
            , _undefinedURLS()
        {
        }
        ~AccessControlList()
        {
        }

    public:
        inline Iterator Unreferenced() const
        {
            return (Iterator(_unusedRoles));
        }
        inline Iterator Undefined() const
        {
            return (Iterator(_undefinedURLS));
        }
        void Clear()
        {
            _urlMap.clear();
            _filterMap.clear();
            _unusedRoles.clear();
            _undefinedURLS.clear();
        }
        const Filter* FilterMapFromURL(const string& URL) const
        {
            const Filter* result = nullptr;
            std::smatch matchList;
            URLList::const_iterator index = _urlMap.begin();

            while ((index != _urlMap.end()) && (result == nullptr)) {
                // regex_search() for searching the regex pattern
                // 'r' in the string 's'. 'm' is flag for determining
                // matching behavior.
                std::regex expression(index->first.c_str());

                if (std::regex_search(URL, matchList, expression) == true) {
                    result = &(index->second);
                }
                index++;
            }

            return (result);
        }
        uint32_t Load(Core::File& source)
        {
            JSONACL controlList;
            Core::OptionalType<Core::JSON::Error> error;
            controlList.IElement::FromFile(source, error);
            if (error.IsSet() == true) {
                SYSLOG(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
            }
            _unusedRoles.clear();

            JSONACL::Roles::Iterator rolesIndex = controlList.ACL.Elements();

            // Now iterate over the Rules
            while (rolesIndex.Next() == true) {
                const string& roleName = rolesIndex.Key();

                _unusedRoles.push_back(roleName);

                _filterMap.emplace(std::piecewise_construct,
                    std::forward_as_tuple(roleName),
                    std::forward_as_tuple(rolesIndex.Current().Configuration));
            }

            Core::JSON::ArrayType<JSONACL::Group>::Iterator index = controlList.Groups.Elements();

            // Let iterate over the groups
            while (index.Next() == true) {
                const string& role(index.Current().Role.Value());

                // Try to find the Role..
                std::map<string, Filter>::iterator selectedFilter(_filterMap.find(role));

                if (selectedFilter == _filterMap.end()) {
                    std::list<string>::iterator found = std::find(_undefinedURLS.begin(), _undefinedURLS.end(), role);
                    if (found == _undefinedURLS.end()) {
                        _undefinedURLS.push_front(role);
                    }
                } else {
                    Filter& entry(selectedFilter->second);

                    _urlMap.emplace_back(std::pair<string, Filter&>(
                        index.Current().URL.Value(), entry));

                    std::list<string>::iterator found = std::find(_unusedRoles.begin(), _unusedRoles.end(), role);

                    if (found != _unusedRoles.end()) {
                        _unusedRoles.erase(found);
                    }
                }
            }
            return ((_unusedRoles.empty() && _undefinedURLS.empty()) ? Core::ERROR_NONE : Core::ERROR_INCOMPLETE_CONFIG);
        }

    private:
        URLList _urlMap;
        std::map<string, Filter> _filterMap;
        std::list<string> _unusedRoles;
        std::list<string> _undefinedURLS;
    };
}
}
