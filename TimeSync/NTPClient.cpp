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

#include "NTPClient.h"
#include <stdio.h>

namespace WPEFramework {
namespace Plugin {

    constexpr uint32_t WaitForResponse = 2000;

#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
    NTPClient::NTPClient()
        : Core::SocketDatagram(false, Core::NodeId(), Core::NodeId(), 128, 512)
        , _adminLock()
        , _packet()
        , _syncedTimestamp()
        , _state(INITIAL)
        , _fired(true)
        , _WaitForNetwork(2000) // Wait for 2 Seconds for a new attempt
        , _retryAttempts(5)
        , _activity(Core::ProxyType<Activity>::Create(this))
        , _clients()
    {
        _packet.LeapIndicator(0x03); // Unknown
        _packet.NTPVersion(0x04); // Version 4
        _packet.NTPMode(0x03); // NTP Client
        _packet.Stratum(0); // Unspecified
        _packet.Poll(3); // 2^3 = 8 seconds poll interval
        _packet.Precision(0xFA); // 2^-6 = 1/64 second precision
        _packet.RootDelay(0x00010000); // Insignificant, 1 second
        _packet.RootDispersion(0x00010000); // Insignificant, 1 second
        _packet.ReferenceID(0x00000000);
    }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif

    /* virtual */ NTPClient::~NTPClient()
    {
        Core::IWorkerPool::Instance().Revoke(_activity);

        Close(Core::infinite);
    }

    void NTPClient::Initialize(SourceIterator& sources, const uint16_t retries, const uint16_t delay)
    {
        _retryAttempts = retries;
        _WaitForNetwork = (delay * 1000); /* in ms */
        _servers.clear();

        while (sources.Next() == true) {
            Core::URL url(sources.Current().Value());

            if (url.Type() == Core::URL::SCHEME_NTP) {

                string hostname(url.Host().Value());

                if (url.Port().IsSet() == true) {

                    hostname += ':' + Core::NumberType<uint16_t>(url.Port().Value()).Text();
                }
                else {
                    hostname += ':' + Core::NumberType<uint16_t>(Core::URL::Port(url.Type())).Text();
                }

                _servers.push_back(hostname);
            }
        }

        _serverIndex = ServerIterator(_servers);
    }

    /* virtual */ uint32_t NTPClient::Synchronize()
    {
        uint32_t result = Core::ERROR_INCOMPLETE_CONFIG;

        TRACE(Trace::Information, (_T("TimeSync: Synchronize")));

        _adminLock.Lock();

        if ((_state == INITIAL) || (_state == SUCCESS) || ((_state == FAILED) && (_serverIndex.IsValid() == false))) {
            result = Core::ERROR_NONE;
            _state = SENDREQUEST;
            Core::IWorkerPool::Instance().Submit(_activity);
        } else if (_state == SENDREQUEST || _state == INPROGRESS) {
            result = Core::ERROR_INPROGRESS;
        }

        _adminLock.Unlock();

        return (result);
    }

    /* virtual */ void NTPClient::Cancel()
    {
        _adminLock.Lock();

        if ((_state != INITIAL) && (_state != FAILED) && (_state != SUCCESS)) {

            if (!IsClosed()) {
                TRACE(Trace::Information, (_T("TimeSync: %s"), "Cancelling, Closing socket"));
                Close(0);
            }

            _state = FAILED;

            Core::IWorkerPool::Instance().Revoke(_activity);
            Core::IWorkerPool::Instance().Submit(_activity);
        }
        _adminLock.Unlock();
    }

    /* virtual */ uint64_t NTPClient::SyncTime() const
    {
        return (_syncedTimestamp.IsValid() ? _syncedTimestamp.Ticks() : 0);
    }

    /* virtual */ string NTPClient::Source() const
    {
        return (_serverIndex.IsValid() == true ? string(_T("NTP://")) + (*_serverIndex) + '/' : _T("NTP:///"));
    }

    /* virtual */ void NTPClient::Register(Exchange::ITimeSync::INotification* notification)
    {
        _adminLock.Lock();

        ASSERT(std::find(_clients.begin(), _clients.end(), notification) == _clients.end());

        notification->AddRef();
        _clients.push_back(notification);

        _adminLock.Unlock();
    }

