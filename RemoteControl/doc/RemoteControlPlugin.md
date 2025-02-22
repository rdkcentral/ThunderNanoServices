<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Remote_Control_Plugin"></a>
# Remote Control Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

RemoteControl plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Interfaces](#head.Interfaces)
- [Methods](#head.Methods)
- [Properties](#head.Properties)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the RemoteControl plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

<a name="head.Case_Sensitivity"></a>
## Case Sensitivity

All identifiers of the interfaces described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a name="head.Acronyms,_Abbreviations_and_Terms"></a>
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

<a name="head.References"></a>
## References

| Ref ID | Description |
| :-------- | :-------- |
| <a name="ref.HTTP">[HTTP](http://www.w3.org/Protocols)</a> | HTTP specification |
| <a name="ref.JSON-RPC">[JSON-RPC](https://www.jsonrpc.org/specification)</a> | JSON-RPC 2.0 specification |
| <a name="ref.JSON">[JSON](http://www.json.org/)</a> | JSON specification |
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20Thunder.docx)</a> | Thunder API Reference |

<a name="head.Description"></a>
# Description

The RemoteControl plugin provides user-input functionality from various key-code sources (e.g. STB RC).

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *RemoteControl*) |
| classname | string | mandatory | Class name: *RemoteControl* |
| locator | string | mandatory | Library name: *libThunderRemoteControl.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| configuration | object | optional | *...* |
| configuration?.mapfile | string | optional | Map File |
| configuration?.postlookupfile | string | optional | PostLookup File |
| configuration?.passon | boolean | optional | Enable passon |
| configuration?.repeatstart | integer | optional | Maximum number of repeats |
| configuration?.repeatinterval | integer | optional | Maximum duration between repeats |
| configuration?.releasetimeout | integer | optional | Release timeout |
| configuration?.devices | array | optional | List of devices |
| configuration?.devices[#] | object | optional | *...* |
| configuration?.devices[#]?.name | string | optional | Name |
| configuration?.devices[#]?.mapfile | string | optional | Map File |
| configuration?.devices[#]?.passon | boolean | optional | Enable passon |
| configuration?.devices[#]?.settings | string | optional | Settings |
| configuration?.virtuals | array | optional | List of virtuals |
| configuration?.virtuals[#] | object | optional | *...* |
| configuration?.virtuals[#]?.name | string | optional | Name |
| configuration?.virtuals[#]?.mapfile | string | optional | Map File |
| configuration?.virtuals[#]?.passon | boolean | optional | Enable passon |
| configuration?.virtuals[#]?.settings | string | optional | Settings |
| configuration?.links | array | optional | List of Links |
| configuration?.links[#] | object | optional | *...* |
| configuration?.links[#]?.name | string | optional | Name |
| configuration?.links[#]?.mapfile | string | optional | Map File |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- [RemoteControl.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/RemoteControl.json) (version 1.0.0) (compliant format)

<a name="head.Methods"></a>
# Methods

The following methods are provided by the RemoteControl plugin:

RemoteControl interface methods:

| Method | Description |
| :-------- | :-------- |
| [key](#method.key) | Gets key code details |
| [send](#method.send) | Sends a key to a device (press and release) |
| [press](#method.press) | Presses a key on a device |
| [release](#method.release) | Releases a key on a device |
| [add](#method.add) | Adds a key code to the key map |
| [modify](#method.modify) | Modifies a key code in the key map |
| [delete](#method.delete) | Deletes a key code from the key map |
| [load](#method.load) | Re-loads the device's key map from persistent memory |
| [save](#method.save) | Saves the device's key map into persistent path |
| [pair](#method.pair) | Activates pairing mode of a device |
| [unpair](#method.unpair) | Unpairs a device |

<a name="method.key"></a>
## *key [<sup>method</sup>](#head.Methods)*

Gets key code details.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.device | string | mandatory | Device name |
| params.code | integer | mandatory | Key code |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | *...* |
| result.code | integer | mandatory | Key code |
| result.key | integer | mandatory | Key ingest value |
| result?.modifiers | array | optional | List of key modifiers |
| result?.modifiers[#] | string | optional | Key modifier (must be one of the following: *leftalt, leftctrl, leftshift, rightalt, rightctrl, rightshift*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Key does not exist |
| ```ERROR_UNAVAILABLE``` | Unknown device |
| ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "RemoteControl.1.key",
  "params": {
    "device": "DevInput",
    "code": 1
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "code": 1,
    "key": 103,
    "modifiers": [
      "leftshift"
    ]
  }
}
```

<a name="method.send"></a>
## *send [<sup>method</sup>](#head.Methods)*

Sends a key to a device (press and release).

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.device | string | mandatory | Device name |
| params.code | integer | mandatory | Key code |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Unknown device |
| ```ERROR_BAD_REQUEST``` | Bad JSON param data format |
| ```ERROR_UNKNOWN_KEY``` | Key does not exist |
| ```ERROR_UNKNOWN_TABLE``` | Key map table does not exist |
| ```ERROR_ALREADY_RELEASED``` | Key is already released |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "RemoteControl.1.send",
  "params": {
    "device": "DevInput",
    "code": 1
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

<a name="method.press"></a>
## *press [<sup>method</sup>](#head.Methods)*

Presses a key on a device.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.device | string | mandatory | Device name |
| params.code | integer | mandatory | Key code |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Unknown device |
| ```ERROR_BAD_REQUEST``` | Bad JSON param data format |
| ```ERROR_UNKNOWN_KEY``` | Key does not exist |
| ```ERROR_UNKNOWN_TABLE``` | Key map table does not exist |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "RemoteControl.1.press",
  "params": {
    "device": "DevInput",
    "code": 1
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

<a name="method.release"></a>
## *release [<sup>method</sup>](#head.Methods)*

Releases a key on a device.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.device | string | mandatory | Device name |
| params.code | integer | mandatory | Key code |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Unknown device |
| ```ERROR_BAD_REQUEST``` | Bad JSON param data format |
| ```ERROR_UNKNOWN_KEY``` | Key does not exist |
| ```ERROR_UNKNOWN_TABLE``` | Key map table does not exist |
| ```ERROR_ALREADY_RELEASED``` | Key is already released |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "RemoteControl.1.release",
  "params": {
    "device": "DevInput",
    "code": 1
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

<a name="method.add"></a>
## *add [<sup>method</sup>](#head.Methods)*

Adds a key code to the key map.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.device | string | mandatory | Device name |
| params.code | integer | mandatory | Key code |
| params.key | integer | mandatory | Key ingest value |
| params?.modifiers | array | optional | List of key modifiers |
| params?.modifiers[#] | string | optional | Key modifier (must be one of the following: *leftalt, leftctrl, leftshift, rightalt, rightctrl, rightshift*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Unknown device |
| ```ERROR_BAD_REQUEST``` | Bad JSON param data format |
| ```ERROR_UNKNOWN_KEY``` | Code already exists |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "RemoteControl.1.add",
  "params": {
    "device": "DevInput",
    "code": 1,
    "key": 103,
    "modifiers": [
      "leftshift"
    ]
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

<a name="method.modify"></a>
## *modify [<sup>method</sup>](#head.Methods)*

Modifies a key code in the key map.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.device | string | mandatory | Device name |
| params.code | integer | mandatory | Key code |
| params.key | integer | mandatory | Key ingest value |
| params?.modifiers | array | optional | List of key modifiers |
| params?.modifiers[#] | string | optional | Key modifier (must be one of the following: *leftalt, leftctrl, leftshift, rightalt, rightctrl, rightshift*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Unknown device |
| ```ERROR_BAD_REQUEST``` | Bad JSON param data format |
| ```ERROR_UNKNOWN_KEY``` | Key does not exist |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "RemoteControl.1.modify",
  "params": {
    "device": "DevInput",
    "code": 1,
    "key": 103,
    "modifiers": [
      "leftshift"
    ]
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

<a name="method.delete"></a>
## *delete [<sup>method</sup>](#head.Methods)*

Deletes a key code from the key map.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.device | string | mandatory | Device name |
| params.code | integer | mandatory | Key code |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Key does not exist |
| ```ERROR_UNAVAILABLE``` | Unknown device |
| ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "RemoteControl.1.delete",
  "params": {
    "device": "DevInput",
    "code": 1
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

<a name="method.load"></a>
## *load [<sup>method</sup>](#head.Methods)*

Re-loads the device's key map from persistent memory.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.device | string | mandatory | Device name |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Unknown device |
| ```ERROR_GENERAL``` | File does not exist |
| ```ERROR_BAD_REQUEST``` | Bad JSON param data format |
| ```ERROR_ILLEGAL_STATE``` | Illegal state |
| ```ERROR_OPENING_FAILED``` | Opening failed |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "RemoteControl.1.load",
  "params": {
    "device": "DevInput"
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

<a name="method.save"></a>
## *save [<sup>method</sup>](#head.Methods)*

Saves the device's key map into persistent path.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.device | string | mandatory | Device name |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Unknown device |
| ```ERROR_GENERAL``` | File is not created |
| ```ERROR_BAD_REQUEST``` | Bad JSON param data format |
| ```ERROR_ILLEGAL_STATE``` | Illegal state |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "RemoteControl.1.save",
  "params": {
    "device": "DevInput"
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

<a name="method.pair"></a>
## *pair [<sup>method</sup>](#head.Methods)*

Activates pairing mode of a device.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.device | string | mandatory | Device name |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Unknown device |
| ```ERROR_GENERAL``` | Failed to activate pairing |
| ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "RemoteControl.1.pair",
  "params": {
    "device": "DevInput"
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

<a name="method.unpair"></a>
## *unpair [<sup>method</sup>](#head.Methods)*

Unpairs a device.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.device | string | mandatory | Device name |
| params.bindid | string | mandatory | Binding ID |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Unknown device |
| ```ERROR_GENERAL``` | Failed to unpair the device |
| ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "RemoteControl.1.unpair",
  "params": {
    "device": "DevInput",
    "bindid": "id"
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

<a name="head.Properties"></a>
# Properties

The following properties are provided by the RemoteControl plugin:

RemoteControl interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [devices](#property.devices) | read-only | Names of all available devices |
| [device](#property.device) | read-only | Metadata of a specific device |

<a name="property.devices"></a>
## *devices [<sup>property</sup>](#head.Properties)*

Provides access to the names of all available devices.

> This property is **read-only**.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | Names of all available devices |
| result[#] | string | mandatory | Device name |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "RemoteControl.1.devices"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    "Web"
  ]
}
```

<a name="property.device"></a>
## *device [<sup>property</sup>](#head.Properties)*

Provides access to the metadata of a specific device.

> This property is **read-only**.

> The *device* parameter shall be passed as the index to the property, e.g. ``RemoteControl.1.device@<device>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| device | string | mandatory | *...* |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | Metadata of a specific device |
| result.metadata | string | mandatory | Device metadata |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_GENERAL``` | Metadata not supported on a virtual device |
| ```ERROR_UNAVAILABLE``` | Unknown device |
| ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "RemoteControl.1.device@DevInput"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "metadata": "It is based on protocol A"
  }
}
```

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the RemoteControl plugin:

RemoteControl interface events:

| Notification | Description |
| :-------- | :-------- |
| [keypressed](#notification.keypressed) | Notifies of a key press/release action |

<a name="notification.keypressed"></a>
## *keypressed [<sup>notification</sup>](#head.Notifications)*

Notifies of a key press/release action.

### Parameters

> The *key code* parameter shall be passed within the client ID during registration, e.g. *42.myid*

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.pressed | boolean | mandatory | Denotes if the key was pressed (true) or released (false) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "RemoteControl.1.register",
  "params": {
    "event": "keypressed",
    "id": "42.myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "42.myid.keypressed",
  "params": {
    "pressed": false
  }
}
```

> The *key code* parameter is passed within the designator, e.g. *42.myid.keypressed*.

