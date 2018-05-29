#include "WebProxy.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Plugin::WebProxy::Config::Link::enumType)
    { Plugin::WebProxy::Config::Link::UDP,    _TXT("udp")    },
    { Plugin::WebProxy::Config::Link::TCP,    _TXT("tcp")    },
    { Plugin::WebProxy::Config::Link::SERIAL, _TXT("serial") },
ENUM_CONVERSION_END(Plugin::WebProxy::Config::Link::enumType);

ENUM_CONVERSION_BEGIN(Core::SerialPort::Parity)
    { Core::SerialPort::EVEN, _TXT("even") },
    { Core::SerialPort::ODD,  _TXT("odd")  },
    { Core::SerialPort::NONE, _TXT("none") },
ENUM_CONVERSION_END(Core::SerialPort::Parity);

	namespace Plugin {

		class StreamChannel : public Core::StreamType<Core::SocketStream> {
		private:
			StreamChannel() = delete;
			StreamChannel(const StreamChannel&) = delete;
			StreamChannel& operator=(const StreamChannel&) = delete;

		public:
			StreamChannel(WebProxy::Connector& parent, const uint32_t bufferSize, const Core::NodeId& remote) :
				Core::StreamType< Core::SocketStream >(false, remote.AnyInterface(), remote, bufferSize, bufferSize),
				_parent(parent) {
			}
			virtual ~StreamChannel() {
			}

		public:
			virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
			{
				return (_parent.SendData(dataFrame, maxSendSize));
			}

			virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
			{
				return (_parent.ReceiveData(dataFrame, receivedSize));
			}
			virtual void StateChange()
			{
				return (_parent.StateChange());
			}

		private:
			WebProxy::Connector& _parent;
		};

		class DatagramChannel : public Core::StreamType<Core::SocketDatagram> {
		private:
			DatagramChannel();
			DatagramChannel(const DatagramChannel&);
			DatagramChannel& operator=(const DatagramChannel&);

			typedef Core::StreamType<Core::SocketDatagram> BaseClass;

		public:
			DatagramChannel(WebProxy::Connector& parent, const uint32_t bufferSize, const Core::NodeId& remote) :
				Core::StreamType< Core::SocketDatagram >(false, remote.AnyInterface(), remote, bufferSize, bufferSize),
				_parent(parent) {
			}
			virtual ~DatagramChannel() {
			}

		public:
			virtual uint32_t Open(const uint32_t waitTime)
			{
				uint32_t result = BaseClass::Open(waitTime);

				if (BaseClass::RemoteNode().IsMulticast() == true) {
					BaseClass::Join(BaseClass::RemoteNode());
				}

				return (result);
			}
			virtual uint32_t Close(const uint32_t waitTime)
			{
				if (BaseClass::RemoteNode().IsMulticast() == true) {
					BaseClass::Leave(BaseClass::RemoteNode());
				}

				return (BaseClass::Close(waitTime));
			}
			virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
			{
				return (_parent.SendData(dataFrame, maxSendSize));
			}

			virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
			{
				return (_parent.ReceiveData(dataFrame, receivedSize));
			}
			virtual void StateChange()
			{
				return (_parent.StateChange());
			}

		private:
			WebProxy::Connector& _parent;
		};

		class DeviceChannel : public Core::StreamType<Core::SerialPort> {
		private:
			DeviceChannel();
			DeviceChannel(const DeviceChannel&);
			DeviceChannel& operator=(const DeviceChannel&);

		public:
			DeviceChannel(
				WebProxy::Connector& parent,
				const uint32_t bufferSize,
				const string& deviceName,
				const Core::SerialPort::BaudRate baudrate,
				const Core::SerialPort::Parity parityE,
				const Core::SerialPort::DataBits dataBits,
				const Core::SerialPort::StopBits stopBits)
				: Core::StreamType<Core::SerialPort>(deviceName, baudrate, parityE, dataBits, stopBits, bufferSize, bufferSize)
				, _parent(parent)
			{
			}
			virtual ~DeviceChannel()
			{
			}

		public:
			virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
			{
				return (_parent.SendData(dataFrame, maxSendSize));
			}

			virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
			{
				return (_parent.ReceiveData(dataFrame, receivedSize));
			}
			virtual void StateChange()
			{
				return (_parent.StateChange());
			}

		private:
			WebProxy::Connector& _parent;
		};

		template <typename STREAMTYPE>
		class ConnectorWrapper : public WebProxy::Connector {
		private:
			ConnectorWrapper() = delete;
			ConnectorWrapper(const ConnectorWrapper<STREAMTYPE>&) = delete;
			ConnectorWrapper<STREAMTYPE>& operator=(const ConnectorWrapper<STREAMTYPE>&) = delete;

		public:
			#ifdef __WIN32__ 
			#pragma warning( disable : 4355 )
			#endif
			inline ConnectorWrapper(PluginHost::Channel& channel, const uint32_t bufferSize)
				: WebProxy::Connector(channel, &_streamType)
				, _streamType(*this, bufferSize)
			{
			}
			inline ConnectorWrapper(PluginHost::Channel& channel, const uint32_t bufferSize, const Core::NodeId& remoteId)
				: WebProxy::Connector(channel, &_streamType)
				, _streamType(*this, bufferSize, remoteId)
			{
			}
			inline ConnectorWrapper(
				PluginHost::Channel& channel,
				const uint32_t bufferSize,
				const string& deviceName,
				const Core::SerialPort::BaudRate baudrate,
				const Core::SerialPort::Parity parityE,
				const Core::SerialPort::DataBits dataBits,
				const Core::SerialPort::StopBits stopBits)
				: WebProxy::Connector(channel, &_streamType)
				, _streamType(*this, bufferSize, deviceName, baudrate, parityE, dataBits, stopBits)
			{
			}
			#ifdef __WIN32__ 
			#pragma warning( default : 4355 )
			#endif
			virtual ~ConnectorWrapper()
			{
			}

		public:
			inline STREAMTYPE& Stream()
			{
				return (_streamType);
			}

		private:
			STREAMTYPE _streamType;
		};

		SERVICE_REGISTRATION(WebProxy, 1, 0);

		/* virtual */ const string WebProxy::Initialize(PluginHost::IShell* service)
		{
			WebProxy::Config config;
			config.FromString(service->ConfigLine());

			_maxConnections = config.Connections.Value();

			// Copy all predefined links...
			if ((config.Links.IsSet() == true) && (config.Links.Length() != 0)) {
				// Build the device info array..
				Core::JSON::ArrayType<Config::Link>::Iterator index(config.Links.Elements());

				while (index.Next() == true) {
					if (index.Current().Name.IsSet() == true) {
						_linkInfo.insert(std::pair<const string, Config::Link>(index.Current().Name.Value(), index.Current()));
					}
				}
			}

			service->EnableWebServer(_T("UI"), EMPTY_STRING);

			// On succes return nullptr, to indicate there is no error text.
			return (_T(""));
		}

		/* virtual */ void WebProxy::Deinitialize(PluginHost::IShell* service)
		{

			service->DisableWebServer();
		}

		// Whenever a Channel (WebSocket connection) is created to the plugin that will be reported via the Attach.
		// Whenever the channel is closed, it is reported via the detach method.
		/* virtual */ bool WebProxy::Attach(PluginHost::Channel& channel)
		{
			bool added = false;
			Core::NodeId nodeId;

			// First do a cleanup of all "completely" closed channels.
			std::map<const uint32_t, Connector*>::iterator connection(_connectionMap.begin());

			while (connection != _connectionMap.end()) {
				if (connection->second->IsClosed() == true) {
					delete connection->second;
					connection = _connectionMap.erase(connection);
				}
				else {
					connection++;
				}
			}

			// See if we are still allowed to create a new connection..
			if (_connectionMap.size() < _maxConnections) {
				Connector* newLink = CreateConnector(channel);

					if (newLink != nullptr) {
						_connectionMap.insert(std::pair<uint32_t, Connector*>(channel.Id(), newLink));
						TRACE(Trace::Information, (Trace::Format(_T("Proxy connection channel ID [%d] to %s"), channel.Id(), newLink->RemoteId().c_str()).c_str()));
						added = true;

									newLink->Attach();
							}
					}

				return (added);
			}

			/* virtual */ void WebProxy::Detach(PluginHost::Channel& channel)
			{
				// See if we can forward this info..
				std::map<const uint32_t, Connector*>::iterator connection = _connectionMap.find(channel.Id());

				if (connection != _connectionMap.end()) {
					connection->second->Detach();
				}
			}

			/* virtual */ string WebProxy::Information() const
			{
				// No additional info to report.
				return (string());
			}

			// IChannel methods
			// -------------------------------------------------------------------------------------------------------
			// Whenever a WebSocket is opened with a locator (URL) pointing to this plugin, it is capable of receiving
			// raw data for the plugin. Raw data received on this link will be exposed to the plugin via this interface.
			/* virtual */ uint32_t WebProxy::Inbound(const uint32_t ID, const uint8_t data[], const uint16_t length)
			{
				uint32_t result = length;

				// See if we can forward this info..
				std::map<const uint32_t, Connector*>::iterator connection = _connectionMap.find(ID);

				if (connection != _connectionMap.end()) {
					result = connection->second->ChannelReceive(data, length);
				}

				return (result);
			}

			// Whenever a WebSocket is opened with a locator (URL) pointing to this plugin, it is capable of sending
			// raw data to the initiator of the websocket.
			/* virtual */ uint32_t WebProxy::Outbound(const uint32_t ID, uint8_t data[], const uint16_t length) const
			{
				uint32_t result = 0;

				// See if we can forward this info..
				std::map<const uint32_t, Connector*>::const_iterator connection = _connectionMap.find(ID);

				if (connection != _connectionMap.end()) {
					result = connection->second->ChannelSend(data, length);
				}

				return (result);
			}

			WebProxy::Connector* WebProxy::CreateConnector(PluginHost::Channel& channel) const
			{
				Core::TextFragment host;
				Core::TextFragment device;
				WebProxy::Connector* result(nullptr);
				Core::SerialPort::BaudRate baudRate(Core::SerialPort::BaudRate::BAUDRATE_115200);
				Core::SerialPort::Parity parity(Core::SerialPort::NONE);
				Core::SerialPort::DataBits dataBits(Core::SerialPort::DataBits::BITS_8);
				Core::SerialPort::StopBits stopBits(Core::SerialPort::StopBits::BITS_1);
				const string& options(channel.Query());
				bool datagram(false);
				bool text(false);

				if (options.empty() == false) {
					Core::TextSegmentIterator index(Core::TextFragment(options), true, '&');

					while (index.Next() == true) {
						Core::TextSegmentIterator section(index.Current(), true, '=');

						if (section.Next() == true) {
							if ((section.Current() == _T("host")) && (section.Next() == true)) {
								host = section.Current();
							}
							else if ((section.Current() == _T("datagram")) && (section.Next() == false)) {
								datagram = true;
							}
							else if ((section.Current() == _T("text")) && (section.Next() == false)) {
								text = true;
							}
							else if ((section.Current() == _T("device")) && (section.Next() == true)) {
								device = section.Current();
							}
						}
					}
				}
				else if (channel.Name().empty() == false) {
					// See of this name is registered ?
					std::map<const string, Config::Link>::const_iterator index(_linkInfo.find(channel.Name()));

					if (index != _linkInfo.end()) {
						const Config::Link& linkInfo(index->second);

						// Seems like we have a valid entry, get the config.
						text = ((linkInfo.Text.IsSet() == true) && (linkInfo.Text.Value() == true));
						host = Core::TextFragment(linkInfo.Host.Value());
						device = Core::TextFragment(linkInfo.Device.Value());
						datagram = ((linkInfo.Type.IsSet() == true) && (linkInfo.Type.Value() == Config::Link::UDP));

						if (linkInfo.Configuration.IsSet() == true) {
							const Config::Link::Settings& configInfo(linkInfo.Configuration);

							parity = (configInfo.Parity.Value());
							stopBits = (configInfo.Stop.Value() == 2 ? Core::SerialPort::StopBits::BITS_2 : Core::SerialPort::StopBits::BITS_1);
							baudRate = (configInfo.Data.Value() == 110 ? Core::SerialPort::BaudRate::BAUDRATE_110 : configInfo.Data.Value() == 300 ? Core::SerialPort::BaudRate::BAUDRATE_300 : configInfo.Data.Value() == 600 ? Core::SerialPort::BaudRate::BAUDRATE_600 : configInfo.Data.Value() == 1200 ? Core::SerialPort::BaudRate::BAUDRATE_1200 : configInfo.Data.Value() == 2400 ? Core::SerialPort::BaudRate::BAUDRATE_2400 : configInfo.Data.Value() == 4800 ? Core::SerialPort::BaudRate::BAUDRATE_4800 : configInfo.Data.Value() == 9600 ? Core::SerialPort::BaudRate::BAUDRATE_9600 : configInfo.Data.Value() == 19200 ? Core::SerialPort::BaudRate::BAUDRATE_19200 : configInfo.Data.Value() == 38400 ? Core::SerialPort::BaudRate::BAUDRATE_38400 : configInfo.Data.Value() == 57600 ? Core::SerialPort::BaudRate::BAUDRATE_57600 : configInfo.Data.Value() == 115200 ? Core::SerialPort::BaudRate::BAUDRATE_115200 : Core::SerialPort::BaudRate::BAUDRATE_9600);
							dataBits = (configInfo.Data.Value() == 5 ? Core::SerialPort::DataBits::BITS_5 : configInfo.Data.Value() == 6 ? Core::SerialPort::DataBits::BITS_6 : configInfo.Data.Value() == 7 ? Core::SerialPort::DataBits::BITS_6 : Core::SerialPort::DataBits::BITS_8);
						}
					}
				}

				if ((host.Length() > 0) && (device.Length() == 0)) {
					Core::NodeId remote(host.Text().c_str());

					if (datagram == true) {
						result = new ConnectorWrapper<DatagramChannel>(channel, 1024, remote);
					}
					else {
						result = new ConnectorWrapper<StreamChannel>(channel, 1024, remote);
					}
				}
				else if ((device.Length() > 0) && (host.Length() == 0)) {
					result = new ConnectorWrapper<DeviceChannel>(channel, 1024, device.Text(), baudRate, parity, dataBits, stopBits);
				}

				if ((result != nullptr) && (text == true)) {
					channel.Binary(false);
				}

				return (result);
			}
	}
}
