# Remote Invocation Tutorial

Remote host invocation can be used to access an out-of-process nanoservice from remote device as it was running on local device. 

## High level overview
![Remote invocation diagram](remote_invocation_diagram.png?raw=true "Remote invocation diagram")

## Host device
First thing you need to do is to enable and start RemoteInvocation plugin on the device, on which the plugin will run. You should also specify address on which the plugin will be listening for proxy requests. You do that by configuring "address" in RemoteInvocation configuration. Eg. to listen for all incoming requests on port 5797 you could use following configuration:
```json
{
 "locator":"libWPEFrameworkRemoteInvocation.so",
 "classname":"RemoteInvocation",
 "autostart":true,
 "configuration":{
  "address":"0.0.0.0:5797",
 }
}
```

From code side of view, the only thing you need to do, to allow access to plugin from remote device, is to implement IRemoteLinker interface in your plugin. The easiest way to do it is to just inherit from default implementation - RPC::RemoteLinker

```cpp
class RemoteHostExampleImpl : public RPC::RemoteLinker, IRemoteHostExample {
public:

    // plugin functions
    // [...]

    BEGIN_INTERFACE_MAP(RemoteHostExampleImpl)
        INTERFACE_ENTRY(Exchange::IRemoteHostExample)
        INTERFACE_ENTRY(RPC::IRemoteLinker)
    END_INTERFACE_MAP
};
```

Thats it! Now start a plugin just as you would for a typical out-of-process

```cpp
_implementation = service->Root<Exchange::IRemoteHostExample>(connectionId, timeout, "RemoteHostExampleImpl", ~0);
```
with example config of
```json
{
 "locator":"libWPEFrameworkRemoteHostExample.so",
 "classname":"RemoteHostExample",
 "autostart":false,
 "configuration":{
  "name":"Local",
  "root":{
   "mode":"local"
  }
 }
}
```

## Proxy device
On device that will use proxy, root call looks exactly the same:
```cpp
_implementation = service->Root<Exchange::IRemoteHostExample>(connectionId, timeout, "RemoteHostExampleImpl", ~0);
```
the only difference is in config. It will look like this:
```json
{
 "locator":"libWPEFrameworkRemoteHostExample.so",
 "classname":"RemoteHostExample",
 "autostart":false,
 "configuration":{
  "name":"Remote",
  "root":{
   "mode":"distributed",
   "remoteaddress":"10.5.0.112:5797"
  }
 }
}
```
One thing to remember is the fact that this root only creates a proxy. Plugin have to be already running on target device!

## Best practices

Although its not strictly required, all functions that will be called from remote device should return an uin32_t type. This is because every call could potentially fail because of network problems (lost connections, weak signal etc...). By specifying uint32_t as return type, appropriate error will be returned. If function would like to return other type, it should do it by reference parameters. 

Eg. by having
```cpp
virtual uint32_t Greet(const string& message, string& response /* @out */) = 0;
```

We could check that everyting is correct by following:
```cpp
string response;
uint32_t result = _implementation->Greet("Hello", response);

if (result == Core::ERROR_TIMEDOUT) {
    TRACE(Trace::Error, (_T("Call to _implementation->Greet(...) failed. Conneciton timed out")));
} else if (result == Core::ERROR_CONNECTION_CLOSED) {
    TRACE(Trace::Error, (_T("Call to _implementation->Greet(...) failed. Connection to remote device was closed")));
} else if (result != Core::ERROR_NONE) {
    TRACE(Trace::Error, (_T("Call to _implementation->Greet(...) failed. Unknown error")));
}
```

## Communicating between 32 bit & 64bit devices
If you want to communicate between devices with different bitness, you should define COMRPC_POINTER_LENGTH flag with highest bitness of devices that will take part in communication. Eg. if you would like to communicate between 64 and 32 bit devices via comrpc, this should be set to 64 as the lowest value that will fil memory addresses. When you have only 32 bit devices it should be set to 32
