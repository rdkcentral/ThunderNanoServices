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
- [Interfaces](#head.Interfaces)
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
| configuration?.autoconnect | string | <sup>*(optional)*</sup> Enable autoconnect |
| configuration?.retryinterval | string | <sup>*(optional)*</sup> Retry interval |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- Exchange::IWifiControl ([IWifiControl.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IWifiControl.h)) (version 1.0.0) (compliant format)

<a name="head.Methods"></a>
# Methods

The following methods are provided by the WifiControl plugin:

WifiControl interface methods:

| Method | Description |
| :-------- | :-------- |
| [scan](#method.scan) | Trigger Scanning |
| [abortscan](#method.abortscan) | Abort Currentlt running scan |
| [connect](#method.connect) | Connect device to requested SSID |
| [disconnect](#method.disconnect) | Disconnect device from requested SSID |
| [status](#method.status) | Status of current device, like which SSID is connected and it is in scanning state or not |


<a name="method.scan"></a>
## *scan [<sup>method</sup>](#head.Methods)*

Trigger Scanning.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "WifiControl.1.scan"
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

<a name="method.abortscan"></a>
## *abortscan [<sup>method</sup>](#head.Methods)*

Abort Currentlt running scan.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "WifiControl.1.abortscan"
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

<a name="method.connect"></a>
## *connect [<sup>method</sup>](#head.Methods)*

Connect device to requested SSID.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.configssid | string |  |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "WifiControl.1.connect",
    "params": {
        "configssid": "TESTWIFI"
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

<a name="method.disconnect"></a>
## *disconnect [<sup>method</sup>](#head.Methods)*

Disconnect device from requested SSID.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.configssid | string |  |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "WifiControl.1.disconnect",
    "params": {
        "configssid": "TESTWIFI"
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

<a name="method.status"></a>
## *status [<sup>method</sup>](#head.Methods)*

Status of current device, like which SSID is connected and it is in scanning state or not.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.connectedssid | string |  |
| result.isscanning | boolean |  |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "WifiControl.1.status"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "connectedssid": "TESTWIFI",
        "isscanning": false
    }
}
```

<a name="head.Properties"></a>
# Properties

The following properties are provided by the WifiControl plugin:

WifiControl interface properties:

| Property | Description |
| :-------- | :-------- |
| [networks](#property.networks) <sup>RO</sup> | Provides available networks information |
| [securities](#property.securities) <sup>RO</sup> | Provides security method of requested SSID |
| [configs](#property.configs) <sup>RO</sup> | Provides configs list |
| [config](#property.config) | Provide config details for requested SSID |


<a name="property.networks"></a>
## *networks [<sup>property</sup>](#head.Properties)*

Provides access to the provides available networks information.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Provides available networks information |
| result[#] | object |  |
| result[#].ssid | string |  |
| result[#].bssid | integer |  |
| result[#].frequency | integer |  |
| result[#].signal | integer |  |
| result[#].security | array |  |
| result[#].security[#] | string |  (must be one of the following: *Open*, *WEP*, *WPA*, *WPA2*, *WPS*, *Enterprise*, *WPA_WPA2*, *Unknown*) |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "WifiControl.1.networks"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": [
        {
            "ssid": "TESTWIFI",
            "bssid": 215985502509648,
            "frequency": 5180,
            "signal": 4294967272,
            "security": ["WPA","WPA2","Enterprise"]
        }
    ]
}
```

<a name="property.securities"></a>
## *securities [<sup>property</sup>](#head.Properties)*

Provides access to the provides security method of requested SSID.

> This property is **read-only**.

### Value

> The *ssid* argument shall be passed as the index to the property, e.g. *WifiControl.1.securities@xyz*.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Provides security method of requested SSID |
| result[#] | object |  |
| result[#].method | string |  (must be one of the following: *Open*, *WEP*, *WPA*, *WPA2*, *WPS*, *Enterprise*, *WPA_WPA2*, *Unknown*) |
| result[#].keys | array |  |
| result[#].keys[#] | string |  (must be one of the following: *PSK*, *EAP*, *CCMP*, *TKIP*, *Preauth*, *PBC*, *PIN*, *PSK_HASHED*, *None*) |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "WifiControl.1.securities@TESTWIFI"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": [
        {
            "method": "WPA",
            "keys": [
                "PSK"
            ]
        }
    ]
}
```

<a name="property.configs"></a>
## *configs [<sup>property</sup>](#head.Properties)*

Provides access to the provides configs list.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Provides configs list |
| result[#] | string |  |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "WifiControl.1.configs"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": [
        "..."
    ]
}
```

<a name="property.config"></a>
## *config [<sup>property</sup>](#head.Properties)*

Provides access to the provide config details for requested SSID.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Provide config details for requested SSID |
| (property).value | object |  |
| (property).value.hidden | boolean |  |
| (property).value.accesspoint | boolean |  |
| (property).value.ssid | string |  |
| (property).value.secret | string |  |
| (property).value.identity | string |  |
| (property).value.method | string |  (must be one of the following: *Open*, *WEP*, *WPA*, *WPA2*, *WPS*, *Enterprise*, *WPA_WPA2*, *Unknown*) |
| (property).value.key | string |  (must be one of the following: *PSK*, *EAP*, *CCMP*, *TKIP*, *Preauth*, *PBC*, *PIN*, *PSK_HASHED*, *None*) |

> The *ssid* argument shall be passed as the index to the property, e.g. *WifiControl.1.config@xyz*.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.hidden | boolean |  |
| result.accesspoint | boolean |  |
| result.ssid | string |  |
| result.secret | string |  |
| result.identity | string |  |
| result.method | string |  (must be one of the following: *Open*, *WEP*, *WPA*, *WPA2*, *WPS*, *Enterprise*, *WPA_WPA2*, *Unknown*) |
| result.key | string |  (must be one of the following: *PSK*, *EAP*, *CCMP*, *TKIP*, *Preauth*, *PBC*, *PIN*, *PSK_HASHED*, *None*) |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "WifiControl.1.config@TESTWIFI"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "hidden": false,
        "accesspoint": false,
        "ssid": "TESTWIFI",
        "secret": "1234test",
        "identity": "",
        "method": "WPA",
        "key": "PSK"
    }
}
```

#### Set Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "WifiControl.1.config@TESTWIFI",
    "params": {
        "value": {
            "hidden": false,
            "accesspoint": false,
            "ssid": "TESTWIFI",
            "secret": "1234test",
            "identity": "",
            "method": "WPA",
            "key": "PSK"
        }
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

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the implementation, and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the WifiControl plugin:

WifiControl interface events:

| Event | Description |
| :-------- | :-------- |
| [networkchange](#event.networkchange) | Notifies that Network were added, removed or modified |
| [connectionchange](#event.connectionchange) | Notifies that wifi connection changes |


<a name="event.networkchange"></a>
## *networkchange [<sup>event</sup>](#head.Notifications)*

Notifies that Network were added, removed or modified.

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
## *connectionchange [<sup>event</sup>](#head.Notifications)*

Notifies that wifi connection changes.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.ssid | string |  |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.connectionchange",
    "params": {
        "ssid": "TESTWIFI"
    }
}
```

