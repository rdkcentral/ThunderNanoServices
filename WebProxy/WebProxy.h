#ifndef __PLUGINWEBPROXY_H
#define __PLUGINWEBPROXY_H

#include "Module.h"

namespace WPEFramework {
	namespace Plugin {

		class WebProxy : public PluginHost::IPluginExtended, public PluginHost::IChannel {
		private:
			WebProxy(const WebProxy&) = delete;
			WebProxy& operator=(const WebProxy&) = delete;

		public:
			class Connector {
			private:
				Connector(const Connector&) = delete;
				Connector& operator=(const Connector&) = delete;

			public:
				Connector(PluginHost::Channel& channel, Core::IStream* link)
					: _link(link)
					, _channel(&channel)
					, _adminLock()
					, _channelBuffer()
					, _socketBuffer()
				{
				}
				virtual ~Connector()
				{
				}

			public:
				inline uint32_t Id() const
				{
					uint32_t result = 0;
					_adminLock.Lock();

					if (_channel != nullptr) {
						result = _channel->Id();
					}

					_adminLock.Unlock();

					return (result);
				}
				inline string RemoteId() const
				{
					return (_link->RemoteId());
				}
				inline bool IsClosed() const
				{
					return ((_channel == nullptr) && (_link->IsClosed()));
				}
				// Methods to extract and insert data into the socket buffers
				uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
				{
					_adminLock.Lock();

					uint16_t result = _socketBuffer.Read(dataFrame, maxSendSize);

					_adminLock.Unlock();

					return (result);
				}

				uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
				{
					_adminLock.Lock();

					bool wasEmpty = _channelBuffer.IsEmpty();

					uint16_t result = _channelBuffer.Write(dataFrame, receivedSize);

					if (wasEmpty == true) {
						// This is new data, there was nothing pending, trigger a request for a frambuffer.
						_channel->RequestOutbound();
					}

					_adminLock.Unlock();

					return (result);
				}

				uint16_t ChannelSend(uint8_t* dataFrame, const uint16_t maxSendSize) const
				{
					_adminLock.Lock();

					uint16_t result = _channelBuffer.Read(dataFrame, maxSendSize);

					_adminLock.Unlock();

					return (result);
				}

				uint16_t ChannelReceive(const uint8_t* dataFrame, const uint16_t receivedSize)
				{
					_adminLock.Lock();

					bool wasEmpty = _socketBuffer.IsEmpty();

					uint16_t result = _socketBuffer.Write(dataFrame, receivedSize);

					if (wasEmpty == true) {
						// This is new data, there was nothing pending, trigger a request for a frambuffer.
						_link->Trigger();
					}

					_adminLock.Unlock();

					return (result);
				}

				// Signal a state change, Opened, Closed or Accepted
				void StateChange()
				{
					if (_link->IsOpen() == true) {
						TRACE(Trace::Information, (_T("Proxy connection for channel ID [%d] is Open"), Id()));
					}
					else if (IsClosed() == true) {
						TRACE(Trace::Information, (_T("Proxy connection for channel ID [%d] is Closed"), Id()));
					}
					else {
						TRACE(Trace::Information, (_T("Proxy connection for channel ID [%d] has reached an exceptional state"), Id()));
					}
				}

				inline void Attach()
				{
					_adminLock.Lock();
					_link->Open(0);
					_adminLock.Unlock();
				}

				inline void Detach()
				{
					_adminLock.Lock();
					_channel = nullptr;
					_link->Close(0);
					_adminLock.Unlock();
				}

			private:
				Core::IStream* _link;
				PluginHost::Channel* _channel;
				mutable Core::CriticalSection _adminLock;
				Core::CyclicDataBuffer<Core::ScopedStorage<8192> > _channelBuffer;
				Core::CyclicDataBuffer<Core::ScopedStorage<8192> > _socketBuffer;
			};
			class Config : public Core::JSON::Container {
			public:
				class Link : public Core::JSON::Container {
				private:
					Link& operator=(const Link&);

				public:
					enum enumType {
						TCP,
						UDP,
						SERIAL
					};

					class Settings : public Core::JSON::Container {
					public:
						Settings()
							: Core::JSON::Container()
						{
							Add(_T("baudrate"), &Baudrate);
							Add(_T("parity"), &Parity);
							Add(_T("data"), &Data);
							Add(_T("stop"), &Stop);
						}
						Settings(const uint32_t baudRate, const Core::SerialPort::Parity parity, const uint8_t bits, const uint8_t stopbits)
							: Core::JSON::Container()
						{
							Add(_T("baudrate"), &Baudrate);
							Add(_T("parity"), &Parity);
							Add(_T("data"), &Data);
							Add(_T("stop"), &Stop);

							Baudrate = baudRate;
							Parity = parity;
							Data = bits;
							Stop = stopbits;
						}
						Settings(const Settings& copy)
							: Core::JSON::Container()
							, Baudrate(copy.Baudrate)
							, Parity(copy.Parity)
							, Data(copy.Data)
							, Stop(copy.Stop)
						{
							Add(_T("baudrate"), &Baudrate);
							Add(_T("parity"), &Parity);
							Add(_T("data"), &Data);
							Add(_T("stop"), &Stop);
						}
						~Settings()
						{
						}

						Settings& operator=(const Settings& rhs)
						{
							Baudrate = rhs.Baudrate;
							Parity = rhs.Parity;
							Data = rhs.Data;
							Stop = rhs.Stop;

							return (*this);
						}

