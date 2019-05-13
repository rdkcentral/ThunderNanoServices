<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Remote_Control_Plugin"></a>
# Remote Control Plugin

**Version: 1.0**

RemoteControl functionality for WPEFramework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the RemoteControl plugin. It includes detailed specification of its configuration and methods provided.

<a name="head.Case_Sensitivity"></a>
## Case Sensitivity

All identifiers on the interface described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

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
| <a name="ref.WPEF">[WPEF](https://github.com/WebPlatformForEmbedded/WPEFramework/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | WPEFramework API Reference |

<a name="head.Description"></a>
# Description

The RemoteControl plugin provides user-input functionality from various key-code sources (e.g. STB RC).

The plugin is designed to be loaded and executed within the WPEFramework. For more information on WPEFramework refer to [[WPEF](#ref.WPEF)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *RemoteControl*) |
| classname | string | Class name: *RemoteControl* |
| locator | string | Library name: *libWPEFrameworkRemoteControl.so* |
| autostart | boolean | Determines if the plugin is to be started automatically along with the framework |

<a name="head.Methods"></a>
# Methods

The following API is provided by the plugin via JSON-RPC:

- [devices](#method.devices)
- [device](#method.device)
- [key](#method.key)
- [delete](#method.delete)
- [modify](#method.modify)
- [pair](#method.pair)
- [unpair](#method.unpair)
- [send](#method.send)
- [press](#method.press)
- [release](#method.release)
- [save](#method.save)
- [load](#method.load)
- [add](#method.add)

This API follows the JSON-RPC 2.0 specification. Refer to [[JSON-RPC](#ref.JSON-RPC)] for more information.


<a name="method.devices"></a>
## *devices*

General information.

### Description

Retrieves the names of all available devices.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array |  |
| result[#] | string | Available device name |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "RemoteControl.1.devices"
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        "Web"
    ]
}
```
<a name="method.device"></a>
## *device*

Get device.

### Description

Retrieves the metadata of specific devices.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.device | string | Device name |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.name | string | Device name |
| result.metadata | string | Device metadata |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 1 | ```ERROR_GENERAL``` | Virtual device is loaded |
| 2 | ```ERROR_UNAVAILABLE``` | Unknown device |
| 30 | ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "RemoteControl.1.device", 
    "params": {
        "device": "DevInput"
    }
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": {
        "name": "DevInput", 
        "metadata": "It is based on protocol A"
    }
}
```
<a name="method.key"></a>
## *key*

Key action.

### Description

Retrieves the key code details.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.device | string | Device name |
| params.code | number | Key code |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.code | number | Key code |
| result.key | number | Key ingest code |
| result.modifiers | array |  |
| result.modifiers[#] | string | Key modifiers (must be one of the following: *leftshift*, *rightshift*, *leftalt*, *rightalt*, *leftctrl*, *rightctrl*) |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Key does not exist |
| 2 | ```ERROR_UNAVAILABLE``` | Unknown device |
| 30 | ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
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
    "id": 1234567890, 
    "result": {
        "code": 1, 
        "key": 103, 
        "modifiers": [
            "leftshift"
        ]
    }
}
```
<a name="method.delete"></a>
## *delete*

Key mapping actions.

### Description

Deletes the key code from map.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.device | string | Device name |
| params.code | number | Key code |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Key does not exist |
| 2 | ```ERROR_UNAVAILABLE``` | Unknown device |
| 30 | ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
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
    "id": 1234567890, 
    "result": null
}
```
<a name="method.modify"></a>
## *modify*

Key mapping actions.

### Description

Modifies the key code in the map.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.device | string | Device name |
| params.code | number | Key code |
| params.key | number | Key ingest code |
| params.modifiers | array |  |
| params.modifiers[#] | string | Key modifiers (must be one of the following: *leftshift*, *rightshift*, *leftalt*, *rightalt*, *leftctrl*, *rightctrl*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unknown device |
| 30 | ```ERROR_BAD_REQUEST``` | Bad JSON param data format |
| 22 | ```ERROR_UNKNOWN_KEY``` | Key does not exist |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
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
    "id": 1234567890, 
    "result": null
}
```
<a name="method.pair"></a>
## *pair*

Pairing device.

### Description

Activates pairing mode of specific device.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.device | string | Device name |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unknown device |
| 1 | ```ERROR_GENERAL``` | Failed to activate pairing |
| 30 | ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
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
    "id": 1234567890, 
    "result": null
}
```
<a name="method.unpair"></a>
## *unpair*

Pairing device.

### Description

Unpairs of specific device.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.device | string | Device name |
| params.bindid | string | Binding id |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unknown device |
| 1 | ```ERROR_GENERAL``` | Failed to unpair a specific device |
| 30 | ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
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
    "id": 1234567890, 
    "result": null
}
```
<a name="method.send"></a>
## *send*

Key mapping actions.

### Description

Sends a code to a device.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.device | string | Device name |
| params.code | number | Key code |
| params.key | number | Key ingest code |
| params.modifiers | array |  |
| params.modifiers[#] | string | Key modifiers (must be one of the following: *leftshift*, *rightshift*, *leftalt*, *rightalt*, *leftctrl*, *rightctrl*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unknown device |
| 30 | ```ERROR_BAD_REQUEST``` | Bad JSON param data format |
| 22 | ```ERROR_UNKNOWN_KEY``` | Key does not exist |
| 28 | ```ERROR_UNKNOWN_TABLE``` | Key map table does not exist |
| 36 | ```ERROR_ALREADY_RELEASED``` | Key is already releases |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "RemoteControl.1.send", 
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
    "id": 1234567890, 
    "result": null
}
```
<a name="method.press"></a>
## *press*

Key mapping actions.

### Description

Sends a code 'Press' to a device.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.device | string | Device name |
| params.code | number | Key code |
| params.key | number | Key ingest code |
| params.modifiers | array |  |
| params.modifiers[#] | string | Key modifiers (must be one of the following: *leftshift*, *rightshift*, *leftalt*, *rightalt*, *leftctrl*, *rightctrl*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unknown device |
| 30 | ```ERROR_BAD_REQUEST``` | Bad JSON param data format |
| 22 | ```ERROR_UNKNOWN_KEY``` | Key does not exist |
| 28 | ```ERROR_UNKNOWN_TABLE``` | Key map table does not exist |
| 36 | ```ERROR_ALREADY_RELEASED``` | Key is already releases |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "RemoteControl.1.press", 
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
    "id": 1234567890, 
    "result": null
}
```
<a name="method.release"></a>
## *release*

Key mapping actions.

### Description

Sends a code 'Release' to a device.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.device | string | Device name |
| params.code | number | Key code |
| params.key | number | Key ingest code |
| params.modifiers | array |  |
| params.modifiers[#] | string | Key modifiers (must be one of the following: *leftshift*, *rightshift*, *leftalt*, *rightalt*, *leftctrl*, *rightctrl*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unknown device |
| 30 | ```ERROR_BAD_REQUEST``` | Bad JSON param data format |
| 22 | ```ERROR_UNKNOWN_KEY``` | Key does not exist |
| 28 | ```ERROR_UNKNOWN_TABLE``` | Key map table does not exist |
| 36 | ```ERROR_ALREADY_RELEASED``` | Key is already releases |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "RemoteControl.1.release", 
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
    "id": 1234567890, 
    "result": null
}
```
<a name="method.save"></a>
## *save*

Key mapping actions.

### Description

Saves the loaded key map as DEVICE_NAME.json into persistent path.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.device | string | Device name |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unknown device |
| 1 | ```ERROR_GENERAL``` | File is not created |
| 30 | ```ERROR_BAD_REQUEST``` | Bad JSON param data format |
| 5 | ```ERROR_ILLEGAL_STATE``` | Illegal state |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
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
    "id": 1234567890, 
    "result": null
}
```
<a name="method.load"></a>
## *load*

Key mapping actions.

### Description

Re-loads DEVICE_NAME.json key map into memory.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.device | string | Device name |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unknown device |
| 1 | ```ERROR_GENERAL``` | File does not exist |
| 30 | ```ERROR_BAD_REQUEST``` | Bad JSON param data format |
| 5 | ```ERROR_ILLEGAL_STATE``` | Illegal state |
| 6 | ```ERROR_OPENING_FAILED``` | Opening failed |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
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
    "id": 1234567890, 
    "result": null
}
```
<a name="method.add"></a>
## *add*

Key mapping actions.

### Description

Adds the key code in the map.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.device | string | Device name |
| params.code | number | Key code |
| params.key | number | Key ingest code |
| params.modifiers | array |  |
| params.modifiers[#] | string | Key modifiers (must be one of the following: *leftshift*, *rightshift*, *leftalt*, *rightalt*, *leftctrl*, *rightctrl*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unknown device |
| 30 | ```ERROR_BAD_REQUEST``` | Bad JSON param data format |
| 22 | ```ERROR_UNKNOWN_KEY``` | Code already exists |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
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
    "id": 1234567890, 
    "result": null
}
```