    /* virtual */ void NTPClient::Unregister(Exchange::ITimeSync::INotification* notification)
    {

        _adminLock.Lock();

        std::list<Exchange::ITimeSync::INotification*>::iterator index(std::find(_clients.begin(), _clients.end(), notification));

        ASSERT(index != _clients.end());

        if (index != _clients.end()) {
            notification->Release();
            _clients.erase(index);
        }

        _adminLock.Unlock();
    }

    /* virtual */ uint16_t NTPClient::SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
    {
        uint16_t result = 0;

        _adminLock.Lock();

        if (_fired == false) {

            _fired = true;

            DataFrame newFrame(dataFrame, maxSendSize);
            DataFrame::Writer writer(newFrame, 0);
            _packet.TransmitTimestamp(NTPPacket::Timestamp(Core::Time::Now()));
            _packet.Serialize(writer);

            result = newFrame.Size();
            TRACE(Trace::Information, (_T("Timesync: Send data: %d bytes"), result));
        }

        _adminLock.Unlock();

        return result;
    }

    inline static uint64_t SecondsToTicks(double seconds)
    {
        return static_cast<uint64_t>(seconds * NTPClient::MicroSeconds);
    }

    /* virtual */ uint16_t NTPClient::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
    {
        double received = static_cast<double>(Core::Time::Now().Ticks() / MicroSeconds);

        TRACE(Trace::Information, (_T("Timesync: Received data: %d bytes"), receivedSize));

        _adminLock.Lock();

        if (receivedSize == NTPPacket::PacketSize) {

            DataFrame frame(dataFrame, receivedSize, receivedSize);
            NTPPacket packet;
            packet.Deserialize(DataFrame::Reader(frame, 0));

#ifdef __DEBUG__
// packet.DisplayPacket();
#endif

            double receivedServerTS = packet.ReceiveTimestamp().TimeSeconds();
            double sentServerTS = packet.TransmitTimestamp().TimeSeconds();
            double sentTS = packet.OriginalTimestamp().TimeSeconds();

            double diffRequest = receivedServerTS - sentTS;
            double diffResponse = sentServerTS - received;
            double offset = (diffRequest + diffResponse) / 2;

#ifdef __DEBUG__
            double elapsedServer = sentServerTS - receivedServerTS;
            double elapsedTotal = received - sentTS;
            double roundTrip = elapsedTotal - elapsedServer;
#endif

            TRACE(Trace::Information, (_T("Request time diff  = %lf"), diffRequest));
            TRACE(Trace::Information, (_T("Response time diff = %lf"), diffResponse));
            TRACE(Trace::Information, (_T("Offset time        = %lf"), offset));

#ifdef __DEBUG__
            TRACE(Trace::Information, (_T("Server elapsed time = %lf"), elapsedServer));
            TRACE(Trace::Information, (_T("Total elapsed time  = %lf"), elapsedTotal));
            TRACE(Trace::Information, (_T("Round trip time     = %lf"), roundTrip));
#endif
            TRACE(Trace::Information, (_T("TimeSync: Request time diff   = %lf s"), diffRequest));
            TRACE(Trace::Information, (_T("TimeSync: Response time diff  = %lf s"), diffResponse));
            TRACE(Trace::Information, (_T("TimeSync: Offset time         = %lf s"), offset));

            uint64_t receivedTicks = SecondsToTicks(received);
            TRACE(Trace::Information, (_T("TimeSync: Current time: %s"), Core::Time(receivedTicks).ToRFC1123(false).c_str()));
            _syncedTimestamp = Core::Time(receivedTicks + SecondsToTicks(offset));
            TRACE(Trace::Information, (_T("TimeSync: New time:     %s"), _syncedTimestamp.ToRFC1123(false).c_str()));

            _state = SUCCESS;

            // We don't need the socket anymore, so close it
            TRACE(Trace::Information, (_T("TimeSync: %s"), "Closing socket, no longer needed"));
            Close(0);

            // Lets remove the watchdog subject, we do not want to wait anymore, we got an answer.
            Core::IWorkerPool::Instance().Revoke(_activity);

            // Schedule it again to broadcast the successfull update of the time.
            Core::IWorkerPool::Instance().Submit(_activity);
        }

        _adminLock.Unlock();

        return (receivedSize);
    }

