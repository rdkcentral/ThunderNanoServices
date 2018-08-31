#ifndef __CENCPARSER_H
#define __CENCPARSER_H

#include "Module.h"
#include <ocdm/IOCDM.h>

namespace WPEFramework {
namespace Plugin {

//This class is not Thread Safe. The user of this class must ensure thread saftey (single thread access) !!!!
class CommonEncryptionData {
private:
    CommonEncryptionData() = delete;
    CommonEncryptionData& operator= (const CommonEncryptionData&) = delete;

    static const uint8_t PSSHeader[];
    static const uint8_t CommonEncryption[];
    static const uint8_t PlayReady[];
    static const uint8_t WideVine[];
    static const uint8_t ClearKey[];
    static const uint8_t HexArray[];

public:
    enum systemType {
        COMMON    = 0x0001,
        CLEARKEY  = 0x0002,
        PLAYREADY = 0x0004,
        WIDEVINE  = 0x0008
    };

    class KeyId {
    private:
        static const KeyId InvalidKey;

    public:
        inline KeyId()
            : _systems(0)
            , _status(::OCDM::ISession::StatusPending) {
            ::memset(_kid, ~0, sizeof(_kid));
        }
        inline KeyId(const systemType type, const uint8_t kid[], const uint8_t length) 
            : _systems(type)
            , _status(::OCDM::ISession::StatusPending) {
            uint8_t copyLength (length > sizeof(_kid) ? sizeof(_kid) : length);

            ::memcpy(_kid, kid, copyLength);

            if (copyLength < sizeof(_kid)) {
                ::memset(&(_kid[copyLength]), 0, sizeof(_kid) - copyLength);
            }
        }
        // Microsoft playready XML flavor retrieval of KID
        inline KeyId(const systemType type, const uint32_t a, const uint16_t b, const uint16_t c, const uint8_t d[])
            : _systems(type)
            , _status(::OCDM::ISession::StatusPending) {
            // A bit confused on how the mapping of the Microsoft KeyId's should go, looking at the spec:
            // https://msdn.microsoft.com/nl-nl/library/windows/desktop/aa379358(v=vs.85).aspx
            // Some test cases have a little endian byte ordering for the GUID, other a MSB ordering.
            _kid[0] = a & 0xFF;
            _kid[1] = (a >> 8) & 0xFF;
            _kid[2] = (a >> 16) & 0xFF;
            _kid[3] = (a >> 24) & 0xFF;
            _kid[4] = b & 0xFF;
            _kid[5] = (b >> 8) & 0xFF;
            _kid[6] = c & 0xFF;
            _kid[7] = (c >> 8) & 0xFF;

            ::memcpy(&(_kid[8]), d, 8);
        }
        inline KeyId(const KeyId& copy)
            : _systems(copy._systems)
            , _status(copy._status) {
            ::memcpy(_kid, copy._kid, sizeof(_kid));
        }
        inline ~KeyId() {
        }

        inline KeyId& operator= (const KeyId& rhs) {
            _systems = rhs._systems;
            _status = rhs._status;
            ::memcpy(_kid, rhs._kid, sizeof(_kid));
            return (*this);
        }

