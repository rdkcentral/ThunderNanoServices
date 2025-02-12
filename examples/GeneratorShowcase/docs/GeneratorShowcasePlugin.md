<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_Generator_Showcase_Plugin"></a>
# Generator Showcase Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

GeneratorShowcase plugin for Thunder framework.

### Table of Contents

- [Introduction](#head_Introduction)
- [Configuration](#head_Configuration)
- [Interfaces](#head_Interfaces)
- [Methods](#head_Methods)
- [Properties](#head_Properties)
- [Notifications](#head_Notifications)

<a id="head_Introduction"></a>
# Introduction

<a id="head_Scope"></a>
## Scope

This document describes purpose and functionality of the GeneratorShowcase plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

<a id="head_Case_Sensitivity"></a>
## Case Sensitivity

All identifiers of the interfaces described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a id="head_Acronyms,_Abbreviations_and_Terms"></a>
## Acronyms, Abbreviations and Terms

The table below provides and overview of acronyms used in this document and their definitions.

| Acronym | Description |
| :-------- | :-------- |
| <a name="acronym.API">API</a> | Application Programming Interface |
| <a name="acronym.HTTP">HTTP</a> | Hypertext Transfer Protocol |
| <a name="acronym.JSON">JSON</a> | JavaScript Object Notation; a data interchange format |
| <a name="acronym.JSON-RPC">JSON-RPC</a> | A remote procedure call protocol encoded in JSON |

The table below provides and overview of terms and abbreviations used in this document and their definitions.

| Term | Description |
| :-------- | :-------- |
| <a name="term.callsign">callsign</a> | The name given to an instance of a plugin. One plugin can be instantiated multiple times, but each instance the instance name, callsign, must be unique. |

<a id="head_References"></a>
## References

| Ref ID | Description |
| :-------- | :-------- |
| <a name="ref.HTTP">[HTTP](http://www.w3.org/Protocols)</a> | HTTP specification |
| <a name="ref.JSON-RPC">[JSON-RPC](https://www.jsonrpc.org/specification)</a> | JSON-RPC 2.0 specification |
| <a name="ref.JSON">[JSON](http://www.json.org/)</a> | JSON specification |
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20Thunder.docx)</a> | Thunder API Reference |

<a id="head_Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *GeneratorShowcase*) |
| classname | string | mandatory | Class name: *GeneratorShowcase* |
| locator | string | mandatory | Library name: *libThunderGeneratorShowcase.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- ISimpleAsync ([ISimpleAsync.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/ISimpleAsync.h)) (version 1.0.0) (compliant format)
- ISimpleInstanceObjects ([ISimpleInstanceObjects.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/ISimpleInstanceObjects.h)) (version 1.0.0) (compliant format)

<a id="head_Methods"></a>
# Methods

The following methods are provided by the GeneratorShowcase plugin:

SimpleAsync interface methods:

| Method | Description |
| :-------- | :-------- |
| [connect](#method_connect) | Connects to a server |
| [abort](#method_abort) | Aborts connecting to a server |
| [disconnect](#method_disconnect) | Disconnects from the server |

SimpleInstanceObjects interface methods:

| Method | Description |
| :-------- | :-------- |
| [acquire](#method_acquire) | Acquires a device |
| [relinquish](#method_relinquish) | Relinquishes a device |
| [device::enable](#method_device__enable) | Enable the device |
| [device::disable](#method_device__disable) | Disable the device |

<a id="method_connect"></a>
## *connect [<sup>method</sup>](#head_Methods)*

Connects to a server.

> This method is asynchronous.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | Server address |
| params?.timeout | integer | optional | Maximum time allowed for connecting in milliseconds (default: *1000*) |
| params.id | string (async ID) | mandatory |  |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_INPROGRESS``` | Currently connecting |
| ```ERROR_ALREADY_CONNECTED``` | Already connected to server |

### Asynchronous Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | *...* |
| result.address | string | mandatory | Device address |
| result.state | string | mandatory | Result of pairing operation (must be one of the following: *CONNECTED, CONNECTING, CONNECTING_ABORTED, CONNECTING_FAILED, CONNECTING_TIMED_OUT, DISCONNECTED*) |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.connect",
  "params": {
    "address": "192.158.1.38",
    "timeout": 1000,
    "id": "myid-completed"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

#### Asynchronous Response

```json
{
  "jsonrpc": "2.0",
  "method": "myid-complete.connect",
  "params": {
    "address": "192.158.1.38",
    "state": "CONNECTING_ABORTED"
  }
}
```

> The *async ID* parameter is passed within the notification designator, e.g. ``myid-complete.connect``.

<a id="method_abort"></a>
## *abort [<sup>method</sup>](#head_Methods)*

Aborts connecting to a server.

### Parameters

This method takes no parameters.

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | There is no ongoing connection |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.abort"
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a id="method_disconnect"></a>
## *disconnect [<sup>method</sup>](#head_Methods)*

Disconnects from the server.

### Parameters

This method takes no parameters.

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_INPROGRESS``` | Connecting in progress, abort first |
| ```ERROR_ALREADY_RELEASED``` | Not connected to server |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.disconnect"
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a id="method_acquire"></a>
## *acquire [<sup>method</sup>](#head_Methods)*

Acquires a device.

> This method creates an instance of a *device* object.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.name | string | mandatory | Name of the device to acquire |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | The device is not available |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.acquire",
  "params": {
    "name": "usb"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": 1
}
```

<a id="method_relinquish"></a>
## *relinquish [<sup>method</sup>](#head_Methods)*

Relinquishes a device.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.device | integer (instance ID) | mandatory | Device instance to relinquish<br> |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | The device is not acquired |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.relinquish",
  "params": {
    "device": 1
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a id="method_device__enable"></a>
## *device::enable [<sup>method</sup>](#head_Methods)*

Enable the device.

### Parameters

This method takes no parameters.

> The *device* instance ID shall be passed within the registration designator, e.g. ``GeneratorShowcase.1.device#<device-id>::enable``.

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ALREADY_CONNECT``` | The device is already enabled |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.device#1::enable"
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a id="method_device__disable"></a>
## *device::disable [<sup>method</sup>](#head_Methods)*

Disable the device.

### Parameters

This method takes no parameters.

> The *device* instance ID shall be passed within the registration designator, e.g. ``GeneratorShowcase.1.device#<device-id>::disable``.

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ALREADY_RELEASED``` | The device is not enabled |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.device#1::disable"
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a id="head_Properties"></a>
# Properties

The following properties are provided by the GeneratorShowcase plugin:

SimpleInstanceObjects interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [device::name](#property_device__name) | read-only | Name of the device |

<a id="property_device__name"></a>
## *device::name [<sup>property</sup>](#head_Properties)*

Provides access to the name of the device.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Name of the device |

> The *device instance ID* shall be passed within the designator, e.g. ``GeneratorShowcase.1.device#<device-id>::name``.

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.device#1::name"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "usb"
}
```

<a id="head_Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the GeneratorShowcase plugin:

SimpleInstanceObjects interface events:

| Notification | Description |
| :-------- | :-------- |
| [device::stateChanged](#notification_device__stateChanged) | Signals device state changes |

<a id="notification_device__stateChanged"></a>
## *device::stateChanged [<sup>notification</sup>](#head_Notifications)*

Signals device state changes.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.state | string | mandatory | New state of the device (must be one of the following: *DISABLED, ENABLED*) |

> The *device* instance ID shall be passed within the registration designator, e.g. ``GeneratorShowcase.1.device#<device-id>::register``.

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.device#1::register",
  "params": {
    "event": "stateChanged",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.device#1::stateChanged",
  "params": {
    "state": "DISABLED"
  }
}
```

> The *client ID* parameter is passed within the notification designator, e.g. ``myid.device#1::stateChanged``.

