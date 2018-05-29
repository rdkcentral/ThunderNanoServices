#include "Controller.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(WPASupplicant::Controller::events)

	{ WPASupplicant::Controller::CTRL_EVENT_SCAN_STARTED,      _TXT("CTRL-EVENT-SCAN-STARTED")      },
	{ WPASupplicant::Controller::CTRL_EVENT_SCAN_RESULTS,      _TXT("CTRL-EVENT-SCAN-RESULTS")      },
	{ WPASupplicant::Controller::CTRL_EVENT_CONNECTED,         _TXT("CTRL-EVENT-CONNECTED")         },
	{ WPASupplicant::Controller::CTRL_EVENT_DISCONNECTED,      _TXT("CTRL-EVENT-DISCONNECTED")      },
	{ WPASupplicant::Controller::CTRL_EVENT_BSS_ADDED,         _TXT("CTRL-EVENT-BSS-ADDED")         },
        { WPASupplicant::Controller::CTRL_EVENT_BSS_REMOVED,       _TXT("CTRL-EVENT-BSS-REMOVED")       },
        { WPASupplicant::Controller::CTRL_EVENT_TERMINATING,       _TXT("CTRL-EVENT-TERMINATING")       },
        { WPASupplicant::Controller::CTRL_EVENT_NETWORK_NOT_FOUND, _TXT("CTRL-EVENT-NETWORK-NOT-FOUND") },
	{ WPASupplicant::Controller::WPS_AP_AVAILABLE,             _TXT("WPS-AP-AVAILABLE")             },
        { WPASupplicant::Controller::AP_ENABLED,                   _TXT("AP-ENABLED")                   },

ENUM_CONVERSION_END(WPASupplicant::Controller::events);

namespace WPASupplicant {

    static const TCHAR hexArray[] = "0123456789abcdef";

    class EXTERNAL Communication {
    private:
        Communication(const Communication& a_Copy) = delete;
	Communication& operator=(const Communication& a_RHS) = delete;

    public:
        inline Communication()
        {
        }
	inline Communication(const TCHAR formatter[], ...)
	{
		va_list ap;
		va_start(ap, formatter);
		Trace::Format(_text, formatter, ap);
		va_end(ap);
	}
	inline Communication(const std::string& text)
            : _text(Core::ToString(text.c_str()))
        {
        }
        inline Communication(const char text[])
            : _text(Core::ToString(text))
        {
        }
        ~Communication()
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

    /* static */ string Controller::BSSID(const uint64_t& bssid) {

        TCHAR text[4];
        string result;    
        text[0] = ':';
        text[3] = '\0';
        for (uint8_t teller = 0; teller < 6; teller++) {
            uint8_t digit  = static_cast<uint8_t>((bssid >> ((5 - teller) * 8)) & 0xFF);
            text[1] = hexArray[(digit >> 4) & 0xF];
            text[2] = hexArray[digit & 0xF];
            result += string(&text[(result.empty() == false ? 0 : 1)]);
        }
        return (result);
    }

    /* static */ uint64_t Controller::BSSID(const string& element) {

        uint64_t bssid = 0;

        if (element.length() >= static_cast<uint32_t>((3 * 6) - 1)) {
            // First thing we find will be 6 bytes...
            for (uint8_t pos = 0; pos < 6; pos++) {
                uint8_t msb = element[0 + (3 * pos)];
                uint8_t lsb = element[1 + (3 * pos)];
                if (::isxdigit(msb) && ::isxdigit(lsb)) {
                    bssid = (bssid << 8) | 
                        ((msb > '9' ? ::tolower(msb) - 'a' + 10 : msb - '0') << 4) |
                         (lsb > '9' ? ::tolower(lsb) - 'a' + 10 : lsb - '0');
                }
            }
        }
        return (bssid);
    }