    public:
        inline bool IsValid () const {
            return (operator!=(InvalidKey));
        }
        inline bool operator==(const uint8_t rhs[]) const {
            // Hack, in case of PlayReady, the key offered on the interface might be
            // ordered incorrectly, cater for this situation, by silenty comparing with this incorrect value.
            bool equal = false;

            // Regardless of the order, the last 8 bytes should be equal
            if (memcmp (&_kid[8], &(rhs[8]), 8)  == 0) {

                // Lets first try the non swapped byte order.
                if (memcmp (_kid, rhs, 8)  == 0) {
                    // this is a match :-)
                    equal = true;
                }
                else {
                    // Let do the byte order alignment as suggested in the spec and see if it matches than :-)
                    // https://msdn.microsoft.com/nl-nl/library/windows/desktop/aa379358(v=vs.85).aspx
                    uint8_t alignedBuffer[8];
                    alignedBuffer[0] = rhs[3];
                    alignedBuffer[1] = rhs[2];
                    alignedBuffer[2] = rhs[1];
                    alignedBuffer[3] = rhs[0];
                    alignedBuffer[4] = rhs[5];
                    alignedBuffer[5] = rhs[4];
                    alignedBuffer[6] = rhs[7];
                    alignedBuffer[7] = rhs[6];
                    equal = (memcmp (_kid, alignedBuffer, 8)  == 0);
                }
            }
            return (equal);
        }
        inline bool operator!=(const uint8_t rhs[]) const {
            return !(operator==(rhs));
        }
        inline bool operator==(const KeyId& rhs) const {
            return (operator==(rhs._kid));
        }
        inline bool operator!=(const KeyId& rhs) const {
            return !(operator==(rhs));
        }
        inline const uint8_t* Id() const {
            return (_kid);
        }
        inline static uint8_t Length() {
            return (sizeof(_kid));
        }
        inline void Flag(const uint32_t systems) {
            _systems |= systems;
        }
        inline uint32_t Systems() const {
            return (_systems);
        }
        inline string ToString() const {
            string result;
            for (uint8_t teller = 0; teller < sizeof(_kid); teller++) {
                result += HexArray[(_kid[teller] >> 4) & 0x0f];
                result += HexArray[_kid[teller] &  0x0f];
            }
            return (result);
        }
        void Status (::OCDM::ISession::KeyStatus status) {
            _status = status;
        }
        ::OCDM::ISession::KeyStatus Status () const {
            return (_status);
        }
        
    private:
        uint8_t  _kid[16];
        uint32_t _systems;
        ::OCDM::ISession::KeyStatus _status;
    };