    // Signal a state change, Opened, Closed or Accepted
    /* virtual */ void NTPClient::StateChange()
    {
        if (HasError() == true) {
            Close(0);
            Core::IWorkerPool::Instance().Revoke(_activity);
            Core::IWorkerPool::Instance().Submit(_activity);
        }
    }

    bool NTPClient::FireRequest()
    {

        bool activated = false;

        // Make sure socket is closed otherwise an assert will fire.
        if (!IsClosed()) {
            TRACE(Trace::Information, (_T("Lingering socket, closing")));
            Close(1000);
        }

        if (true == IsClosed() && (_serverIndex.Count() > 0)) {

            uint32_t lastonetried = _serverIndex.Index();

            do {

                // next server
                if( _serverIndex.Index() >= _serverIndex.Count() ) {
                    _serverIndex.Reset(0);
                }
                _serverIndex.Next();
                ASSERT(_serverIndex.IsValid());

                TRACE(Trace::Information, (_T("Trying NTP Server: [%s]"), (*_serverIndex).c_str()));

                // Set the socket to send to the remote
                Core::NodeId remote((*_serverIndex).c_str(), Core::NodeId::TYPE_IPV4);

                if (remote.IsValid() == true) {
                    // Set the remote endpoint for the socket to the selected NTP server
                    RemoteNode(remote);
                    LocalNode(remote.AnyInterface());

                    // UDP should open by definition directly...
                    uint32_t status = Open(100);

                    if ((status == Core::ERROR_NONE) || (status == Core::ERROR_INPROGRESS)) {

                        activated = true;
                        _fired = false;
                        Trigger();
                    }
                    else {
                        TRACE(Trace::Warning, (_T("Could not open connection to NTP Server [%s]"), (*_serverIndex).c_str()));
                    }
                }
                else {
                    TRACE(Trace::Warning, (_T("Could not resolve NTP Server [%s]"), (*_serverIndex).c_str()));
                }
            }
            while ((activated == false) && ( ((lastonetried != 0) && (lastonetried != _serverIndex.Index())) ||
                                             ((lastonetried == 0) && (_serverIndex.Index() != _serverIndex.Count())) 
                                           ) );
        }

        return (activated);
    }

    void NTPClient::Update()
    {

        // runs always in the context of the adminlock

        if(_state == FAILED){
            TRACE(Trace::Error, (_T("Could not determine a valid time")));
        }

        std::list<Exchange::ITimeSync::INotification*>::iterator index(_clients.begin());

        while (index != _clients.end()) {
            (*index)->Completed();
            index++;
        }

    }

    void NTPClient::Dispatch()
    {
        uint32_t result = Core::infinite;

        _adminLock.Lock();

        switch (_state) {
        case SENDREQUEST: {
            // This case means that nothing has started yet, let reset the list of servers and start at the beginning...
            _serverIndex.Reset(0);
            _state = INPROGRESS;
            _currentAttempt = _retryAttempts;
        }
        case INPROGRESS: {
            // If we end up here in this state, it means that the package was send but no response was received,
            // or a response was received but is was not properly formatted...
            // Lets move to the next server in the list, see if that one responds correctly, if we tried all servers let's
            // start at the first one again, this time it might work
            if (FireRequest() == true) {
              result = WaitForResponse;
            } else {
                if (_currentAttempt-- != 0) {

                    // Looks like there is no network connectivity, Just sleep and retry later
                    result = _WaitForNetwork;
                } else {

                    // Looks like there is no valid server anymore that we could use. There where servers
                    // that did resolve correctly, so it is not a network connectivity problem.
                    _state = FAILED;

                    // Report the failure. Always report back when we are finished.
                    Update();
                }
            }
            break;
        }
        case FAILED:
        case SUCCESS: {
            Update();
            break;
        }
        default:
            // New state, probably needs some action???
       ASSERT(false);
        }

        _adminLock.Unlock();

        // See if we need rescheduling
        if (result != Core::infinite) {
            Core::Time timestamp(Core::Time::Now());
            timestamp.Add(result);
            Core::IWorkerPool::Instance().Schedule(timestamp, _activity);
        }
   }

} // namespace Plugin
} // namespace WPEFramework
