#include <fcntl.h>

#include "RemoteControl.h"
#include "RemoteAdministrator.h"

namespace WPEFramework {
namespace Plugin {

    static const string DefaultMappingTable(_T("default"));
    static Core::ProxyPoolType<Web::JSONBodyType<RemoteControl::Data> > jsonResponseFactory(4);
    static Core::ProxyPoolType<Web::JSONBodyType<PluginHost::VirtualInput::KeyMap::KeyMapEntry> > jsonCodeFactory(1);

    SERVICE_REGISTRATION(RemoteControl, 1, 0);

    class KeyActivity {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        KeyActivity(const KeyActivity& a_Copy) = delete;
        KeyActivity& operator=(const KeyActivity& a_RHS) = delete;

    public:
        KeyActivity(const string& mapName, const uint32_t code, const bool pressed)
            : _text(Core::ToString(Trace::Format(_T("Inserted Code: [%s:%08X] state %s."), mapName.c_str(), code, (pressed ? _T("pressed") : _T("released")))))
        {
        }
        ~KeyActivity()
        {
        }

    public:
        inline const char* Data() const
        {
            return (_text.c_str());
        }
        inline uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        const std::string _text;
    };

    class UnknownKey {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        UnknownKey(const UnknownKey& a_Copy) = delete;
        UnknownKey& operator=(const UnknownKey& a_RHS) = delete;

    public:
        UnknownKey(const string& mapName, const uint32_t code, const bool pressed, const uint32_t result)
        {
            const TCHAR* text(pressed ? _T("pressed") : _T("released"));

            if (result == Core::ERROR_UNKNOWN_TABLE) {
                _text = Core::ToString(Trace::Format(_T("Invalid table: [%s,%08X]"), mapName.c_str(), code));
            }
            else if (result == Core::ERROR_UNKNOWN_KEY) {
                _text = Core::ToString(Trace::Format(_T("Unknown: [%s:%08X] state %s, blocked."), mapName.c_str(), code, text));
            }
            else if (result == Core::ERROR_UNKNOWN_KEY_PASSED) {
                _text = Core::ToString(Trace::Format(_T("Unknown: [%s:%08X] state %s, passed."), mapName.c_str(), code, text));
            }
        }
        ~UnknownKey()
        {
        }

    public:
        inline const char* Data() const
        {
            return (_text.c_str());
        }
        inline uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        std::string _text;
    };

    static Core::ProxyPoolType<Web::TextBody> _remoteInfo(2);

    static string MappingFile(const string& fileName, const string& directory1, const string& directory2)
    {

        string result(directory1 + fileName);
        Core::File testObject(directory1 + fileName);

        if ((testObject.Exists() == false) || (testObject.IsDirectory() == true)) {
            result = directory2 + fileName;
            testObject = result;

            if ((testObject.Exists() == false) || (testObject.IsDirectory() == true)) {
                result.clear();
            }
        }

        return (result);
    }

#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
    RemoteControl::RemoteControl()
        : _skipURL(0)
        , _inputHandler(PluginHost::InputHandler::KeyHandler())
        , _persistentPath()
        , _virtualDevices()
    {
        ASSERT(_inputHandler != nullptr);
    }
#ifdef __WIN32__
#pragma warning(default : 4355)
#endif

    /* virtual */ RemoteControl::~RemoteControl()
    {
    }

