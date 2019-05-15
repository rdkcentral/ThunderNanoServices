<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Time_Sync_Plugin"></a>
# Time Sync Plugin

**Version: 1.0**

TimeSync functionality for WPEFramework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the TimeSync plugin. It includes detailed specification of its configuration and methods provided.

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

The Time Sync plugin provides time synchronisation functionality from various time sources (e.g. NTP).

The plugin is designed to be loaded and executed within the WPEFramework. For more information on WPEFramework refer to [[WPEF](#ref.WPEF)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *TimeSync*) |
| classname | string | Class name: *TimeSync* |
| locator | string | Library name: *libWPEFrameworkTimeSync.so* |
| autostart | boolean | Determines if the plugin is to be started automatically along with the framework |
| deferred | boolean | <sup>*(optional)*</sup> Determines if automatic time sync shall be initially disabled |
| periodicity | number | <sup>*(optional)*</sup> Periodicity of time synchronisation (in hours), 0 for one-off synchronisation |
| retries | number | <sup>*(optional)*</sup> Number of synchronisation attempts if the source cannot be reached (may be 0) |
| interval | number | <sup>*(optional)*</sup> Time to wait (in miliseconds) before retrying a synchronisation attempt after a failure |
| sources | array | Time sources |
| sources[#] | string | (a time source entry) |

<a name="head.Methods"></a>
# Methods

The following API is provided by the plugin via JSON-RPC:

- [time](#method.time)
- [synchronize](#method.synchronize)
- [set](#method.set)

This API follows the JSON-RPC 2.0 specification. Refer to [[JSON-RPC](#ref.JSON-RPC)] for more information.


<a name="method.time"></a>
## *time*

Returns the synchronized time

### Description

Use this method to retrieve the last synchronized time.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.time | string | Synchronized time (in ISO8601 format); empty string if the time has never been synchronized |
| result?.source | string | <sup>*(optional)*</sup> The synchronization source e.g. an NTP server |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "TimeSync.1.time"
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": {
        "time": "2019-05-07T07:20:26Z", 
        "source": "ntp://example.com"
    }
}
```
<a name="method.synchronize"></a>
## *synchronize*

Synchronizes time

### Description

Use this method to synchronize the system time with the currently configured time source. If automatic time synchronization is initially disabled or stopped, it will be restarted.

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
| 23 | ```ERROR_INCOMPLETE_CONFIG``` | Returned when the source configuration is missing or invalid. |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "TimeSync.1.synchronize"
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
<a name="method.set"></a>
## *set*

Sets the current time from an external source

### Description

Use this method to set the current system time to an arbitrary value. Automatic synchronization will be stopped. If not already active, the framework's *time* subsystem will also become activated after this call.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params?.time | string | <sup>*(optional)*</sup> New system time (in ISO8601 format); if this parameter is omitted then the *time* subsystem will still become activated but without setting the time (thereby notifying the framework that the time has been set externally); also automatic synchronization will be stopped |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 30 | ```ERROR_BAD_REQUEST``` | Returned when the requested time was invalid |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "TimeSync.1.set", 
    "params": {
        "time": "2019-05-07T07:20:26Z"
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
