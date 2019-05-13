<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.WiFi_Control_Plugin"></a>
# WiFi Control Plugin

**Version: 1.0**

WifiControl functionality for WPEFramework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the WifiControl plugin. It includes detailed specification of its configuration, methods provided and notifications sent.

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

The WiFi Control plugin allows to manage various aspects of wireless connectivity.

The plugin is designed to be loaded and executed within the WPEFramework. For more information on WPEFramework refer to [[WPEF](#ref.WPEF)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *WifiControl*) |
| classname | string | Class name: *WifiControl* |
| locator | string | Library name: *libWPEWifiControl.so* |
| autostart | boolean | Determines if the plugin is to be started automatically along with the framework |

<a name="head.Methods"></a>
# Methods

The following API is provided by the plugin via JSON-RPC:

- [status](#method.status)
- [networks](#method.networks)
- [config](#method.config)
- [delete](#method.delete)
- [setconfig](#method.setconfig)
- [store](#method.store)
- [scan](#method.scan)
- [connect](#method.connect)
- [disconnect](#method.disconnect)
- [debug](#method.debug)

This API follows the JSON-RPC 2.0 specification. Refer to [[JSON-RPC](#ref.JSON-RPC)] for more information.


<a name="method.status"></a>
## *status*

Returns the current status information

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.connected | string | Identifier of the connected network (32-bytes long) |
| result.scanning | boolean | Indicates whether a scanning for available network is in progress |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "WifiControl.1.status"
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": {
        "connected": "MyCorporateNetwork", 
        "scanning": false
    }
}
```
<a name="method.networks"></a>
## *networks*

Returns information about available networks

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array |  |
| result[#] | object |  |
| result[#].ssid | string | Identifier of a network (32-bytes long) |
| result[#].pairs | array |  |
| result[#].pairs[#] | object |  |
| result[#].pairs[#].method | string | Encryption method used by a network |
| result[#].pairs[#].keys | array |  |
| result[#].pairs[#].keys[#] | string | Type of a supported key |
| result[#]?.bssid | string | <sup>*(optional)*</sup> 48-bits long BSS' identifier (might be MAC format) |
| result[#].frequency | number | Network's frequency in MHz |
| result[#].signal | number | Network's signal level in dBm |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "WifiControl.1.networks"
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        {
            "ssid": "MyCorporateNetwork", 
            "pairs": [
                {
                    "method": "WPA", 
                    "keys": [
                        "psk"
                    ]
                }
            ], 
            "bssid": "94:b4:0f:77:cc:71", 
            "frequency": 5180, 
            "signal": -44
        }
    ]
}
```
<a name="method.config"></a>
## *config*

Returns information about configuration of the specified network(s) (FIXME!!!)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.ssid | string | Identifier of a network (32-bytes long) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array |  |
| result[#] | object |  |
| result[#].ssid | string | Identifier of a network (32-bytes long) |
| result[#]?.type | string | <sup>*(optional)*</sup> Level of security (must be one of the following: *Unknown*, *Unsecure*, *WPA*, *Enterprise*) |
| result[#].hidden | boolean | Indicates whether a network is hidden |
| result[#].accesspoint | boolean | Indicates if the network operates in AP mode |
| result[#]?.psk | string | <sup>*(optional)*</sup> Network's PSK in plaintext (irrelevant if hash is provided) |
| result[#]?.hash | string | <sup>*(optional)*</sup> Network's PSK as a hash |
| result[#].identity | string | User credentials (username part) for EAP |
| result[#].password | string | User credentials (password part) for EAP |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "WifiControl.1.config", 
    "params": {
        "ssid": "MyCorporateNetwork"
    }
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        {
            "ssid": "MyCorporateNetwork", 
            "type": "WPA", 
            "hidden": false, 
            "accesspoint": true, 
            "psk": "secretpresharedkey", 
            "hash": "59e0d07fa4c7741797a4e394f38a5c321e3bed51d54ad5fcbd3f84bc7415d73d", 
            "identity": "user", 
            "password": "password"
        }
    ]
}
```
<a name="method.delete"></a>
## *delete*

Forgets configuration of the specified network

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.ssid | string | Identifier of a network (32-bytes long) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "WifiControl.1.delete", 
    "params": {
        "ssid": "MyCorporateNetwork"
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
<a name="method.setconfig"></a>
## *setconfig*

Updates or creates a configuration for the specified network

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.ssid | string | Identifier of a network (32-bytes long) |
| params.config | object |  |
| params.config.ssid | string | Identifier of a network (32-bytes long) |
| params.config?.type | string | <sup>*(optional)*</sup> Level of security (must be one of the following: *Unknown*, *Unsecure*, *WPA*, *Enterprise*) |
| params.config.hidden | boolean | Indicates whether a network is hidden |
| params.config.accesspoint | boolean | Indicates if the network operates in AP mode |
| params.config?.psk | string | <sup>*(optional)*</sup> Network's PSK in plaintext (irrelevant if hash is provided) |
| params.config?.hash | string | <sup>*(optional)*</sup> Network's PSK as a hash |
| params.config.identity | string | User credentials (username part) for EAP |
| params.config.password | string | User credentials (password part) for EAP |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Config does not exist |
| 23 | ```ERROR_INCOMPLETE_CONFIG``` | Passed in config is invalid |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "WifiControl.1.setconfig", 
    "params": {
        "ssid": "MyCorporateNetwork", 
        "config": {
            "ssid": "MyCorporateNetwork", 
            "type": "WPA", 
            "hidden": false, 
            "accesspoint": true, 
            "psk": "secretpresharedkey", 
            "hash": "59e0d07fa4c7741797a4e394f38a5c321e3bed51d54ad5fcbd3f84bc7415d73d", 
            "identity": "user", 
            "password": "password"
        }
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
<a name="method.store"></a>
## *store*

Stores configurations in the permanent storage

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 40 | ```ERROR_WRITE_ERROR``` | Returned when the operation failed |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "WifiControl.1.store"
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
<a name="method.scan"></a>
## *scan*

Searches for an available networks

Also see: [scanresults](#event.scanresults)

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 12 | ```ERROR_INPROGRESS``` | Operation in progress |
| 2 | ```ERROR_UNAVAILABLE``` | Returned when scanning is not available for some reason |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "WifiControl.1.scan"
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
<a name="method.connect"></a>
## *connect*

Attempt connection to the specified network

Also see: [connectionchange](#event.connectionchange)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.ssid | string | Identifier of a network (32-bytes long) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Returned when the network with a the given SSID doesn't exists |
| 4 | ```ERROR_ASYNC_ABORTED``` | Returned when connection attempt fails for other reasons |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "WifiControl.1.connect", 
    "params": {
        "ssid": "MyCorporateNetwork"
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
<a name="method.disconnect"></a>
## *disconnect*

Disconnects from the specified network

Also see: [connectionchange](#event.connectionchange)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.ssid | string | Identifier of a network (32-bytes long) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Returned when the network with a the given SSID doesn't exists |
| 4 | ```ERROR_ASYNC_ABORTED``` | Returned when disconnection attempt fails for other reasons |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "WifiControl.1.disconnect", 
    "params": {
        "ssid": "MyCorporateNetwork"
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
<a name="method.debug"></a>
## *debug*

Sets specified debug level

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.level | number | Debul level |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Returned when the operation is unavailable |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "WifiControl.1.debug", 
    "params": {
        "level": 0
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
<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the plugin, and broadcasted via JSON-RPC to all registered observers. Refer to [[WPEF](#ref.WPEF)] for information on how to register for a notification.

The following notifications are provided by the plugin:

- [scanresults](#event.scanresults)
- [networkchange](#event.networkchange)
- [connectionchange](#event.connectionchange)

<a name="event.scanresults"></a>
## *scanresults*

Signals that the scan operation has finished and carries results of it

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | array |  |
| params[#] | object |  |
| params[#].ssid | string | Identifier of a network (32-bytes long) |
| params[#].pairs | array |  |
| params[#].pairs[#] | object |  |
| params[#].pairs[#].method | string | Encryption method used by a network |
| params[#].pairs[#].keys | array |  |
| params[#].pairs[#].keys[#] | string | Type of a supported key |
| params[#]?.bssid | string | <sup>*(optional)*</sup> 48-bits long BSS' identifier (might be MAC format) |
| params[#].frequency | number | Network's frequency in MHz |
| params[#].signal | number | Network's signal level in dBm |

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "client.events.1.scanresults", 
    "params": [
        {
            "ssid": "MyCorporateNetwork", 
            "pairs": [
                {
                    "method": "WPA", 
                    "keys": [
                        "psk"
                    ]
                }
            ], 
            "bssid": "94:b4:0f:77:cc:71", 
            "frequency": 5180, 
            "signal": -44
        }
    ]
}
```
<a name="event.networkchange"></a>
## *networkchange*

Informs that something has changed with the network e.g. frequency

### Parameters

This event has no parameters.

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "client.events.1.networkchange"
}
```
<a name="event.connectionchange"></a>
## *connectionchange*

Notifies about connection state change i.e. connected/disconnected

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | string | Identifier of a network (32-bytes long) |

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "client.events.1.connectionchange", 
    "params": "MyCorporateNetwork"
}
```