    /* virtual */ const string RemoteControl::Initialize(PluginHost::IShell* service)
    {
        string result;
        RemoteControl::Config config;
        config.FromString(service->ConfigLine());
        string mappingFile(MappingFile(config.MapFile.Value(), service->PersistentPath(), service->DataPath()));

        // First check that we at least can create a default lookup table.
        if ((mappingFile.empty() == true) || (_inputHandler == nullptr)) {
            result = "Could not configure remote control.";
        }
        else {
            TRACE(Trace::Information, (_T("Opening default map file: %s"), mappingFile.c_str()));
            TRACE_L1(_T("Opening default map file: %s"), mappingFile.c_str());

            // Keep this path for save operation
            _persistentPath = service->PersistentPath();

            // Seems like we have a default mapping file. Load it..
            PluginHost::VirtualInput::KeyMap& map(_inputHandler->Table(DefaultMappingTable));

            if (map.Load(mappingFile) == Core::ERROR_NONE) {

                map.PassThrough(config.PassOn.Value());

                Remotes::RemoteAdministrator& admin(Remotes::RemoteAdministrator::Instance());

                // Strawl over all remotes (inputs) and see if you need to load mapping tables.
                Remotes::RemoteAdministrator::Iterator index(admin.Producers());
                Core::JSON::ArrayType<RemoteControl::Config::Device>::Iterator configList(config.Devices.Elements());

                while (index.Next() == true) {

                    const TCHAR* producer((*index)->Name());
                    string loadName(producer);

                    TRACE_L1(_T("Searching map file for: %s"), producer);

                    configList.Reset();

                    while ((configList.Next() == true) && (configList.Current().Name.Value() != loadName)) { /* intentionally left empty */
                    }

                    if (configList.IsValid() == true) {
                        (*index)->Configure(configList.Current().Settings.Value());
                        // We found an overruling name.
                        loadName = configList.Current().MapFile.Value();
                    }
                    else {
                        (*index)->Configure(EMPTY_STRING);
                        loadName += _T(".json");
                    }

                    // See if we need to load a table.
                    string specific(MappingFile(loadName, service->PersistentPath(), service->DataPath()));

                    if ((specific.empty() == false) && (specific != mappingFile)) {

                        TRACE(Trace::Information, (_T("Opening map file: %s"), specific.c_str()));
                        TRACE_L1(_T("Opening map file: %s"), specific.c_str());

                        // Get our selves a table..
                        PluginHost::VirtualInput::KeyMap& map(_inputHandler->Table(producer));
                        map.Load(specific);
                        if (configList.IsValid() == true) {
                            map.PassThrough(configList.Current().PassOn.Value());
                        }
                    }
                }

                configList = config.Virtuals.Elements();

                while (configList.Next() == true) {
                    // Configure the virtual inputs.
                    string loadName(configList.Current().MapFile.Value());

                    if (loadName.empty() == true) {
                        loadName = configList.Current().Name.Value() + _T(".json");
                    }

                    // See if we need to load a table.
                    string specific(MappingFile(loadName, service->PersistentPath(), service->DataPath()));

                    if ((specific.empty() == false) && (specific != mappingFile)) {
                        TRACE(Trace::Information, (_T("Opening map file: %s"), specific.c_str()));
                        TRACE_L1(_T("Opening map file: %s"), specific.c_str());

                        // Get our selves a table..
                        PluginHost::VirtualInput::KeyMap& map(_inputHandler->Table(configList.Current().Name.Value()));
                        map.Load(specific);
                        map.PassThrough(configList.Current().PassOn.Value());
                    }

                    _virtualDevices.push_back(configList.Current().Name.Value());
                }

                _skipURL = service->WebPrefix().length();
                _inputHandler->Interval(config.RepeatStart.Value(), config.RepeatInterval.Value());
                _inputHandler->Default(DefaultMappingTable);
                admin.Callback(this);
            }
        }

        // On succes return nullptr, to indicate there is no error text.
        return (result);
    }

    /* virtual */ void RemoteControl::Deinitialize(PluginHost::IShell* service)
    {

        Remotes::RemoteAdministrator& admin(Remotes::RemoteAdministrator::Instance());
        Remotes::RemoteAdministrator::Iterator index(admin.Producers());

        // Clear all injected device key maps
        while (index.Next() == true) {

            const TCHAR* producer((*index)->Name());

            _inputHandler->ClearTable(producer);
        }

        // Clear default key map
        _inputHandler->Default(EMPTY_STRING);
        _inputHandler->ClearTable(DefaultMappingTable);

        // CLear the virtual devices.
        _virtualDevices.clear();

        Remotes::RemoteAdministrator::Instance().RevokeAll();
    }

    /* virtual */ string RemoteControl::Information() const
    {
        // No additional info to report.
        return (EMPTY_STRING);
    }

    /* virtual */ void RemoteControl::Inbound(Web::Request& request)
    {

        request.Body(Core::proxy_cast<Web::IBody>(jsonCodeFactory.Element()));
        jsonCodeFactory.Element().Release();
    }