    typedef Core::IteratorType< const std::list<KeyId>, const KeyId&, std::list<KeyId>::const_iterator > Iterator;

public:
    CommonEncryptionData(const uint8_t data[], const uint16_t length) : _keyIds() {
        Parse(data, length);
    }
    CommonEncryptionData(const CommonEncryptionData& copy) :  _keyIds(copy._keyIds) {
    }
    ~CommonEncryptionData() {
    }

public:
    inline ::OCDM::ISession::KeyStatus Status() const {
        return (_keyIds.size() > 0 ? _keyIds.begin()->Status() : ::OCDM::ISession::StatusPending);
    }
    inline ::OCDM::ISession::KeyStatus Status(const KeyId& key) const {
        ::OCDM::ISession::KeyStatus result (::OCDM::ISession::StatusPending);
        if (key.IsValid() == true) {
            std::list<KeyId>::const_iterator index (std::find(_keyIds.begin(), _keyIds.end(), key));
            if (index != _keyIds.end()) {
                result = index->Status();
            }
        }
        return (result);
    }
    inline Iterator Keys() const {
        return (Iterator(_keyIds));
    }
    inline bool HasKeyId(const uint8_t keyId[]) const {
        return (std::find(_keyIds.begin(), _keyIds.end(), keyId) != _keyIds.end());
    }
    inline void AddKeyId(const KeyId& key) {
       std::list<KeyId>::iterator index (std::find(_keyIds.begin(), _keyIds.end(), key));

       if (index == _keyIds.end()) {
            TRACE_L1("Added key: %s for system: %02X\n", key.ToString().c_str(), key.Systems());
            _keyIds.emplace_back(key);
        }
        else {
            TRACE_L1("Updated key: %s for system: %02X\n", key.ToString().c_str(), key.Systems());
            index->Flag(key.Systems());
        }
    }
    inline const KeyId* UpdateKeyStatus(::OCDM::ISession::KeyStatus status, const KeyId& key) {
        KeyId* entry = nullptr;

        if (key.IsValid() == true) {
            std::list<KeyId>::iterator index (std::find(_keyIds.begin(), _keyIds.end(), key));

            if (index == _keyIds.end()) {
                _keyIds.emplace_back(key);
                entry = &(_keyIds.back());
            }
            else {
                entry = &(*index);
            }
        }
        else if (_keyIds.size() > 0) {
            // Just update the first key
            entry = &(*(_keyIds.begin()));
        }

        if (entry != nullptr) {
            entry->Status(status);
        }

        return (entry);
    }
    inline bool IsSupported(const CommonEncryptionData& keys) const {

        bool result = true;
        std::list<KeyId>::const_iterator requested(keys._keyIds.begin());

        while ((requested != keys._keyIds.end()) && (result == true)) {
            std::list<KeyId>::const_iterator index(_keyIds.begin());

            while ((index != _keyIds.end()) && (*index != *requested)) { index++; }

            result = (index != _keyIds.end());

            requested++;
        }

        return (result);
    }


private:
    uint8_t Base64(const uint8_t value[], const uint8_t sourceLength, uint8_t object[], const uint8_t length)
    {
        uint8_t state = 0;
        uint8_t index = 0;
        uint8_t filler = 0;
        uint8_t lastStuff = 0;

        while ((index < sourceLength) && (filler < length)) {
            uint8_t converted;
            uint8_t current = value[index];

            if ((current >= 'A') && (current <= 'Z')) {
                converted = static_cast<uint8_t>(current - 'A');
            }
            else if ((current >= 'a') && (current <= 'z')) {
                converted = static_cast<uint8_t>(current - 'a' + 26);
            }
            else if ((current >= '0') && (current <= '9')) {
                converted = static_cast<uint8_t>(current - '0' + 52);
            }
            else if (current == '+') {
                converted = 62;
            }
            else if (current == '/') {
                converted = 63;
            }
            else {
                break;
            }

            if (state == 0) {
                lastStuff = converted << 2;
                state = 1;
            }
            else if (state == 1) {
                object[filler++] = (((converted & 0x30) >> 4) | lastStuff);
                lastStuff = ((converted & 0x0F) << 4);
                state = 2;
            }
            else if (state == 2) {
                object[filler++] = (((converted & 0x3C) >> 2) | lastStuff);
                lastStuff = ((converted & 0x03) << 6);
                state = 3;
            }
            else if (state == 3) {
                object[filler++] = ((converted & 0x3F) | lastStuff);
                state = 0;
            }
            index++;
            index++;
        }

        return (filler);
    }

    void Parse(const uint8_t data[], const uint16_t length) {
        uint16_t offset = 0; 

        do {
            // Check if this is a PSSH box...
            uint32_t size = (data[offset] << 24) | (data[offset+1] << 16) | (data[offset+2] << 8) | data[offset+3];
            if (size == 0) {
               TRACE_L1("While parsing CENC, found chunk of size 0, are you sure the data is valid? %d\n", __LINE__);
               break;
            }

            if ((size <= static_cast<uint32_t>(length - offset)) && (memcmp(&(data[offset+4]), PSSHeader, 4) == 0)) {
                ParsePSSHBox(&(data[offset + 4 + 4]), size - 4 - 4);
            }
            else {
                uint32_t XMLSize = (data[offset] | (data[offset+1] << 8) | (data[offset+2] << 16) | (data[offset+3] << 24));

                if (XMLSize <= static_cast<uint32_t>(length - offset)) {

                    size += XMLSize;

                    // Seems like it is an XMLBlob, without PSSH header, we have seen that on PlayReady only.. 
                    ParseXMLBox(&(data[offset]), size);
                }
                else {
                    TRACE_L1("Have no clue what this is!!! %d\n", __LINE__);
                }
            }
            offset += size;

        } while (offset < length);
    }

