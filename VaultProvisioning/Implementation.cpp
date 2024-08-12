/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Metrological
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

#include "Module.h"

#include <interfaces/IConfiguration.h>
#include <cryptography/cryptography.h>


namespace Thunder {

namespace Plugin {

    class VaultProvisioningImplementation : public Exchange::IConfiguration {
    public:
        enum provisioningtype : uint8_t {
            RANDOM
        };

    private:
        class Config : public Core::JSON::Container {
        public:
            class Object : public Core::JSON::Container {
            public:
                Object()
                    : Core::JSON::Container()
                    , Type()
                    , Vault()
                    , Label()
                    , Info()
                {
                    Init();
                }
                ~Object() = default;

                Object(const Object& copy)
                    : Core::JSON::Container()
                    , Type()
                    , Vault()
                    , Label()
                    , Info()
                {
                    Type = copy.Type;
                    Vault = copy.Vault;
                    Label = copy.Label;
                    Info = copy.Info;

                    Init();
                }

                Object& operator=(const Object& rhs)
                {
                    Type = rhs.Type;
                    Vault = rhs.Vault;
                    Label = rhs.Label;
                    Info = rhs.Info;

                    return (*this);
                }

            public:
                Core::JSON::EnumType<provisioningtype> Type;
                Core::JSON::String Vault;
                Core::JSON::String Label;
                Core::JSON::String Info;

            private:
                void Init()
                {
                    Add(_T("type"), &Type);
                    Add(_T("vault"), &Vault);
                    Add(_T("label"), &Label);
                    Add(_T("info"), &Info);
                }
            }; // class Object

        public:
            Config()
                : Core::JSON::Container()
                , Location(_T("objects"))
            {
                Add(_T("location"), &Location);
                Add(_T("objects"), &Objects);
            }
            ~Config() = default;

            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Core::JSON::String Location;
            Core::JSON::ArrayType<Object> Objects;
        }; // class Config

        class ObjectFile : public Core::JSON::Container {
        public:
            ObjectFile(const ObjectFile&) = delete;
            ObjectFile& operator=(const ObjectFile&) = delete;
            ObjectFile()
                : Core::JSON::Container()
                , Vault()
                , Label()
                , Data()
            {
                Add(_T("vault"), &Vault);
                Add(_T("label"), &Label);
                Add(_T("data"), &Data);

            }
            ~ObjectFile() = default;

        public:
            Core::JSON::String Vault;
            Core::JSON::String Label;
            Core::JSON::String Data;
        };

    public:
        VaultProvisioningImplementation()
            : _lock()
            , _persistentPath()
        {
        }
        ~VaultProvisioningImplementation() override = default;

        VaultProvisioningImplementation(const VaultProvisioningImplementation&) = delete;
        VaultProvisioningImplementation& operator= (const VaultProvisioningImplementation&) = delete;

    public:
        // Exchange::IConfiguration interface
        uint32_t Configure(PluginHost::IShell* service) override
        {
            uint32_t result = Core::ERROR_NONE;

            Config config;
            config.FromString(service->ConfigLine());

            _lock.Lock();

            _persistentPath = service->PersistentPath();
            ASSERT(_persistentPath.empty() == false);

            if (config.Location.Value().empty() == false) {
                _persistentPath += config.Location.Value() + _T("/");

                bool valid = false;
                Core::File::Normalize(_persistentPath, valid);
                ASSERT(valid == true);
            }

            Core::Directory directory(_persistentPath.c_str());

            if (directory.CreatePath() != true) {
                TRACE(Trace::Error, (_T("Failed to create objects directory!")));
            }

            TRACE(Trace::Information, (_T("VaultProvisioning object location: '%s'"), _persistentPath.c_str()));

            Exchange::ICryptography* crypto = Exchange::ICryptography::Instance("");
            ASSERT(crypto != nullptr);

            if (crypto != nullptr) {
                Provision(crypto, config);
                crypto->Release();
            }

            _lock.Unlock();

            return (result);
        }

    public:
        BEGIN_INTERFACE_MAP(VaultProvisioningImplementation)
            INTERFACE_ENTRY(Exchange::IConfiguration)
        END_INTERFACE_MAP

