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

#include "DHCPServerImplementation.h"

namespace WPEFramework {

namespace Plugin {

    /* static */ constexpr uint8_t DHCPServerImplementation::MagicCookie[];
    /* static */ constexpr uint16_t DHCPServerImplementation::Identifier::maxLength;

    uint32_t DHCPServerImplementation::Open()
    {

        uint32_t result = Core::ERROR_OPENING_FAILED;

        // get an interface with a public IP address, then we will have a proper MAC address..
        Core::NodeId source(Plugin::Config::IPV4UnicastNode(_interfaceName));
        Core::IPNode selectedNode(source, source.DefaultMask());
        if (selectedNode.Type() == Core::NodeId::TYPE_IPV4) {

            // INITIALIZE SERVER SOCKET !!!
            // Open socket and set broadcast option on it
            // saServerAddress.sin_addr.s_addr = dwServerAddr;  // Already in network byte order
            // saServerAddress.sin_port = htons((u_short)DefaultDHCPServerPort);
            printf("LocalNode on interface[%s] bind to [%s:%d]\n", _interfaceName.c_str(), SocketDatagram::LocalNode().HostAddress().c_str(), SocketDatagram::LocalNode().PortNumber());
            result = SocketDatagram::Open(Core::infinite, _interfaceName);

            if ((result == Core::ERROR_NONE) && (SocketDatagram::Broadcast(true) == false)) {
                result = Core::ERROR_BAD_REQUEST;
            } else {
                _server = static_cast<const Core::NodeId::SocketInfo&>(selectedNode).IPV4Socket.sin_addr.s_addr;

                // Now lets define the under and upper marker of the dhcp server address pool.
                uint32_t mask = (0xFFFFFFFF >> selectedNode.Mask());
                uint32_t address = ntohl(_server);

                _minAddress = ((address & (~mask)) + (_poolStart & mask));
                _maxAddress = ((address & (~mask)) + ((_poolStart + _poolSize) & mask));
                _nextFreeIp = _minAddress;

                if (_router != static_cast<uint32_t>(~0)) {
                    if (_router == 0) {
                        _router = address;
                    } else {
                        _router = ((address & (~mask)) + (_router & mask));
                    }
                }
                if (_dns == static_cast<uint32_t>(~0)) {
                    _dns = address;
                }
            }
        }

        return (result);
    }
    uint32_t DHCPServerImplementation::Close()
    {
        return (SocketDatagram::Close(Core::infinite));
    }

    /* static */ Core::ProxyPoolType<DHCPServerImplementation::Response> DHCPServerImplementation::_responseFactory(2);
}

} // Namespace WPEFramework::plugin