    /* static */ uint16_t Controller::KeyPair(const Core::TextFragment& infoLine, uint32_t& keys) {
        uint16_t pairs = 0;
        uint16_t index = 0;
        uint16_t end = infoLine.Length();
        keys = 0;

        while (index < end) {
            uint16_t keyBegin;
            uint16_t keyEnd;

            while ((index < end) && (infoLine[index] != '[')) { index++; }

            keyBegin = index + 1;
        
            while ((index < end) && (infoLine[index] != ']')) { index++; }

            keyEnd = index;

            if (keyBegin < keyEnd) {
                // Find enum value
                Core::TextFragment entry (Core::TextFragment(infoLine, keyBegin, keyEnd - keyBegin));
                Core::TextSegmentIterator element (entry, false, _T("+-"));

                // The first key should be the pair system:
                Core::EnumerateType<Network::pair> pair (element.Next() == true ? element.Current() : entry);

                if (pair.IsSet() == true) {
                    uint32_t value = pair.Value();

                    if (value != 0) {

                        uint32_t shift = 0;

                        pairs |= pair.Value();

                        while ((value & 0x1) == 0) { shift += 8; value = value >> 1; }

                        while (element.Next() == true) {
                            Core::EnumerateType<Network::key> value (element.Current());
                            keys |= (value.IsSet() ? (value.Value() << shift) : 0);
                        }
                    }
                }
            }
        }

        return (pairs);
    }

    /* virtual */ uint16_t Controller::SendData(uint8_t* dataFrame, const uint16_t maxSendSize) {
        uint16_t result = 0;

        if ((_requests.size() > 0) && (_requests.front()->Message().empty() == false)) {
            string& data = _requests.front()->Message();
            TRACE(Communication, (_T("Send: [%s]"), data.c_str()));
            result = (data.length() > maxSendSize ? maxSendSize : data.length());
            memcpy (dataFrame, data.c_str(), result);
            data = data.substr(result);
        }
        return (result);
    }
    /* virtual */ uint16_t Controller::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) {

        string response = string(reinterpret_cast<const char*>(dataFrame), (dataFrame[receivedSize-1] == '\n' ? receivedSize - 1 : receivedSize));

        if (response[0] == '<') {

            uint32_t number = 0;
            uint16_t index = 1;

            // See if there are only digit between this and the closing bracket.
            while ( (index < response.length()) && (isdigit(response[index]) == true) ) {
                number += (number * 10) + (response[index] - '0');
                ++index;
            }

            ASSERT ((index < response.length()) && (response[index] == '>'));

            // Seems that sometimes the end is a WhiteSpace, remove it as well.
            string message = (response.substr(index+1, (response[response.length()-1] == ' ' ? response.length() - 2 - index : string::npos)));

	    const size_t position = message.find(' ');

            // Looks like it is a unsolicited message
            Core::EnumerateType<Controller::events> event (position == string::npos ? message.c_str() : message.substr(0, position).c_str());

            if (event.IsSet() == true) {

                TRACE(Communication, (_T("Dispatch message: [%s]"), message.c_str()));

                if ( (event == CTRL_EVENT_CONNECTED) || (event == CTRL_EVENT_DISCONNECTED) || (event == WPS_AP_AVAILABLE) ) {
                    _statusRequest.Event(event.Value());
                    Submit(&_statusRequest);
                }
                else if ((_networks.size() == 0) && (event.Value() == CTRL_EVENT_SCAN_RESULTS) && (_scanRequest.Set() == true)) {
                    Submit(&_scanRequest);
                }
                else if ( (event ==  CTRL_EVENT_BSS_ADDED) || (event ==  CTRL_EVENT_BSS_REMOVED) ) {

                    ASSERT (position != string::npos);

                    Core::TextFragment infoLine (message, position, message.length() - position);

                    // extract the Network ID from the list
                    uint16_t index = infoLine.ForwardSkip(_T(" \t"), 0);

                    // Skip over the Network ID
                    index = infoLine.ForwardFind(_T(" \t"), index);

                    // Skip this white space then we are at the BSSID
                    index = infoLine.ForwardSkip(_T(" \t"), index);

                    // now take out the BSSID
                    uint64_t bssid = BSSID(Core::TextFragment(infoLine, index, infoLine.Length() - index).Text());

                    _adminLock.Lock();

                    // Let see what we need to do with this BSSID, add or remove :-)
                    if ((event ==  CTRL_EVENT_BSS_ADDED) && (_detailRequest.Set(bssid) == true))  {
                        // send out a request for detail.
                        Submit (&_detailRequest);
                    }
                    else if (event ==  CTRL_EVENT_BSS_REMOVED) {

                        NetworkInfoContainer::iterator network (_networks.find(bssid));

                        if (network != _networks.end()) {
                            _networks.erase(network);
                        }
                    }
  
                    if (_callback != nullptr) {
                        _callback->Dispatch(event.Value());
                    }

                    _adminLock.Unlock();
                }
           }
            else {
                TRACE(Communication, (_T("RAW EVENT MESSAGE: [%s]"), message.c_str()));
            }
        }
        else {
            _adminLock.Lock();

            Request* current = _requests.front();
            _requests.pop_front();
            current->Completed(response, false);

            _adminLock.Unlock();

            if (_requests.size() > 0) {
                Trigger();
            }
        }

        return(receivedSize);
    }
  // These methods (add/add/update) are assumed to be running in a locked context. 
   // Completion of requests are running in a locked context, so oke to update maps/lists
   void Controller::Add (const uint64_t& bssid, const NetworkInfo& entry) {
        TRACE(Communication, (_T("Added SSID: %llX - %s"), bssid, entry.SSID().c_str()));
       _networks[bssid] = entry;
       Reevaluate();
   }
   void Controller::Add (const string& ssid, const bool current, const uint64_t& bssid) {
        TRACE(Communication, (_T("Added Network: %s"), ssid.c_str()));

       NetworkInfoContainer::iterator index (_networks.begin());

       while (index != _networks.end()) {
           if (ssid == index->second.SSID()) {
               if (_enabled.find(index->second.SSID()) == _enabled.end()) {
                   // This is an enabled network.
                   TRACE(Communication, (_T("Add network from network list: %d"), index->second.Id()));
                   _enabled[index->second.SSID()] = ConfigInfo(index->second.Id(), true);

               }
           }
           index++;
       }

       Reevaluate();
    }
    void Controller::Update (const uint64_t& bssid, const string& ssid, const uint32_t id, uint32_t frequency, const int32_t signal, const uint16_t pairs, const uint32_t keys, const uint32_t throughput) {

       bool scanInProgress = false;

       NetworkInfoContainer::iterator index (_networks.find(bssid));

       if (index != _networks.end()) {

           scanInProgress = (index->second.Id() == static_cast<uint32_t>(~0));

           TRACE(Communication, (_T("Updated BSSID: %llX, %d"), bssid, id));

           index->second.Set(id, ssid, frequency, signal, pairs, keys, throughput);
      }
      else {
          _networks[bssid] = NetworkInfo(id, ssid, frequency, signal, pairs, keys, throughput);
      }

      if (scanInProgress == true) {
          Reevaluate();
      }
      else if (_callback != nullptr) {
          _callback->Dispatch(CTRL_EVENT_NETWORK_CHANGED);
      }
   }
 
