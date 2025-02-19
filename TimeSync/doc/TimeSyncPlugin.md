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
- [Interfaces](#head.Interfaces)
- [Methods](#head.Methods)
- [Properties](#head.Properties)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the TimeSync plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

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

The Time Sync plugin provides time synchronization functionality from various time sources (e.g. NTP).

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *TimeSync*) |
| classname | string | mandatory | Class name: *TimeSync* |
| locator | string | mandatory | Library name: *libThunderTimeSync.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| deferred | boolean | optional | <sup>*(deprecated)*</sup> Determines if automatic time sync shall be initially disabled. This parameter is deprecated and SubSystemControl could be used instead |
| periodicity | integer | optional | Periodicity of time synchronization (in hours), 0 for one-off synchronization |
| retries | integer | optional | Number of synchronization attempts if the source cannot be reached (may be 0) |
| interval | integer | optional | Time to wait (in milliseconds) before retrying a synchronization attempt after a failure |
| sources | array | mandatory | Time sources |
| sources[#] | string | mandatory | (a time source entry) |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- [TimeSync.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/TimeSync.json) (version 1.0.0) (uncompliant-extended format)

<a name="head.Methods"></a>
# Methods

The following methods are provided by the TimeSync plugin:

TimeSync interface methods:

| Method | Description |
| :-------- | :-------- |
| [synchronize](#method.synchronize) | Synchronizes time |

<a name="method.synchronize"></a>
## *synchronize [<sup>method</sup>](#head.Methods)*

Synchronizes time.

### Description

Use this method to synchronize the system time with the currently configured time source. If automatic time synchronization is initially disabled or stopped, it will be restarted.

### Parameters

This method takes no parameters.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_INPROGRESS``` | Returned when the method is called while previously triggered synchronization is in progress. |
| ```ERROR_INCOMPLETE_CONFIG``` | Returned when the source configuration is missing or invalid. |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TimeSync.1.synchronize"
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

The following properties are provided by the TimeSync plugin:

TimeSync interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [synctime](#property.synctime) | read-only | Most recent synchronized time |
| [time](#property.time) | read/write | Current system time |

<a name="property.synctime"></a>
## *synctime [<sup>property</sup>](#head.Properties)*

Provides access to the most recent synchronized time.

> This property is **read-only**.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | Most recent synchronized time |
| result.time | string | mandatory | Synchronized time (in ISO8601 format); empty string if the time has never been synchronized |
| result?.source | string | optional | The synchronization source e.g. an NTP server |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TimeSync.1.synctime"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "time": "2019-05-07T07:20:26Z",
    "source": "ntp://example.com"
  }
}
```

<a name="property.time"></a>
## *time [<sup>property</sup>](#head.Properties)*

Provides access to the current system time.

### Description

Upon setting this property automatic time synchronization will be stopped. Usage of this property is deprecated and the SubSystem control plugin can be used as an alternative to achieve the same

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | System time (in ISO8601 format) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | System time (in ISO8601 format) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_BAD_REQUEST``` | The time is invalid |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TimeSync.1.time"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "2019-05-07T07:20:26Z"
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TimeSync.1.time",
  "params": "2019-05-07T07:20:26Z"
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

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the TimeSync plugin:

TimeSync interface events:

| Notification | Description |
| :-------- | :-------- |
| [timechange](#notification.timechange) | Signals a time change |

<a name="notification.timechange"></a>
## *timechange [<sup>notification</sup>](#head.Notifications)*

Signals a time change.

### Notification Parameters

This notification carries no parameters.

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TimeSync.1.register",
  "params": {
    "event": "timechange",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.timechange"
}
```