    void ParsePSSHBox (const uint8_t data[], const uint16_t length) {
        systemType     system   (COMMON);
        const uint8_t* psshData (&(data[KeyId::Length() + 4 /* flags */]));
        uint32_t       count    ((psshData[0] << 24) | (psshData[1] << 16) | (psshData[2] << 8) | psshData[3]);

        if (::memcmp(&(data[4]), CommonEncryption, KeyId::Length()) == 0) {
            psshData += 4;
            TRACE_L1("Common detected [%d]\n", __LINE__);
        }
        else if (::memcmp(&(data[4]), PlayReady, KeyId::Length()) == 0) {
            if (ParseXMLBox (&(psshData[4]), count) == true) {
                TRACE_L1("PlayReady XML detected [%d]\n", __LINE__);
                count = 0;
            }
            else {
                TRACE_L1("PlayReady BIN detected [%d]\n", __LINE__);
                system = PLAYREADY;
                psshData += 4;
            }
        }
        else if (::memcmp(&(data[4]), WideVine, KeyId::Length()) == 0) {
            TRACE_L1("WideVine detected [%d]\n", __LINE__);
            system = WIDEVINE;
            psshData += 4 + 4 /* God knows what this uint32 means, we just skip it. */;
        }
        else if (::memcmp(&(data[4]), ClearKey, KeyId::Length()) == 0) {
            TRACE_L1("ClearKey detected [%d]\n", __LINE__);
            system = CLEARKEY;
            psshData += 4;
        }
        else {
            TRACE_L1 ("Unknown system: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X.\n", data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11]);
            count = 0;
        }

        if (data[0] != 1) {
            count /= KeyId::Length();
        }

        TRACE_L1("Adding %d keys from PSSH box\n", count);

        while (count-- != 0) {
            AddKeyId(KeyId(system, psshData, KeyId::Length()));
            psshData += KeyId::Length();
        }
    }

    uint16_t FindInXML(const uint8_t data[], const uint16_t length, const char key[], const uint8_t keyLength) {
        uint8_t  index = 0;
        uint16_t result = 0;

        while ((result <= length) && (index < keyLength)) {
            if (static_cast<uint8_t>(key[index]) == data[result]) {
                index++;
                result += 2;
            }
            else if (index > 0) {
                result -= ((index - 1) * 2); 
                index   = 0;
            }
            else {
                result += 2;
            }
        }
        return ((index == keyLength) ? (result - (keyLength * 2)) : result);
    }    

    bool ParseXMLBox (const uint8_t data[], const uint16_t length) {

        uint32_t size = (data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
        uint16_t stringLength = (data[8] | (data[9] << 8));
        bool     result = ((size == length) && (stringLength == (length - 10)));

        if (result == true) {
            uint16_t begin;
            uint16_t size = (length - 10);
            const uint8_t* slot = &data[10];

            // Now find the string <KID> in this text
            while ((size > 0) && ((begin = FindInXML (slot, size, "<KID>", 5)) < size)) {
                uint16_t end = FindInXML (&(slot[begin + 10]), size - begin - 10, "</KID>", 6);

                if (end < (size - begin - 10)) {
                    uint8_t byteArray[32];

                    // We got a KID, translate it
                    if (Base64(&(slot[begin + 10]), static_cast<uint8_t>(end), byteArray, sizeof(byteArray)) == KeyId::Length()) {
                        // Pass it the microsoft way :-(
			uint32_t a = byteArray[0];
			         a = (a << 8)  | byteArray[1];
			         a = (a << 8)  | byteArray[2];
			         a = (a << 8)  | byteArray[3];
                        uint16_t b = byteArray[4];
                                 b = (b << 8) | byteArray[5];
                        uint16_t c = byteArray[6];
                                 c = (c << 8) | byteArray[7];
                        uint8_t* d = &byteArray[8];

                        // Add them in both endiannesses, since we have encountered both in the wild.
                        AddKeyId(KeyId(PLAYREADY, a, b, c, d));
                    }
                    size -= (begin + 10 +  end + 12);
                    slot += (begin + 10 +  end + 12);
                }
                else {
                    size = 0;
                }
            }
        }
        return (result);
    }

private:
    std::list<KeyId> _keyIds;
};

} } // namespace WPEFramework::Plugin

#endif // __CENCPARSER_H
