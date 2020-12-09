<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Wifi_Control_Plugin"></a>
# Wifi Control Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::white_circle:**

WifiControl plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)
- [Properties](#head.Properties)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the WifiControl plugin. It includes detailed specification about its configuration, methods and properties provided, as well as notifications sent.

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
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | Thunder API Reference |

<a name="head.Description"></a>
# Description

The WiFi Control plugin allows to manage various aspects of wireless connectivity.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *WifiControl*) |
| classname | string | Class name: *WifiControl* |
| locator | string | Library name: *libWPEWifiControl.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |
| configuration | object | <sup>*(optional)*</sup>  |
| configuration?.connector | string | <sup>*(optional)*</sup> Connector name |
| configuration?.interface | string | <sup>*(optional)*</sup> Interface name |
| configuration?.application | string | <sup>*(optional)*</sup> Application name |
| configuration?.preferred | string | <sup>*(optional)*</sup> Preferred |
| configuration?.autoconnect | string | <sup>*(optional)*</sup> Enable autoconnect |
| configuration?.retryinterval | string | <sup>*(optional)*</sup> Retry interval |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the WifiControl plugin:

WifiControl interface methods:

| Method | Description |
| :-------- | :-------- |
| [delete](#method.delete) | Forgets the configuration of a network |
| [store](#method.store) | Stores the configurations in persistent storage |
| [scan](#method.scan) | Searches for available networks |
| [connect](#method.connect) | Attempts connection to a network |
| [disconnect](#method.disconnect) | Disconnects from a network |


<a name="method.delete"></a>
## *delete <sup>method</sup>*

Forgets the configuration of a network.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.ssid | string | Identifier of a network |

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

<a name="method.store"></a>
## *store <sup>method</sup>*

Stores the configurations in persistent storage.

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
## *scan <sup>method</sup>*

Searches for available networks.

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
| 12 | ```ERROR_INPROGRESS``` | Returned when scan is already in progress |
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
## *connect <sup>method</sup>*

Attempts connection to a network.

Also see: [connectionchange](#event.connectionchange)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.ssid | string | Identifier of a network |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Returned when the network with a the given SSID doesn't exists |
| 2 | ```ERROR_UNAVAILABLE``` | Returned when connection fails if there is no associated bssid to connect and not defined as AccessPoint. Rescan and try to connect |
| 38 | ```ERROR_INVALID_SIGNATURE``` | Returned when connection is attempted with wrong password |
| 9 | ```ERROR_ALREADY_CONNECTED``` | Returned when connection already exists |
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
## *disconnect <sup>method</sup>*

Disconnects from a network.

Also see: [connectionchange](#event.connectionchange)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.ssid | string | Identifier of a network |

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

<a name="head.Properties"></a>
# Properties

The following properties are provided by the WifiControl plugin:

WifiControl interface properties:

| Property | Description |
| :-------- | :-------- |
| [status](#property.status) <sup>RO</sup> | Network status |
| [networks](#property.networks) <sup>RO</sup> | Available networks |
| [configs](#property.configs) <sup>RO</sup> | All WiFi configurations |
| [config](#property.config) | Single WiFi configuration |
| [debug](#property.debug) <sup>WO</sup> | Sets debug level |


<a name="property.status"></a>
## *status <sup>property</sup>*

Provides access to the network status.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Network status |
| (property).connected | string | Identifier of the connected network |
| (property).scanning | boolean | Indicates whether a scanning for available network is in progress |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "WifiControl.1.status"
}
```

#### Get Response

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

<a name="property.networks"></a>
## *networks <sup>property</sup>*

Provides access to the available networks.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | Available networks |
| (property)[#] | object |  |
| (property)[#].ssid | string | Identifier of a network |
| (property)[#].pairs | array |  |
| (property)[#].pairs[#] | object |  |
| (property)[#].pairs[#].method | string | Encryption method used by the network |
| (property)[#].pairs[#].keys | array |  |
| (property)[#].pairs[#].keys[#] | string | Types of supported keys |
| (property)[#]?.bssid | string | <sup>*(optional)*</sup> 48-bits long BSS identifier (might be MAC format) |
| (property)[#].frequency | number | Network's frequency in MHz |
| (property)[#].signal | number | Network's signal level in dBm |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "WifiControl.1.networks"
}
```

#### Get Response

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

<a name="property.configs"></a>
## *configs <sup>property</sup>*

Provides access to the all WiFi configurations.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | All WiFi configurations |
| (property)[#] | object |  |
| (property)[#].ssid | string | Identifier of a network |
| (property)[#]?.type | string | <sup>*(optional)*</sup> Type of protection. WPA_WPA2 means WPA, WPA2 and mixed types are allowed (must be one of the following: *Unknown*, *Unsecure*, *WPA*, *WPA2*, *WPA_WPA2*, *Enterprise*) |
| (property)[#].hidden | boolean | Indicates whether a network is hidden |
| (property)[#].accesspoint | boolean | Indicates if the network operates in AP mode |
| (property)[#]?.psk | string | <sup>*(optional)*</sup> Network's PSK in plaintext (irrelevant if hash is provided) |
| (property)[#]?.hash | string | <sup>*(optional)*</sup> Network's PSK as a hash |
| (property)[#].identity | string | User credentials (username part) for EAP |
| (property)[#].password | string | User credentials (password part) for EAP |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Configuration does not exist |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "WifiControl.1.configs"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": [
        {
            "ssid": "MyCorporateNetwork",
            "type": "WPA_WPA2",
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

<a name="property.config"></a>
## *config <sup>property</sup>*

Provides access to the single WiFi configuration.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Single WiFi configuration |
| (property).ssid | string | Identifier of a network |
| (property)?.type | string | <sup>*(optional)*</sup> Type of protection. WPA_WPA2 means WPA, WPA2 and mixed types are allowed (must be one of the following: *Unknown*, *Unsecure*, *WPA*, *WPA2*, *WPA_WPA2*, *Enterprise*) |
| (property).hidden | boolean | Indicates whether a network is hidden |
| (property).accesspoint | boolean | Indicates if the network operates in AP mode |
| (property)?.psk | string | <sup>*(optional)*</sup> Network's PSK in plaintext (irrelevant if hash is provided) |
| (property)?.hash | string | <sup>*(optional)*</sup> Network's PSK as a hash |
| (property).identity | string | User credentials (username part) for EAP |
| (property).password | string | User credentials (password part) for EAP |

> The *ssid* shall be passed as the index to the property, e.g. *WifiControl.1.config@MyCorporateNetwork*. If not specified all configurations are returned.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Configuration does not exist |
| 23 | ```ERROR_INCOMPLETE_CONFIG``` | Passed in configuration is invalid |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "WifiControl.1.config@MyCorporateNetwork"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": {
        "ssid": "MyCorporateNetwork",
        "type": "WPA_WPA2",
        "hidden": false,
        "accesspoint": true,
        "psk": "secretpresharedkey",
        "hash": "59e0d07fa4c7741797a4e394f38a5c321e3bed51d54ad5fcbd3f84bc7415d73d",
        "identity": "user",
        "password": "password"
    }
}
```

#### Set Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "WifiControl.1.config@MyCorporateNetwork",
    "params": {
        "ssid": "MyCorporateNetwork",
        "type": "WPA_WPA2",
        "hidden": false,
        "accesspoint": true,
        "psk": "secretpresharedkey",
        "hash": "59e0d07fa4c7741797a4e394f38a5c321e3bed51d54ad5fcbd3f84bc7415d73d",
        "identity": "user",
        "password": "password"
    }
}
```

#### Set Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": "null"
}
```

<a name="property.debug"></a>
## *debug <sup>property</sup>*

Provides access to the sets debug level.

> This property is **write-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | number | Debug level |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Returned when the operation is unavailable |

### Example

#### Set Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "WifiControl.1.debug",
    "params": 0
}
```

#### Set Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": "null"
}
```

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the implementation, and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the WifiControl plugin:

WifiControl interface events:

| Event | Description |
| :-------- | :-------- |
| [scanresults](#event.scanresults) | Signals that the scan operation has finished |
| [networkchange](#event.networkchange) | Signals that a network property has changed |
| [connectionchange](#event.connectionchange) | Notifies about connection state change |


<a name="event.scanresults"></a>
## *scanresults <sup>event</sup>*

Signals that the scan operation has finished.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | array |  |
| params[#] | object |  |
| params[#].ssid | string | Identifier of a network |
| params[#].pairs | array |  |
| params[#].pairs[#] | object |  |
| params[#].pairs[#].method | string | Encryption method used by the network |
| params[#].pairs[#].keys | array |  |
| params[#].pairs[#].keys[#] | string | Types of supported keys |
| params[#]?.bssid | string | <sup>*(optional)*</sup> 48-bits long BSS identifier (might be MAC format) |
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
## *networkchange <sup>event</sup>*

Signals that a network property has changed. e.g. frequency.

### Parameters

This event carries no parameters.

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.networkchange"
}
```

<a name="event.connectionchange"></a>
## *connectionchange <sup>event</sup>*

Notifies about connection state change. i.e. connected/disconnected.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | string | SSID of the connected network in case of connect or empty in case of disconnect |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.connectionchange",
    "params": "MyCorporateNetwork"
}
```