    /* virtual */ Core::ProxyType<Web::Response> RemoteControl::Process(const Web::Request& request)
    {

        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result;
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

        // By default, we are in front of any element, jump onto the first element, which is if, there is something an empty slot.
        index.Next();

        // If there is nothing or only a slashe, we will now jump over it, and otherwise, we have data.
        if (request.Verb == Web::Request::HTTP_GET) {

            result = GetMethod(index, request);
        }
        else if (request.Verb == Web::Request::HTTP_PUT) {
            result = PutMethod(index, request);
        }
        else if (request.Verb == Web::Request::HTTP_POST) {

            result = PostMethod(index, request);
        }
        else if (request.Verb == Web::Request::HTTP_DELETE) {

            result = DeleteMethod(index, request);
        }
        else {
            result = PluginHost::Factories::Instance().Response();
            result->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
            result->Message = string(_T("Unknown request path specified."));
        }

        return (result);
    }

    /* virtual */ uint32_t RemoteControl::KeyEvent(const bool pressed, const uint32_t code, const string& mapName)
    {

        uint32_t result = _inputHandler->KeyEvent(pressed, code, mapName);

        if (result == Core::ERROR_NONE) {
            TRACE(KeyActivity, (mapName, code, pressed));
        }
        else {
            TRACE(UnknownKey, (mapName, code, pressed, result));
        }
        return (result);
    }

    bool RemoteControl::ParseRequestBody(const Web::Request& request, uint32_t& code, uint16_t& key, uint32_t& modifiers)
    {
        bool parsed = false;

        if (request.HasBody() == true) {

            const PluginHost::VirtualInput::KeyMap::KeyMapEntry& data = *(request.Body<const Web::JSONBodyType<PluginHost::VirtualInput::KeyMap::KeyMapEntry> >());

            if (data.Code.IsSet() == true) {
                code = data.Code.Value();

                if (data.Key.IsSet())
                    key = data.Key.Value();

                if (data.Modifiers.IsSet()) {

                    Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::VirtualInput::KeyMap::modifier> >::ConstIterator flags(data.Modifiers.Elements());

                    while (flags.Next() == true) {
                        switch (flags.Current().Value()) {
                        case PluginHost::VirtualInput::KeyMap::modifier::LEFTSHIFT:
                        case PluginHost::VirtualInput::KeyMap::modifier::RIGHTSHIFT:
                        case PluginHost::VirtualInput::KeyMap::modifier::LEFTALT:
                        case PluginHost::VirtualInput::KeyMap::modifier::RIGHTALT:
                        case PluginHost::VirtualInput::KeyMap::modifier::LEFTCTRL:
                        case PluginHost::VirtualInput::KeyMap::modifier::RIGHTCTRL:
                            modifiers |= flags.Current().Value();
                            break;
                        default:
                            ASSERT(false);
                            break;
                        }
                    }
                }
                parsed = true;
            }
        }
        return (parsed);
    }

    Core::ProxyType<Web::IBody> RemoteControl::CreateResponseBody(const uint32_t code, const uint32_t key, uint16_t modifiers) const
    {

        Core::ProxyType<Web::JSONBodyType<PluginHost::VirtualInput::KeyMap::KeyMapEntry> >
            response(jsonCodeFactory.Element());

        response->Code = code;
        response->Key = key;

        uint16_t flag(1);
        while (modifiers != 0) {

            if ((modifiers & 0x01) != 0) {
                switch (flag) {
                case PluginHost::VirtualInput::KeyMap::modifier::LEFTSHIFT:
                case PluginHost::VirtualInput::KeyMap::modifier::RIGHTSHIFT:
                case PluginHost::VirtualInput::KeyMap::modifier::LEFTALT:
                case PluginHost::VirtualInput::KeyMap::modifier::RIGHTALT:
                case PluginHost::VirtualInput::KeyMap::modifier::LEFTCTRL:
                case PluginHost::VirtualInput::KeyMap::modifier::RIGHTCTRL: {
                    Core::JSON::EnumType<PluginHost::VirtualInput::KeyMap::modifier>& jsonRef = response->Modifiers.Add();
                    jsonRef = static_cast<PluginHost::VirtualInput::KeyMap::modifier>(flag);
                    break;
                }
                default:
                    ASSERT(false);
                    break;
                }
            }

            flag = flag << 1;
            modifiers = modifiers >> 1;
        }

        return (Core::proxy_cast<Web::IBody>(response));
    }

