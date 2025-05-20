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
- ISimpleCustomObjects ([ISimpleCustomObjects.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/ISimpleCustomObjects.h)) (version 1.0.0) (compliant format)

<a id="head_Methods"></a>
# Methods

The following methods are provided by the GeneratorShowcase plugin:

SimpleAsync interface methods:

| Method | Description |
| :-------- | :-------- |
| [connect](#method_connect) | Connects to a server |
| [abort](#method_abort) | Aborts connecting to a server |
| [disconnect](#method_disconnect) | Disconnects from the server |
| [link](#method_link) | Links a device |
| [unlink](#method_unlink) | Unlinks a device |
| [bind](#method_bind) | Binds a device |
| [unbind](#method_unbind) | Unlinks a device |

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
| params?.address | string (mac) | optional | Device address<br>*Decoded data length must be equal to 6 bytes.* |
| params?.timeout | integer | optional | Maximum time allowed for connecting in milliseconds (default: *1000*) |
| params.id | string (async ID) | mandatory |  |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_INPROGRESS``` | Currently connecting |
| ```ERROR_ALREADY_CONNECTED``` | Already connected to server |

### Asynchronous Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | *...* |
| result?.address | string (mac) | optional | Device address<br>*Decoded data length must be equal to 6 bytes.* |
| result.state | string | mandatory | Result of pairing operation (must be one of the following: *CONNECTED, CONNECTING, CONNECTING_ABORTED, CONNECTING_FAILED, CONNECTING_TIMED_OUT, DISCONNECTED*) |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.connect",
  "params": {
    "address": "aa:bb:cc:dd:ee:ff",
    "timeout": 1000,
    "id": "myid-complete"
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
    "address": "aa:bb:cc:dd:ee:ff",
    "state": "CONNECTING_ABORTED"
  }
}
```

> The *async ID* parameter is passed within the notification designator, i.e. ``<async-id>.connect``.

<a id="method_abort"></a>
## *abort [<sup>method</sup>](#head_Methods)*

Aborts connecting to a server.

### Parameters

This method takes no parameters.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

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

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

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

<a id="method_link"></a>
## *link [<sup>method</sup>](#head_Methods)*

Links a device.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string (base64) | mandatory | Device address<br>*Decoded data length must be equal to 6 bytes.* |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.link",
  "params": {
    "address": "11:22:33:44:55:66"
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

<a id="method_unlink"></a>
## *unlink [<sup>method</sup>](#head_Methods)*

Unlinks a device.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string (base64) | mandatory | Device address<br>*Decoded data length must be equal to 6 bytes.* |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.unlink",
  "params": {
    "address": "11:22:33:44:55:66"
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

<a id="method_bind"></a>
## *bind [<sup>method</sup>](#head_Methods)*

Binds a device.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | Device address<br>*String length must be equal to 6 chars.* |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.bind",
  "params": {
    "address": "11:22:33:44:55:66"
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

<a id="method_unbind"></a>
## *unbind [<sup>method</sup>](#head_Methods)*

Unlinks a device.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | Device address<br>*String length must be equal to 6 chars.* |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.unbind",
  "params": {
    "address": "11:22:33:44:55:66"
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

<a id="method_acquire"></a>
## *acquire [<sup>method</sup>](#head_Methods)*

Acquires a device.

> This method creates an instance of a *device* object.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.name | string | mandatory | Name of the device to acquire |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | integer (instance ID) | mandatory | Instance of the acquired device<br> |

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
  "result": 0
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

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

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
    "device": 0
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

> The *device instance ID* shall be passed within the method designator, i.e. ``device#<device-id>::enable``.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ALREADY_CONNECTED``` | The device is already enabled |

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

> The *device instance ID* shall be passed within the method designator, i.e. ``device#<device-id>::disable``.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

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

SimpleAsync interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [connected](#property_connected) | read-only | Connection status |
| [linkedDevice](#property_linkedDevice) | read-only | Linked device |
| [metadata](#property_metadata) | read/write | Device metadata |
| [boundDevice](#property_boundDevice) | read-only |  |
| [type](#property_type) | read/write | Device metadata |

SimpleInstanceObjects interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [device::name](#property_device__name) | read/write | Name of the device |
| [device::pin](#property_device__pin) | read/write | A pin |

SimpleCustomObjects interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [accessory](#property_accessory) | read-only | Accessory by name |
| [accessory::name](#property_accessory__name) | read/write | Name of the accessory |
| [accessory::pin](#property_accessory__pin) | read/write | Pin state |

<a id="property_connected"></a>
## *connected [<sup>property</sup>](#head_Properties)*

Provides access to the connection status.

> This property is **read-only**.

> The *address* parameter shall be passed as the index to the property, i.e. ``connected@<address>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| address | string (mac) | mandatory | Device address<br>*Decoded data length must be equal to 6 bytes.* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | boolean | mandatory | Connection status |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.connected@11:22:33:44:55:66"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": false
}
```

<a id="property_linkedDevice"></a>
## *linkedDevice [<sup>property</sup>](#head_Properties)*

Provides access to the linked device.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string (base64) | mandatory | Device address<br>*Decoded data length must be equal to 6 bytes.* |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.linkedDevice"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "11:22:33:44:55:66"
}
```

<a id="property_metadata"></a>
## *metadata [<sup>property</sup>](#head_Properties)*

Provides access to the device metadata.

> The *address* parameter shall be passed as the index to the property, i.e. ``metadata@<address>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| address | string (base64) | mandatory | Device address<br>*Decoded data length must be equal to 6 bytes.* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Device metadata |
| (property).value | string | mandatory | *...* |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Device metadata |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.metadata@11:22:33:44:55:66"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "..."
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.metadata@11:22:33:44:55:66",
  "params": {
    "value": "..."
  }
}
```

#### Set Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": "null"
}
```

<a id="property_boundDevice"></a>
## *boundDevice [<sup>property</sup>](#head_Properties)*

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | *...*<br>*String length must be equal to 6 chars.* |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.boundDevice"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "..."
}
```

<a id="property_type"></a>
## *type [<sup>property</sup>](#head_Properties)*

Provides access to the device metadata.

> The *address* parameter shall be passed as the index to the property, i.e. ``type@<address>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| address | string | mandatory | Device address<br>*String length must be equal to 6 chars.* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Device metadata |
| (property).value | string | mandatory | *...* |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Device metadata |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.type@11:22:33:44:55:66"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "..."
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.type@11:22:33:44:55:66",
  "params": {
    "value": "..."
  }
}
```

#### Set Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": "null"
}
```

<a id="property_device__name"></a>
## *device::name [<sup>property</sup>](#head_Properties)*

Provides access to the name of the device.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Name of the device |
| (property).value | string | mandatory | *...* |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Name of the device |

> The *device instance ID* shall be passed within the method designator, i.e. ``device#<device-id>::name``.

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

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.device#1::name",
  "params": {
    "value": "..."
  }
}
```

#### Set Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": "null"
}
```

<a id="property_device__pin"></a>
## *device::pin [<sup>property</sup>](#head_Properties)*

Provides access to the A pin.

> The *pin* parameter shall be passed as the index to the property, i.e. ``device::pin@<pin>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| pin | integer | mandatory | Pin number |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | A pin |
| (property).value | boolean | mandatory | *...* |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | boolean | mandatory | A pin |

> The *device instance ID* shall be passed within the method designator, i.e. ``device#<device-id>::pin``.

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Unknown pin number |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.device#1::pin@0"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": false
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.device#1::pin@0",
  "params": {
    "value": false
  }
}
```

#### Set Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": "null"
}
```

<a id="property_accessory"></a>
## *accessory [<sup>property</sup>](#head_Properties)*

Provides access to the accessory by name.

> This property is **read-only**.

> The *name* parameter shall be passed as the index to the property, i.e. ``accessory@<name>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| name | string | mandatory | Name of the accessory to look for |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string (instance ID) | mandatory | Accessory instance<br> |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.accessory@mouse"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "id1"
}
```

<a id="property_accessory__name"></a>
## *accessory::name [<sup>property</sup>](#head_Properties)*

Provides access to the name of the accessory.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Name of the accessory |
| (property).value | string | mandatory | *...* |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Name of the accessory |

> The *accessory instance ID* shall be passed within the method designator, i.e. ``accessory#<accessory-id>::name``.

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.accessory#id1::name"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "mouse"
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.accessory#id1::name",
  "params": {
    "value": "..."
  }
}
```

#### Set Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": "null"
}
```

<a id="property_accessory__pin"></a>
## *accessory::pin [<sup>property</sup>](#head_Properties)*

Provides access to the pin state.

> The *pin* parameter shall be passed as the index to the property, i.e. ``accessory::pin@<pin>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| pin | integer | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Pin state |
| (property).value | boolean | mandatory | *...* |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | boolean | mandatory | Pin state |

> The *accessory instance ID* shall be passed within the method designator, i.e. ``accessory#<accessory-id>::pin``.

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.accessory#id1::pin@0"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": false
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.accessory#id1::pin@0",
  "params": {
    "value": false
  }
}
```

#### Set Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": "null"
}
```

<a id="head_Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the GeneratorShowcase plugin:

SimpleAsync interface events:

| Notification | Description |
| :-------- | :-------- |
| [statusChanged](#notification_statusChanged) | Signals completion of the Connect method |
| [bindingChanged](#notification_bindingChanged) | Signals completion of the Connect method |

SimpleInstanceObjects interface events:

| Notification | Description |
| :-------- | :-------- |
| [device::nameChanged](#notification_device__nameChanged) | Signals device name changes |
| [device::stateChanged](#notification_device__stateChanged) | Signals device state changes |
| [device::pinChanged](#notification_device__pinChanged) | Signals pin state changes |

SimpleCustomObjects interface events:

| Notification | Description |
| :-------- | :-------- |
| [added](#notification_added) | Signals addition of a accessory |
| [removed](#notification_removed) | Signals removal of a accessory |
| [accessory::nameChanged](#notification_accessory__nameChanged) | Signals addition of a accessory |

<a id="notification_statusChanged"></a>
## *statusChanged [<sup>notification</sup>](#head_Notifications)*

Signals completion of the Connect method.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | array | mandatory | Device address |
| params.address[#] | integer | mandatory | Device address |
| params.linked | boolean | mandatory | Denotes if device is linked |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.register",
  "params": {
    "event": "statusChanged",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.statusChanged",
  "params": {
    "address": [
      11,
      22
    ],
    "linked": false
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.statusChanged``.

<a id="notification_bindingChanged"></a>
## *bindingChanged [<sup>notification</sup>](#head_Notifications)*

Signals completion of the Connect method.

### Parameters

> The *address* parameter shall be passed within the *id* parameter to the ``register`` call, i.e. ``<address>.<client-id>``.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.bound | boolean | mandatory | *...* |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.register",
  "params": {
    "event": "bindingChanged",
    "id": "[11,22].myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "[11,22].myid.bindingChanged",
  "params": {
    "bound": false
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<address>.<client-id>.bindingChanged``.

> The *address* parameter is passed within the notification designator, i.e. ``<address>.<client-id>.bindingChanged``.

<a id="notification_device__nameChanged"></a>
## *device::nameChanged [<sup>notification</sup>](#head_Notifications)*

Signals device name changes.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.state | string | mandatory | New name of the device |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.device#1::register",
  "params": {
    "event": "nameChanged",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.device#1::nameChanged",
  "params": {
    "state": "..."
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.device#<device-id>::nameChanged``.

> The *device instance id* parameter is passed within the notification designator, i.e. ``<client-id>.device#<device-id>::nameChanged``.

<a id="notification_device__stateChanged"></a>
## *device::stateChanged [<sup>notification</sup>](#head_Notifications)*

Signals device state changes.

> This notification may also be triggered by client registration.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.state | string | mandatory | New state of the device (must be one of the following: *DISABLED, ENABLED*) |

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

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.device#<device-id>::stateChanged``.

> The *device instance id* parameter is passed within the notification designator, i.e. ``<client-id>.device#<device-id>::stateChanged``.

<a id="notification_device__pinChanged"></a>
## *device::pinChanged [<sup>notification</sup>](#head_Notifications)*

Signals pin state changes.

> This notification may also be triggered by client registration.

### Parameters

> The *pin* parameter shall be passed within the *id* parameter to the ``register`` call, i.e. ``<pin>.<client-id>``.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.high | boolean | mandatory | *...* |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.device#1::register",
  "params": {
    "event": "pinChanged",
    "id": "0.myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "0.myid.device#1::pinChanged",
  "params": {
    "high": false
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<pin>.<client-id>.device#<device-id>::pinChanged``.

> The *pin* parameter is passed within the notification designator, i.e. ``<pin>.<client-id>.device#<device-id>::pinChanged``.

> The *device instance id* parameter is passed within the notification designator, i.e. ``<pin>.<client-id>.device#<device-id>::pinChanged``.

<a id="notification_added"></a>
## *added [<sup>notification</sup>](#head_Notifications)*

Signals addition of a accessory.

> This notification may also be triggered by client registration.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.accessory | string (instance ID) | mandatory | Accessory instance<br> |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.register",
  "params": {
    "event": "added",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.added",
  "params": {
    "accessory": "id1"
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.added``.

<a id="notification_removed"></a>
## *removed [<sup>notification</sup>](#head_Notifications)*

Signals removal of a accessory.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.accessory | string (instance ID) | mandatory | Accessory instance<br> |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.register",
  "params": {
    "event": "removed",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.removed",
  "params": {
    "accessory": "id1"
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.removed``.

<a id="notification_accessory__nameChanged"></a>
## *accessory::nameChanged [<sup>notification</sup>](#head_Notifications)*

Signals addition of a accessory.

> This notification may also be triggered by client registration.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.name | string | mandatory | Name of the accessory |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "GeneratorShowcase.1.accessory#id1::register",
  "params": {
    "event": "nameChanged",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.accessory#id1::nameChanged",
  "params": {
    "name": "mouse"
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.accessory#<accessory-id>::nameChanged``.

> The *accessory instance id* parameter is passed within the notification designator, i.e. ``<client-id>.accessory#<accessory-id>::nameChanged``.

