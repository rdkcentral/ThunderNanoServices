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

#include "DHCPClientImplementation.h"

namespace WPEFramework {

namespace Plugin {

    /* static */ constexpr uint8_t DHCPClient::MagicCookie[];

    DHCPClient::DHCPClient(const string& interfaceName, ICallback* callback)
        : Core::SocketDatagram(false, Core::NodeId(), Core::NodeId(), 512, 1024)
        , _adminLock()
        , _interfaceName(interfaceName)
        , _state(IDLE)
        , _modus(CLASSIFICATION_INVALID)
        , _serverIdentifier(0)
        , _xid(0)
        , _discoverXID(0)
        , _acknowledgeXID(0)
        , _offers()
        , _callback(callback)
    {
        Core::AdapterIterator adapters(_interfaceName);

        if (adapters.IsValid() == true) {
            adapters.MACAddress(_MAC, sizeof(_MAC));
        } else {
            TRACE_L1("Could not read mac address of %s\n", _interfaceName.c_str());
        }
    }

    /* virtual */ DHCPClient::~DHCPClient()
    {
        SocketDatagram::Close(Core::infinite);
    }

    // Methods to extract and insert data into the socket buffers
    /* virtual */ uint16_t DHCPClient::SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
    {
        uint16_t result = 0;

        if (_state == SENDING) {
            _state = RECEIVING;
            TRACE_L1("Sending DHCP message type: %d for interface: %s", _modus, _interfaceName.c_str());

            result = Message(dataFrame, maxSendSize);
        }

        return (result);
    }

    /* virtual */ uint16_t DHCPClient::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
    {
        ProcessMessage(SocketDatagram::ReceivedNode(), dataFrame, receivedSize);

        return (receivedSize);
    }

    // Signal a state change, Opened, Closed or Accepted
    /* virtual */ void DHCPClient::StateChange()
    {
    }
}
} // namespace WPEFramework::Plugin