    Core::ProxyType<Web::Response> RemoteControl::GetMethod(Core::TextSegmentIterator& index, const Web::Request& request) const
    {

        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());

        result->ErrorCode = Web::STATUS_NOT_FOUND;
        result->Message = string(_T("Unknown request path specified."));

        if (index.IsValid() == true && index.Next() == true) {
            // Perform operations on that specific device
            const string deviceName = index.Current().Text();

            if ((IsVirtualDevice(deviceName)) || (IsPhysicalDevice(deviceName))) {
                // GET .../RemoteControl/<DEVICE_NAME>?Code=XXX : Get code of DEVICE_NAME
                if (request.Query.IsSet() == true) {
                    Core::URL::KeyValue options(request.Query.Value());

                    Core::NumberType<uint32_t> code(options.Number<uint32_t>(_T("Code"), static_cast<uint32_t>(~0)));

                    result->ErrorCode = Web::STATUS_NOT_FOUND;
                    result->Message = string(_T("Key does not exist in ") + deviceName);

                    if (code.Value() != static_cast<uint32_t>(~0)) {
                        // Load default or specific device mapping
                        PluginHost::VirtualInput::KeyMap& map(_inputHandler->Table(deviceName));

                        const PluginHost::VirtualInput::KeyMap::ConversionInfo* codeElements = map[code];
                        if (codeElements != nullptr) {

                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = string(_T("Get key info of ") + deviceName);
                            result->ContentType = Web::MIMETypes::MIME_JSON;
                            result->Body(CreateResponseBody(code, codeElements->Code, codeElements->Modifiers));
                        }
                    }
                    else {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = string(_T("No key code in request"));
                    }
                }
                // GET .../RemoteControl/<DEVICE_NAME> : Return metadata of specific DEVICE_NAME
                else {

                    if (IsVirtualDevice(deviceName) == true) {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = string(_T("Virtual device is loaded"));
                    }
                    else if (IsPhysicalDevice(deviceName) == true) {
                        uint32_t error = Remotes::RemoteAdministrator::Instance().Error(deviceName);
                        if (error == Core::ERROR_NONE) {
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = string(_T("Physical device is loaded"));

                            Core::ProxyType<Web::TextBody> body(_remoteInfo.Element());

                            (*body) = Remotes::RemoteAdministrator::Instance().MetaData(deviceName);
                            result->Body<Web::TextBody>(body);
                        }
                        else {

                            result->ErrorCode = Web::STATUS_BAD_GATEWAY;
                            result->Message = string(_T("Error during loading of device. ErrorCode: ")) + Core::NumberType<uint32_t>(error).Text();
                        }
                    }
                }
            }
        }
        // GET .../RemoteControl : Return name-list of all registered devices
        else {

            Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());

            // Add virtual devices
            std::list<string>::const_iterator index(_virtualDevices.begin());

            while (index != _virtualDevices.end()) {
                Core::JSON::String newElement;
                newElement = *index;
                response->Devices.Add(newElement);
                index++;
            }

            // Look at specific devices, if we have, append them to response
            Remotes::RemoteAdministrator& admin(Remotes::RemoteAdministrator::Instance());
            Remotes::RemoteAdministrator::Iterator remoteDevices(admin.Producers());

            while (remoteDevices.Next() == true) {

                Core::JSON::String newElement;
                newElement = (*remoteDevices)->Name();
                response->Devices.Add(newElement);
            }

            result->ErrorCode = Web::STATUS_OK;
            result->Message = string(_T("List of loaded remote devices"));
            result->ContentType = Web::MIMETypes::MIME_JSON;
            result->Body(Core::proxy_cast<Web::IBody>(response));
        }