   void Controller::Update (const string& ssid, const uint32_t id, const bool succeeded) {
       NetworkInfoContainer::iterator index (_networks.begin());

       // See if we can find the associated BSSID
       index = _networks.begin();
       while ((index != _networks.end()) && (index->second.Id() != id)) {
           index++;
       }

       if (index != _networks.end()) {
           TRACE(Communication, (_T("Linked: %s to known: %d"), index->second.SSID().c_str(), id));
           _enabled[ssid] = ConfigInfo((succeeded ? id : 0), false);
       }
       else {
           TRACE(Communication, (_T("Linked: %llX, %d"), index->first, id));
           _enabled[ssid] = ConfigInfo((succeeded ? id : 0), false);
       }

       Reevaluate();
   }
   void Controller::Reevaluate() {

       NetworkInfoContainer::iterator index (_networks.begin());

       while ((index != _networks.end()) && (index->second.HasDetail() == true)) {
           index++;
       }
       if (index != _networks.end()) {
           if (_detailRequest.Set(index->first) == true) {
               // send out a request for detail.
               Submit (&_detailRequest);
           }
       }
       else if (_enabled.size() == 0) {
           // send out a request for the network list
            if (_networkRequest.Set() == true) {
               // send out a request for detail.
               Submit (&_networkRequest);
           }
       }
       else {
           _callback->Dispatch(CTRL_EVENT_NETWORK_CHANGED);
       }
    }

} } // WPEFramework::WPASupplicant