    private:
        void Provision(Exchange::ICryptography* crypto, Config& config) const
        {
            ASSERT(crypto != nullptr);

            auto it = config.Objects.Elements();

            while (it.Next() == true) {

                uint32_t result = Core::ERROR_GENERAL;

                const Config::Object& obj = it.Current();
                const string& vaultLabel = obj.Vault.Value();
                const string& label = obj.Label.Value();
                const string& info = obj.Info.Value();

                const Exchange::CryptographyVault vaultId = Cryptography::VaultId(vaultLabel);

                Exchange::IVault* const vault = crypto->Vault(vaultId);

                if (vault != nullptr) {
                    switch (obj.Type.Value()) {
                    case RANDOM:
                        result = GenerateRandomKey(vault, vaultLabel, info, label);
                        break;
                    default:
                        TRACE(Trace::Error, (_T("Invalid object type [%d]"), obj.Type.Value()));
                        break;
                    }

                    vault->Release();
                }

                if (result == Core::ERROR_NONE) {
                    TRACE(Trace::Information, (_T("Provisioned '%s' object(s) in vault '%s'"), label.c_str(), vaultLabel.c_str()));
                }
                else if (result != Core::ERROR_ALREADY_CONNECTED) {
                    TRACE(Trace::Error, (_T("Failed to provision '%s' object(s) in vault '%s'"), label.c_str(), vaultLabel.c_str()));
                }
                else {
                    TRACE(Trace::Text, (_T("'%s' object(s) already provisioned in vault '%s'"), label.c_str(), vaultLabel.c_str()));
                }
            }
        }

    private:
        uint32_t GenerateRandomKey(Exchange::IVault* const vault, const string& vaultLabel, const string& info, const string& label) const
        {
            uint32_t result = Core::ERROR_GENERAL;

            class Info : public Core::JSON::Container {
            public:
                Info()
                    : Core::JSON::Container()
                    , Size()
                {
                    Add(_T("size"), &Size);
                }
                ~Info() = default;
                Info(const Info&) = delete;
                Info& operator=(const Info&) = delete;

            public:
                Core::JSON::DecUInt32 Size;
            };

            ASSERT(vault != nullptr);

            if (Check(label) == true) {
                result = Core::ERROR_ALREADY_CONNECTED;
            }
            else {
                Info context;
                context.FromString(info);

                if (context.Size.IsSet() == true) {
                    const uint32_t& size = context.Size.Value();

                    TRACE(Trace::Text, (_T("Generating random key of size %d bytes"), size));

                    if ((size != 0) && (size <= 0xFFFF-64)) {
                        const uint32_t id = vault->Generate(size);

                        if (id != 0) {
                            result = Extract(vault, vaultLabel, label, id);
                        }
                    }
                }
            }

            return (result);
        }

    private:
        string FileName(const string& label) const
        {
            return (_persistentPath + label + _T(".json"));
        }
        bool Check(const string& label) const
        {
            return (Core::File(FileName(label)).Exists());
        }
        uint32_t Extract(Exchange::IVault* const vault, const string& vaultLabel, const string& label, const uint32_t id) const
        {
            uint32_t result = Core::ERROR_GENERAL;

            ASSERT(vault != nullptr);
            ASSERT(vaultLabel.empty() == false);
            ASSERT(label.empty() == false);
            ASSERT(id != 0);

            Core::File file(FileName(label));

            ASSERT(file.Exists() == false);

            if (file.Create() == true) {
                uint16_t size = USHRT_MAX;
                uint8_t* blob = static_cast<uint8_t*>(ALLOCA(size));

                size = vault->Get(id, size, blob);
                ASSERT(size != 0);

                if (size != 0) {
                    string base64;
                    Core::ToString(blob, size, true, base64);
                    ASSERT(base64.empty() == false);

                    ObjectFile obj;
                    obj.Vault = vaultLabel;
                    obj.Label = label;
                    obj.Data = base64;

                    obj.IElement::ToFile(file);
                    file.Close();

                    result = Core::ERROR_NONE;

                    TRACE(Trace::Text, (_T("Created file '%s'"), file.Name().c_str()));
                }
            }
            else {
                TRACE(Trace::Error, (_T("Failed to create file '%s'"), file.Name().c_str()));
            }

            return (result);
        }

    private:
        Core::CriticalSection _lock;
        string _persistentPath;
    };

    SERVICE_REGISTRATION(VaultProvisioningImplementation, 1, 0)

} // namespace Plugin

ENUM_CONVERSION_BEGIN(Plugin::VaultProvisioningImplementation::provisioningtype)
    { Plugin::VaultProvisioningImplementation::provisioningtype::RANDOM, _TXT("random") },
ENUM_CONVERSION_END(Plugin::VaultProvisioningImplementation::provisioningtype)

}