        return (result);
    }

    Core::ProxyType<Web::Response> RemoteControl::PutMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {

        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());

        result->ErrorCode = Web::STATUS_NOT_FOUND;
        result->Message = string(_T("Unknown request path specified."));

        // This is a PUT request, search command
        // PUT RemoteControl/<DEVICE_NAME>?Code=XXX
        if (index.IsValid() == true && index.Next() == true) {
            // Perform operations on that specific device
            const string deviceName(index.Current().Text());
            const bool physical(IsPhysicalDevice(deviceName));

            if ((physical == true) || (IsVirtualDevice(deviceName))) {

                if (index.Next() == true) {

                    bool pressed = false;

                    // PUT .../RemoteControl/<DEVICE_NAME>/Pair: activate pairing mode of specific DEVICE_NAME
                    if (index.Current() == _T("Pair")) {
                        if ((physical == true) && (Remotes::RemoteAdministrator::Instance().Pair(deviceName) == true)) {
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = string(_T("Pairing mode active: ") + deviceName);
                        }
                        else {
                            result->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
                            result->Message = string(_T("Failed to activate pairing: ") + deviceName);
                        }
                    }
                    // PUT .../RemoteControl/<DEVICE_NAME>/Unpair : unpair specific DEVICE_NAME & bindId
                    if (index.Current() == _T("Unpair")) {

                        if (index.Next() == true) {
                            if (Remotes::RemoteAdministrator::Instance().Unpair(deviceName, index.Current().Text()) == true) {
                                result->ErrorCode = Web::STATUS_OK;
                                result->Message = string(_T("Unpaired ") + deviceName);
                            }
                            else {
                                result->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
                                result->Message = string(_T("Failed to unpair: ") + deviceName);
                            }
                        }
                    }
                    // PUT .../RemoteControl/<DEVICE_NAME>/Send : send a code to DEVICE_NAME
                    else if (index.Current() == _T("Send")) {
                        uint32_t code = 0;
                        uint16_t key = 0;
                        uint32_t modifiers = 0;
                        if (ParseRequestBody(request, code, key, modifiers) == true) {

                            result->ErrorCode = Web::STATUS_NOT_FOUND;
                            result->Message = string(_T("Key does not exist in ") + deviceName);

                            if (code != 0) {

                                uint32_t errCode = KeyEvent(true, code, deviceName);
                                if (errCode == Core::ERROR_NONE) {

                                    errCode = KeyEvent(false, code, deviceName);
                                    if (errCode == Core::ERROR_NONE) {

                                        result->ErrorCode = Web::STATUS_ACCEPTED;
                                        result->Message = string(_T("Soft key is sent to ") + deviceName);
                                    }
                                }
                            }
                        }
                        else {
                            result->ErrorCode = Web::STATUS_BAD_REQUEST;
                            result->Message = string(_T("No key code in request"));
                        }
                    }
                    // PUT .../RemoteControl/<DEVICE_NAME>/Press|Release : send a code to DEVICE_NAME
                    else if (((pressed = (index.Current() == _T("Press"))) == true) || (index.Current() == _T("Release"))) {

                        uint32_t code = 0;
                        uint16_t key = 0;
                        uint32_t modifiers = 0;
                        if (ParseRequestBody(request, code, key, modifiers) == true) {

                            result->ErrorCode = Web::STATUS_NOT_FOUND;
                            result->Message = string(_T("Key does not exist in ") + deviceName);

                            if ((code != 0) && (KeyEvent(pressed, code, deviceName) == Core::ERROR_NONE)) {

                                result->ErrorCode = Web::STATUS_ACCEPTED;
                                result->Message = string(_T("Soft key is sent to ") + deviceName);
                            }
                        }
                        else {
                            result->ErrorCode = Web::STATUS_BAD_REQUEST;
                            result->Message = string(_T("No key code in request"));
                        }
                    }
                    // PUT .../RemoteControl/<DEVICE_NAME>/Save : Save the loaded key map as DEVICE_NAME.json into persistent path
                    else if (index.Current() == _T("Save")) {

                        result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                        result->Message = string(_T("File is not created"));

                        string fileName;
                        if (_persistentPath.empty() == false) {
                            Core::Directory directory(_persistentPath.c_str());
                            if (directory.CreatePath()) {
                                fileName = _persistentPath + deviceName + _T(".json");
                            }
                        }

                        if (fileName.empty() == false) {
                            // Seems like we have a default mapping file. Load it..
                            PluginHost::VirtualInput::KeyMap& map(_inputHandler->Table(deviceName));

                            if (map.Save(fileName) == Core::ERROR_NONE) {
                                result->ErrorCode = Web::STATUS_OK;
                                result->Message = string(_T("File is created: " + fileName));
                            }
                        }
                    }
                    // PUT .../RemoteControl/<DEVICE_NAME>/Load : Re-load DEVICE_NAME.json into memory
                    else if (index.Current() == _T("Load")) {

                        result->ErrorCode = Web::STATUS_NOT_FOUND;
                        result->Message = string(_T("File does not exist"));

                        string fileName;
                        if (_persistentPath.empty() == false) {
                            Core::Directory directory(_persistentPath.c_str());
                            if (directory.CreatePath()) {
                                fileName = _persistentPath + deviceName + _T(".json");
                            }
                        }

                        if (fileName.empty() == false) {
                            PluginHost::VirtualInput::KeyMap& map(_inputHandler->Table(deviceName));

                            if (map.Load(fileName) == Core::ERROR_NONE) {
                                result->ErrorCode = Web::STATUS_OK;
                                result->Message = string(_T("File is reloaded: " + deviceName));
                            }
                        }
                    }
                }
                else {
                    uint32_t code = 0;
                    uint16_t key = 0;
                    uint32_t modifiers = 0;
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = string(_T("Bad json data format"));

                    if (ParseRequestBody(request, code, key, modifiers) == true) {
                        // Valid code-key pair
                        if (code != 0 && key != 0) {
                            // Load default or specific device mapping
                            PluginHost::VirtualInput::KeyMap& map(_inputHandler->Table(deviceName));

                            if (map.Add(code, key, modifiers) == true) {
                                result->ErrorCode = Web::STATUS_CREATED;
                                result->Message = string(_T("Code is added"));
                            }
                            else {
                                result->ErrorCode = Web::STATUS_FORBIDDEN;
                                result->Message = string(_T("Code already exists"));
                            }
                        }
                    }
                }
            }
        }
        return (result);
    }

    Core::ProxyType<Web::Response> RemoteControl::DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());

        result->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
        result->Message = string(_T("Unknown request path specified."));

        // DELETE .../RemoteControl/<DEVICE_NAME>: delete code from mapping of DEVICE_NAME
        if (index.IsValid() == true && index.Next() == true) {
            // Perform operations on that specific device
            const string deviceName(index.Current().Text());

            const bool physical(IsPhysicalDevice(deviceName));

            if ((physical == true) || (IsVirtualDevice(deviceName))) {

                if (index.Next() == false) {

                    uint32_t code = 0;
                    uint16_t key = 0;
                    uint32_t modifiers = 0;

                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = string(_T("Bad json data format"));

                    if (ParseRequestBody(request, code, key, modifiers) == true) {

                        if (code != 0) {

                            // Load default or specific device mapping
                            PluginHost::VirtualInput::KeyMap& map(_inputHandler->Table(deviceName));

                            map.Delete(code);

                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = string(_T("Code is deleted"));
                        }
                        else {
                            result->ErrorCode = Web::STATUS_FORBIDDEN;
                            result->Message = string(_T("Key does not exist in ") + deviceName);
                        }
                    }
                }
            }
        }
        return (result);
    }

    Core::ProxyType<Web::Response> RemoteControl::PostMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());

        result->ErrorCode = Web::STATUS_NOT_FOUND;
        result->Message = string(_T("Unknown request path specified."));

        // POST .../RemoteControl/<DEVICE_NAME> : Modify a new pair in specific DEVICE_NAME
        if (index.IsValid() == true && index.Next() == true) {
            // Perform operations on that specific device
            const string deviceName(index.Current().Text());

            const bool physical(IsPhysicalDevice(deviceName));

            if ((physical == true) || (IsVirtualDevice(deviceName))) {

                if (index.Next() == false) {

                    uint32_t code = 0;
                    uint16_t key = 0;
                    uint32_t modifiers = 0;

                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = string(_T("Bad json data format"));
                    if (ParseRequestBody(request, code, key, modifiers) == true) {
                        // Valid code-key pair
                        if (code != 0 && key != 0) {
                            // Load default or specific device mapping
                            PluginHost::VirtualInput::KeyMap& map(_inputHandler->Table(deviceName));

                            if (map.Modify(code, key, modifiers) == true) {
                                result->ErrorCode = Web::STATUS_OK;
                                result->Message = string(_T("Code is modified"));
                            }
                            else {
                                result->ErrorCode = Web::STATUS_FORBIDDEN;
                                result->Message = string(_T("Code does not exist"));
                            }
                        }
                    }
                }
            }
        }
        return (result);
    }
}
}
