#include "SecurityOfficer.h"
#include "SecurityContext.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(SecurityOfficer, 1, 0);

	class SecurityCallsign : public PluginHost::ISubSystem::ISecurity {
    public:
        SecurityCallsign() = delete;
        SecurityCallsign(const SecurityCallsign&) = delete;
        SecurityCallsign& operator= (const SecurityCallsign&) = delete;

		SecurityCallsign(const string callsign) : _callsign (callsign) {
		}
        virtual ~SecurityCallsign() {
        }

	public:
        // Security information
		virtual string Callsign() const {
            return (_callsign);
		}

	private:
        BEGIN_INTERFACE_MAP(SecurityCallsign)
        INTERFACE_ENTRY(PluginHost::ISubSystem::ISecurity)
        END_INTERFACE_MAP

    private:
        const string _callsign;
    };

    SecurityOfficer::SecurityOfficer()
    {
        for (uint8_t index = 0; index < sizeof(_secretKey); index++) {
            Crypto::Random(_secretKey[index]);
		}
    }

    /* virtual */ SecurityOfficer::~SecurityOfficer()
    {
    }

    /* virtual */ const string SecurityOfficer::Initialize(PluginHost::IShell* service)
    {
        Config config;
        config.FromString(service->ConfigLine());
        string version = service->Version();

        Core::File aclFile(service->PersistentPath() + config.ACL.Value(), true);

		if (aclFile.Exists() == false) {
            aclFile = service->DataPath() + config.ACL.Value();
        }
        if ( (aclFile.Exists() == true) && (aclFile.Open(true) ==true) ) {

			if (_acl.Load(aclFile) == Core::ERROR_INCOMPLETE_CONFIG) {
                AccessControlList::Iterator index(_acl.Unreferenced());
                while (index.Next()) {
                    SYSLOG(Logging::Startup, (_T("Role: %s not referenced"), index.Current().c_str()));
                }
                index = _acl.Undefined();
                while (index.Next()) {
                    SYSLOG(Logging::Startup, (_T("Role: %s is undefined"), index.Current().c_str()));
                }
            }
		}

        PluginHost::ISubSystem* subSystem = service->SubSystems();

        ASSERT(subSystem != nullptr);

        if (subSystem != nullptr) {
            Core::Sink<SecurityCallsign> information(service->Callsign());

			if (subSystem->IsActive(PluginHost::ISubSystem::SECURITY) != false)
			{
                SYSLOG(Logging::Startup, (_T("Security is not defined as External !!")));
			}
			else
			{
                subSystem->Set(PluginHost::ISubSystem::SECURITY, &information);
			}

			subSystem->Release();
        }

        // On success return empty, to indicate there is no error text.
        return _T("");
    }

    /* virtual */ void SecurityOfficer::Deinitialize(PluginHost::IShell* service)
    {
        PluginHost::ISubSystem* subSystem = service->SubSystems();

        ASSERT(subSystem != nullptr);

        if (subSystem != nullptr) {
            subSystem->Set(PluginHost::ISubSystem::SECURITY, nullptr);
            subSystem->Release();
        }
        _acl.Clear();
    }

    /* virtual */ string SecurityOfficer::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ uint32_t SecurityOfficer::CreateToken(const uint16_t length, const uint8_t buffer[], string& token)
    {
        // Generate the token from the buffer coming in...
        Web::JSONWebToken newToken(Web::JSONWebToken::SHA256, sizeof(_secretKey), _secretKey);

		return (newToken.Encode(token, length, buffer) > 0 ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
    }

    /* virtual */ PluginHost::ISecurity* SecurityOfficer::Officer(const string& token) {
        PluginHost::ISecurity* result = nullptr;

        Web::JSONWebToken webToken(Web::JSONWebToken::SHA256, sizeof(_secretKey), _secretKey);
        uint16_t load = webToken.PayloadLength(token);

        // Validate the token 
        if (load != static_cast<uint16_t> (~0)) {
			// It is potentially a valid token, extract the payload.
            uint8_t* payload = reinterpret_cast<uint8_t*>(ALLOCA(load));

            load = webToken.Decode(token, load, payload);

			if (load != ~0) {
				// Seems like we extracted a valid payload, time to create an security context
                result = Core::Service<SecurityContext>::Create<SecurityContext>(load, payload);
			}
		}
        return (result);
    }

} // namespace Plugin
} // namespace WPEFramework
