<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Time_Sync_Plugin"></a>
# Time Sync Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

TimeSync plugin for Thunder framework.

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

This document describes purpose and functionality of the TimeSync plugin. It includes detailed specification about its configuration, methods and properties provided, as well as notifications sent.

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

The Time Sync plugin provides time synchronization functionality from various time sources (e.g. NTP).

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *TimeSync*) |
| classname | string | Class name: *TimeSync* |
| locator | string | Library name: *libWPEFrameworkTimeSync.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |
| deferred | boolean | <sup>*(optional)*</sup> Determines if automatic time sync shall be initially disabled |
| periodicity | number | <sup>*(optional)*</sup> Periodicity of time synchronization (in hours), 0 for one-off synchronization |
| retries | number | <sup>*(optional)*</sup> Number of synchronization attempts if the source cannot be reached (may be 0) |
| interval | number | <sup>*(optional)*</sup> Time to wait (in milliseconds) before retrying a synchronization attempt after a failure |
| sources | array | Time sources |
| sources[#] | string | (a time source entry) |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the TimeSync plugin:

TimeSync interface methods:

| Method | Description |
| :-------- | :-------- |
| [synchronize](#method.synchronize) | Synchronizes time |


<a name="method.synchronize"></a>
## *synchronize <sup>method</sup>*

Synchronizes time.

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
| 12 | ```ERROR_INPROGRESS``` | Returned when the method is called while previously triggered synchronization is in progress. |
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

<a name="head.Properties"></a>
# Properties

The following properties are provided by the TimeSync plugin:

TimeSync interface properties:

| Property | Description |
| :-------- | :-------- |
| [synctime](#property.synctime) <sup>RO</sup> | Most recent synchronized time |
| [time](#property.time) | Current system time |


<a name="property.synctime"></a>
## *synctime <sup>property</sup>*

Provides access to the most recent synchronized time.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Most recent synchronized time |
| (property).time | string | Synchronized time (in ISO8601 format); empty string if the time has never been synchronized |
| (property)?.source | string | <sup>*(optional)*</sup> The synchronization source e.g. an NTP server |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "TimeSync.1.synctime"
}
```

#### Get Response

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

<a name="property.time"></a>
## *time <sup>property</sup>*

Provides access to the current system time.

### Description

Upon setting this property automatic time synchronization will be stopped. If not already active, the framework's *time* subsystem will become activated. If the property is set empty then the *time* subsystem will still become activated but without setting the time (thereby notifying the framework that the time has been set externally).

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | System time (in ISO8601 format) |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 30 | ```ERROR_BAD_REQUEST``` | The time is invalid |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "TimeSync.1.time"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": "2019-05-07T07:20:26Z"
}
```

#### Set Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "TimeSync.1.time",
    "params": "2019-05-07T07:20:26Z"
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

The following events are provided by the TimeSync plugin:

TimeSync interface events:

| Event | Description |
| :-------- | :-------- |
| [timechange](#event.timechange) | Signals a time change |


<a name="event.timechange"></a>
## *timechange <sup>event</sup>*

Signals a time change.

### Parameters

This event carries no parameters.

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.timechange"
}
```