					public:
						Core::JSON::DecUInt32								Baudrate;
						Core::JSON::EnumType<Core::SerialPort::Parity>	Parity;
						Core::JSON::DecUInt8								Data;
						Core::JSON::DecUInt8								Stop;
					};

				public:
					Link()
						: Core::JSON::Container()
					{
						Add(_T("name"), &Name);
						Add(_T("type"), &Type);
						Add(_T("text"), &Text);
						Add(_T("host"), &Host);
						Add(_T("device"), &Device);
						Add(_T("configuration"), &Configuration);
					}
					Link(const string& name, const enumType type, const bool text, const string host)
						: Core::JSON::Container()
					{
						Add(_T("name"), &Name);
						Add(_T("type"), &Type);
						Add(_T("text"), &Text);
						Add(_T("host"), &Host);
						Add(_T("device"), &Device);
						Add(_T("configuration"), &Configuration);

						Name = name;
						Type = type;
						Host = host;

						if (text == true) {
							Text = text;
						}
					}
					Link(const string& name, const enumType type, const bool text, const string device, const uint32_t baudRate, const Core::SerialPort::Parity parity, const uint8_t bits, const uint8_t stopbits)
						: Core::JSON::Container()
					{
						Add(_T("name"), &Name);
						Add(_T("type"), &Type);
						Add(_T("text"), &Text);
						Add(_T("host"), &Host);
						Add(_T("device"), &Device);
						Add(_T("configuration"), &Configuration);

						Name = name;
						Type = type;
						Device = device;
						Configuration = Settings(baudRate, parity, bits, stopbits);

						if (text == true) {
							Text = text;
						}
					}
					Link(const Link& copy)
						: Core::JSON::Container()
						, Name(copy.Name)
						, Type(copy.Type)
						, Text(copy.Text)
						, Host(copy.Host)
						, Device(copy.Device)
						, Configuration(copy.Configuration)
					{
						Add(_T("name"), &Name);
						Add(_T("type"), &Type);
						Add(_T("text"), &Text);
						Add(_T("host"), &Host);
						Add(_T("device"), &Device);
						Add(_T("configuration"), &Configuration);
					}
					~Link()
					{
					}

				public:
					Core::JSON::String              Name;
					Core::JSON::EnumType<enumType>  Type;
					Core::JSON::Boolean             Text;
					Core::JSON::String              Host;
					Core::JSON::String              Device;
					Settings                            Configuration;
				};

			private:
				Config(const Config&);
				Config& operator=(const Config&);

			public:
				Config()
					: Core::JSON::Container()
					, Connections(10)
				{
					Add(_T("connections"), &Connections);
					Add(_T("links"), &Links);
				}
				~Config()
				{
				}

			public:
				Core::JSON::DecUInt16 Connections;
				Core::JSON::ArrayType<Link> Links;
			};

		public:
			WebProxy()
				: _connectionMap()
			{
			}
			virtual ~WebProxy()
			{
			}

			BEGIN_INTERFACE_MAP(WebProxy)
				INTERFACE_ENTRY(PluginHost::IPlugin)
				INTERFACE_ENTRY(PluginHost::IPluginExtended)
				INTERFACE_ENTRY(PluginHost::IChannel)
			END_INTERFACE_MAP

		public:
			//	IPluginExtended methods
			// -------------------------------------------------------------------------------------------------------

			// First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
			// information and services for this particular plugin. The Service object contains configuration information that
			// can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
			// If there is an error, return a string describing the issue why the initialisation failed.
			// The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
			// The lifetime of the Service object is guaranteed till the deinitialize method is called.
			virtual const string Initialize(PluginHost::IShell* service);

			// The plugin is unloaded from WPEFramework. This is call allows the module to notify clients
			// or to persist information if needed. After this call the plugin will unlink from the service path
			// and be deactivated. The Service object is the same as passed in during the Initialize.
			// After theis call, the lifetime of the Service object ends.
			virtual void Deinitialize(PluginHost::IShell* service);

			// Whenever a Channel (WebSocket connection) is created to the plugin that will be reported via the Attach.
			// Whenever the channel is closed, it is reported via the detach method.
			virtual bool Attach(PluginHost::Channel& channel);
			virtual void Detach(PluginHost::Channel& channel);

			// Returns an interface to a JSON struct that can be used to return specific metadata information with respect
			// to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
			virtual string Information() const;

			//	IChannel methods
			// -------------------------------------------------------------------------------------------------------
			// Whenever a WebSocket is opened with a locator (URL) pointing to this plugin, it is capable of receiving
			// raw data for the plugin. Raw data received on this link will be exposed to the plugin via this interface.
			virtual uint32_t Inbound(const uint32_t ID, const uint8_t data[], const uint16_t length);

			// Whenever a WebSocket is opened with a locator (URL) pointing to this plugin, it is capable of sending
			// raw data to the initiator of the websocket.
			virtual uint32_t Outbound(const uint32_t ID, uint8_t data[], const uint16_t length) const;

		private:
			Connector* CreateConnector(PluginHost::Channel& channel) const;

		private:
			string _prefix;
			uint32_t _maxConnections;
			std::map<const uint32_t, Connector*> _connectionMap;
			std::map<const string, Config::Link> _linkInfo;
		};
	}
}

#endif // __PLUGINWEBPROXY_H
