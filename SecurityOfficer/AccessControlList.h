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
            class Filter : public Core::JSON::Container {
            public:
                Filter(const Filter&) = delete;
                Filter& operator=(const Filter&) = delete;

                Filter()
                {
                    Add(_T("allow"), &Allow);
                    Add(_T("block"), &Block);
                }
                virtual ~Filter()
                {
                }

            public:
                Core::JSON::ArrayType<Core::JSON::String> Allow;
                Core::JSON::ArrayType<Core::JSON::String> Block;
            };
            class Role : public Core::JSON::Container {
            private:
                using RoleMap = std::map<string, Filter>;

            public:
                using Iterator = Core::IteratorMapType<const RoleMap, const Filter&, const string&, RoleMap::const_iterator>;

                Role(const Role&) = delete;
                Role& operator=(const Role&) = delete;

                Role()
                    : _filters()
                {
                }
                virtual ~Role()
                {
                }

                inline Iterator Elements() const
                {
                    return (Iterator(_filters));
                }

            private:
                virtual bool Request(const TCHAR label[])
                {
                    if (_filters.find(label) == _filters.end()) {
                        auto element = _filters.emplace(std::piecewise_construct,
                            std::forward_as_tuple(label),
                            std::forward_as_tuple());
                        Add(element.first->first.c_str(), &(element.first->second));
                    }
                    return (true);
                }

            private:
                RoleMap _filters;
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
                Add(_T("groups"), &Groups);
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

            Filter(const JSONACL::Filter& filter)
                : _allowSet(true)
            {
                Core::JSON::ArrayType<Core::JSON::String>::ConstIterator index(filter.Allow.Elements());
                while (index.Next() == true) {
                    _allow.emplace_back(index.Current().Value());
                }
                index = (filter.Block.Elements());
                while (index.Next() == true) {
                    _block.emplace_back(index.Current().Value());
                }
                _allowSet = !filter.Block.IsSet();
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

        using FilterMap = std::map<string, Filter>;
        using URLList = std::list<std::pair<string, FilterMap&>>;
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
        const FilterMap* FilterMapFromURL(const string& URL) const
        {
            const FilterMap* result = nullptr;
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
            controlList.FromFile(source);
            _unusedRoles.clear();

            JSONACL::Roles::Iterator rolesIndex = controlList.ACL.Elements();

            // Now iterate over the Rules
            while (rolesIndex.Next() == true) {
                const string& roleName = rolesIndex.Key();
                const JSONACL::Role& contents = rolesIndex.Current();

                JSONACL::Role::Iterator filterIndex = contents.Elements();
                FilterMap& entry = _filterMap.emplace(std::piecewise_construct,
                                                 std::forward_as_tuple(roleName),
                                                 std::forward_as_tuple())
                                       .first->second;

                _unusedRoles.push_back(roleName);

                while (filterIndex.Next() == true) {
                    entry.emplace(std::piecewise_construct,
                        std::forward_as_tuple(filterIndex.Key()),
                        std::forward_as_tuple(filterIndex.Current()));
                }
            }

            Core::JSON::ArrayType<JSONACL::Group>::Iterator index = controlList.Groups.Elements();

            // Let iterate over the groups
            while (index.Next() == true) {
                const string& role(index.Current().Role.Value());

                // Try to find the Role..
                std::map<string, FilterMap>::iterator selectedFilter(_filterMap.find(role));

                if (selectedFilter == _filterMap.end()) {
                    std::list<string>::iterator found = std::find(_undefinedURLS.begin(), _undefinedURLS.end(), role);
                    if (found == _undefinedURLS.end()) {
                        _undefinedURLS.push_front(role);
                    }
                } else {
                    FilterMap& entry(selectedFilter->second);

                    _urlMap.emplace_back(std::pair<string, FilterMap&>(
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
        std::map<string, FilterMap> _filterMap;
        std::list<string> _unusedRoles;
        std::list<string> _undefinedURLS;
    };
}
}