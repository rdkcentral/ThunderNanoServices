<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_Wifi_Control_Plugin"></a>
# Wifi Control Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::white_circle:**

WifiControl plugin for Thunder framework.

### Table of Contents

- [Introduction](#head_Introduction)
- [Description](#head_Description)
- [Configuration](#head_Configuration)
- [Interfaces](#head_Interfaces)
- [Methods](#head_Methods)
- [Properties](#head_Properties)
- [Notifications](#head_Notifications)

<a id="head_Introduction"></a>
# Introduction

<a id="head_Scope"></a>
## Scope

This document describes purpose and functionality of the WifiControl plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

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

<a id="head_Description"></a>
# Description

The WiFi Control plugin allows to manage various aspects of wireless connectivity.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a id="head_Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *WifiControl*) |
| classname | string | mandatory | Class name: *WifiControl* |
| locator | string | mandatory | Library name: *libWPEWifiControl.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| configuration | object | optional | *...* |
| configuration?.connector | string | optional | Connector name |
| configuration?.interface | string | optional | Interface name |
| configuration?.application | string | optional | Application name |
| configuration?.autoconnect | string | optional | Enable autoconnect |
| configuration?.retryinterval | string | optional | Retry interval |

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- IWifiControl ([IWifiControl.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IWifiControl.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

<a id="head_Methods"></a>
# Methods

The following methods are provided by the WifiControl plugin:

WifiControl interface methods:

| Method | Description |
| :-------- | :-------- |
| [scan](#method_scan) | Trigger Scanning |
| [abortscan](#method_abortscan) | Abort Currentlt running scan |
| [connect](#method_connect) | Connect device to requested SSID |
| [disconnect](#method_disconnect) | Disconnect device from requested SSID |
| [status](#method_status) | Status of current device, like which SSID is connected and it is in scanning state or not |

<a id="method_scan"></a>
## *scan [<sup>method</sup>](#head_Methods)*

Trigger Scanning.

### Parameters

This method takes no parameters.

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

<a id="method_abortscan"></a>
## *abortscan [<sup>method</sup>](#head_Methods)*

Abort Currentlt running scan.

### Parameters

This method takes no parameters.

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

<a id="method_connect"></a>
## *connect [<sup>method</sup>](#head_Methods)*

Connect device to requested SSID.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.configssid | string | mandatory | SSID to be connected |

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
  "method": "WifiControl.1.connect",
  "params": {
    "configssid": "..."
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

<a id="method_disconnect"></a>
## *disconnect [<sup>method</sup>](#head_Methods)*

Disconnect device from requested SSID.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.configssid | string | mandatory | SSID to be disconnected |

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
  "method": "WifiControl.1.disconnect",
  "params": {
    "configssid": "..."
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

<a id="method_status"></a>
## *status [<sup>method</sup>](#head_Methods)*

Status of current device, like which SSID is connected and it is in scanning state or not.

### Parameters

This method takes no parameters.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | *...* |
| result.connectedssid | string | mandatory | SSID of connected router/ap |
| result.isscanning | boolean | mandatory | Scanning is in progress or not |

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
    "connectedssid": "...",
    "isscanning": false
  }
}
```

<a id="head_Properties"></a>
# Properties

The following properties are provided by the WifiControl plugin:

WifiControl interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [networks](#property_networks) | read-only | Provides available networks information |
| [securities](#property_securities) | read-only | Provides security method of requested SSID |
| [configs](#property_configs) | read-only | Provides configs list |
| [config](#property_config) | read/write | Provide config details for requested SSID |

<a id="property_networks"></a>
## *networks [<sup>property</sup>](#head_Properties)*

Provides access to the provides available networks information.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | Provides available networks information |
| (property)[#] | object | mandatory | *...* |
| (property)[#]?.ssid | string | optional | SSID of the network |
| (property)[#].bssid | integer | mandatory | BSSID of the network |
| (property)[#].frequency | integer | mandatory | Frequency used |
| (property)[#].signal | integer | mandatory | Signal strength |
| (property)[#].security | array | mandatory | Security method |
| (property)[#].security[#] | string | mandatory | *...* (must be one of the following: *Enterprise, Open, Unknown, WEP, WPA, WPA2, WPA_WPA2, WPS*) |

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
      "ssid": "...",
      "bssid": 0,
      "frequency": 0,
      "signal": 0,
      "security": [
        "WEP"
      ]
    }
  ]
}
```

<a id="property_securities"></a>
## *securities [<sup>property</sup>](#head_Properties)*

Provides access to the provides security method of requested SSID.

> This property is **read-only**.

> The *ssid* parameter shall be passed as the index to the property, i.e. ``securities@<ssid>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| ssid | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | Provides security method of requested SSID |
| (property)[#] | object | mandatory | *...* |
| (property)[#].method | string | mandatory | Security method (must be one of the following: *Enterprise, Open, Unknown, WEP, WPA, WPA2, WPA_WPA2, WPS*) |
| (property)[#]?.keys | array | optional | Security Keys |
| (property)[#]?.keys[#] | string | mandatory | *...* (must be one of the following: *CCMP, EAP, None, PBC, PIN, PSK, PSK_HASHED, Preauth, TKIP*) |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "WifiControl.1.securities@xyz"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "method": "WEP",
      "keys": [
        "EAP"
      ]
    }
  ]
}
```

<a id="property_configs"></a>
## *configs [<sup>property</sup>](#head_Properties)*

Provides access to the provides configs list.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | Provides configs list |
| (property)[#] | string | mandatory | *...* |

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

<a id="property_config"></a>
## *config [<sup>property</sup>](#head_Properties)*

Provides access to the provide config details for requested SSID.

> The *ssid* parameter shall be passed as the index to the property, i.e. ``config@<ssid>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| ssid | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Provide config details for requested SSID |
| (property).value | object | mandatory | *...* |
| (property).value.hidden | boolean | mandatory | Visibility of the router (hidden or visible) |
| (property).value.accesspoint | boolean | mandatory | Accesspoint or not |
| (property).value?.ssid | string | optional | SSID of the router/ap |
| (property).value?.secret | string | optional | Secret key used |
| (property).value?.identity | string | optional | Identity |
| (property).value.method | string | mandatory | Security method (must be one of the following: *Enterprise, Open, Unknown, WEP, WPA, WPA2, WPA_WPA2, WPS*) |
| (property).value?.key | string | optional | Security Info: method and keys (must be one of the following: *CCMP, EAP, None, PBC, PIN, PSK, PSK_HASHED, Preauth, TKIP*) |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Details about requested SSID |
| (property).hidden | boolean | mandatory | Visibility of the router (hidden or visible) |
| (property).accesspoint | boolean | mandatory | Accesspoint or not |
| (property)?.ssid | string | optional | SSID of the router/ap |
| (property)?.secret | string | optional | Secret key used |
| (property)?.identity | string | optional | Identity |
| (property).method | string | mandatory | Security method (must be one of the following: *Enterprise, Open, Unknown, WEP, WPA, WPA2, WPA_WPA2, WPS*) |
| (property)?.key | string | optional | Security Info: method and keys (must be one of the following: *CCMP, EAP, None, PBC, PIN, PSK, PSK_HASHED, Preauth, TKIP*) |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "WifiControl.1.config@xyz"
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
    "ssid": "...",
    "secret": "...",
    "identity": "...",
    "method": "WEP",
    "key": "EAP"
  }
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "WifiControl.1.config@xyz",
  "params": {
    "value": {
      "hidden": false,
      "accesspoint": false,
      "ssid": "...",
      "secret": "...",
      "identity": "...",
      "method": "WEP",
      "key": "EAP"
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

<a id="head_Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the WifiControl plugin:

WifiControl interface events:

| Notification | Description |
| :-------- | :-------- |
| [networkchange](#notification_networkchange) | Notifies that Network were added, removed or modified |
| [connectionchange](#notification_connectionchange) | Notifies that wifi connection changes |

<a id="notification_networkchange"></a>
## *networkchange [<sup>notification</sup>](#head_Notifications)*

Notifies that Network were added, removed or modified.

### Notification Parameters

This notification carries no parameters.

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "WifiControl.1.register",
  "params": {
    "event": "networkchange",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.networkchange"
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.networkchange``.

<a id="notification_connectionchange"></a>
## *connectionchange [<sup>notification</sup>](#head_Notifications)*

Notifies that wifi connection changes.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.ssid | string | mandatory | SSID of connection changed network |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "WifiControl.1.register",
  "params": {
    "event": "connectionchange",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.connectionchange",
  "params": {
    "ssid": "..."
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.connectionchange``.

