<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Firmware_Control_Plugin"></a>
# Firmware Control Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

FirmwareControl plugin for Thunder framework.

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

This document describes purpose and functionality of the FirmwareControl plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

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

Control Firmware upgrade to the device.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *FirmwareControl*) |
| classname | string | mandatory | Class name: *FirmwareControl* |
| locator | string | mandatory | Library name: *libThunderFirmwareControl.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| configuration | object | optional | *...* |
| configuration?.source | string | optional | Source URL or location of the firmware |
| configuration?.download | string | optional | Location where the firmware to be downloaded |
| configuration?.waittime | integer | optional | Maximum duration to finish download or install process |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- [FirmwareControl.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/FirmwareControl.json) (version 1.0.0) (compliant format)

<a name="head.Methods"></a>
# Methods

The following methods are provided by the FirmwareControl plugin:

FirmwareControl interface methods:

| Method | Description |
| :-------- | :-------- |
| [upgrade](#method.upgrade) | Upgrade the device to the given firmware |
| [resume](#method.resume) | Resume download from the last paused position |

<a name="method.upgrade"></a>
## *upgrade [<sup>method</sup>](#head.Methods)*

Upgrade the device to the given firmware. (Note: Ensure size of firmware image should be < 500MB).

Also see: [upgradeprogress](#event.upgradeprogress)

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.name | string | mandatory | Name of the firmware |
| params?.location | string | optional | Location or URL of the firmware to be upgraded |
| params?.type | string | optional | Type of the firmware (must be one of the following: *CDL, RCDL*) |
| params?.progressinterval | integer | optional | Number of seconds between progress update events (5 seconds, 10 seconds etc). 0 means invoking callback only once to report final upgrade result |
| params?.hmac | string | optional | HMAC value of firmare |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_INPROGRESS``` | Operation in progress |
| ```ERROR_INCORRECT_URL``` | Invalid location given |
| ```ERROR_UNAVAILABLE``` | Error in download |
| ```ERROR_BAD_REQUEST``` | Bad file name given |
| ```ERROR_ILLEGAL_STATE``` | Invalid state of device |
| ```ERROR_INCORRECT_HASH``` | Incorrect hash given |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "FirmwareControl.1.upgrade",
  "params": {
    "name": "firmware_v.0",
    "location": "http://my.site.com/images",
    "type": "RCDL",
    "progressinterval": 10,
    "hmac": "2834e6d07fa4c7778ef7a4e394f38a5c321afbed51d54ad512bd3fffbc7aa5debc"
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

<a name="method.resume"></a>
## *resume [<sup>method</sup>](#head.Methods)*

Resume download from the last paused position.

Also see: [upgradeprogress](#event.upgradeprogress)

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.name | string | mandatory | Name of the firmware |
| params?.location | string | optional | Location or URL of the firmware to be upgraded |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_INPROGRESS``` | Operation in progress |
| ```ERROR_INCORRECT_URL``` | Invalid location given |
| ```ERROR_UNAVAILABLE``` | Error in download |
| ```ERROR_BAD_REQUEST``` | Bad file name given |
| ```ERROR_ILLEGAL_STATE``` | Invalid state of device |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "FirmwareControl.1.resume",
  "params": {
    "name": "firmware_v.0",
    "location": "http://my.site.com/images"
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

The following properties are provided by the FirmwareControl plugin:

FirmwareControl interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [status](#property.status) | read-only | Current status of a upgrade |
| [downloadsize](#property.downloadsize) | read-only | Max free space available to download image |

<a name="property.status"></a>
## *status [<sup>property</sup>](#head.Properties)*

Provides access to the current status of a upgrade.

> This property is **read-only**.

Also see: [upgradeprogress](#event.upgradeprogress)

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | Upgrade status (must be one of the following: *downloadaborted, downloadcompleted, downloadstarted, installaborted, installinitiated, installnotstarted, installstarted, none, upgradecancelled, upgradecompleted, upgradestarted*) |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "FirmwareControl.1.status"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "completed"
}
```

<a name="property.downloadsize"></a>
## *downloadsize [<sup>property</sup>](#head.Properties)*

Provides access to the max free space available to download image.

> This property is **read-only**.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | integer | mandatory | Available free space in bytes |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "FirmwareControl.1.downloadsize"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": 315576
}
```

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the FirmwareControl plugin:

FirmwareControl interface events:

| Notification | Description |
| :-------- | :-------- |
| [upgradeprogress](#notification.upgradeprogress) | Notifies progress of upgrade |

<a name="notification.upgradeprogress"></a>
## *upgradeprogress [<sup>notification</sup>](#head.Notifications)*

Notifies progress of upgrade.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.status | string | mandatory | Upgrade status (must be one of the following: *downloadaborted, downloadcompleted, downloadstarted, installaborted, installinitiated, installnotstarted, installstarted, none, upgradecancelled, upgradecompleted, upgradestarted*) |
| params.error | string | mandatory | Reason of error (must be one of the following: *downloaddirectorynotexist, generic, incorrecthash, invalidparameters, invalidrange, invalidstate, noenoughspace, none, operationotsupported, resumenotsupported, timedout, unauthenticated, unavailable, unkown*) |
| params.progress | integer | mandatory | Progress of upgrade (number of bytes transferred during download or percentage of completion during install |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "FirmwareControl.1.register",
  "params": {
    "event": "upgradeprogress",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.upgradeprogress",
  "params": {
    "status": "completed",
    "error": "operationotsupported",
    "progress": 89
  }
}
```

