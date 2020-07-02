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
- [Methods](#head.Methods)
- [Properties](#head.Properties)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the FirmwareControl plugin. It includes detailed specification of its configuration, methods and properties provided, as well as notifications sent.

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
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | Thunder API Reference |

<a name="head.Description"></a>
# Description

Control Firmware upgrade to the device

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *FirmwareControl*) |
| classname | string | Class name: *FirmwareControl* |
| locator | string | Library name: *libWPEFrameworkFirmwareControl.so* |
| autostart | boolean | Determines if the plugin is to be started automatically along with the framework |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the FirmwareControl plugin:

FirmwareControl interface methods:

| Method | Description |
| :-------- | :-------- |
| [upgrade](#method.upgrade) | Upgrade the device to the given firmware |

<a name="method.upgrade"></a>
## *upgrade <sup>method</sup>*

Upgrade the device to the given firmware.

Also see: [upgradeprogress](#event.upgradeprogress)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.name | string | name of the firmware |
| params?.location | string | <sup>*(optional)*</sup> location/url of the firmware to be upgraded |
| params?.type | string | <sup>*(optional)*</sup> type of the firmware (must be one of the following: *CDL*, *RCDL*) |
| params?.progressinterval | number | <sup>*(optional)*</sup> number of seconds between progress update events (5 seconds, 10 seconds etc). 0 means invoking callback only once to report final upgrade result |
| params?.hmac | string | <sup>*(optional)*</sup> HMAC value of firmare |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 12 | ```ERROR_INPROGRESS``` | Operation in progress |
| 15 | ```ERROR_INCORRECT_URL``` | Invalid location given |
| 2 | ```ERROR_UNAVAILABLE``` | Error in download |
| 30 | ```ERROR_BAD_REQUEST``` | Bad file name given |
| 22 | ```ERROR_UNKNOWN_KEY``` | Bad hash value given |
| 5 | ```ERROR_ILLEGAL_STATE``` | Invalid state of device |
| 14 | ```ERROR_INCORRECT_HASH``` | Incorrect hash given |
| 42 | ```ERROR_UNAUTHENTICATED``` | Authenitcation failed |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "FirmwareControl.1.upgrade",
    "params": {
        "name": "firmware_v.0",
        "location": "http://my.site.com/images",
        "type": "CDL",
        "progressinterval": 10,
        "hmac": "2834e6d07fa4c7778ef7a4e394f38a5c321afbed51d54ad512bd3fffbc7aa5debc"
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

The following properties are provided by the FirmwareControl plugin:

FirmwareControl interface properties:

| Property | Description |
| :-------- | :-------- |
| [status](#property.status) <sup>RO</sup> | Current status of a upgrade |

<a name="property.status"></a>
## *status <sup>property</sup>*

Provides access to the current status of a upgrade.

> This property is **read-only**.

Also see: [upgradeprogress](#event.upgradeprogress)

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | upgrade status (must be one of the following: *none*, *upgradestarted*, *downloadstarted*, *downloadaborted*, *downloadcompleted*, *installnotstarted*, *installaborted*, *installstarted*, *upgradecompleted*, *upgradecancelled*) |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "FirmwareControl.1.status"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": "completed"
}
```
<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the plugin, and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the FirmwareControl plugin:

FirmwareControl interface events:

| Event | Description |
| :-------- | :-------- |
| [upgradeprogress](#event.upgradeprogress) | Notifies progress of upgrade |

<a name="event.upgradeprogress"></a>
## *upgradeprogress <sup>event</sup>*

Notifies progress of upgrade.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.status | string | upgrade status (must be one of the following: *none*, *upgradestarted*, *downloadstarted*, *downloadaborted*, *downloadcompleted*, *installnotstarted*, *installaborted*, *installstarted*, *upgradecompleted*, *upgradecancelled*) |
| params.error | string | reason of error (must be one of the following: *none*, *generic*, *invalidparameters*, *invalidstate*, *operationotsupported*, *incorrecthash*, *unautheticated*, *unavailable*, *timedout*, *unkown*) |
| params.percentage | number |  |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.upgradeprogress",
    "params": {
        "status": "completed",
        "error": "operationotsupported",
        "percentage": 89
    }
}
```
