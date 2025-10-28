<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_Time_Sync_Plugin"></a>
# Time Sync Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

TimeSync plugin for Thunder framework.

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

This document describes purpose and functionality of the TimeSync plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

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

The Time Sync plugin provides time synchronization functionality from various time sources (e.g. NTP).

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a id="head_Configuration"></a>
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

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- ITimeSync ([ITimeSync.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/ITimeSync.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

<a id="head_Methods"></a>
# Methods

The following methods are provided by the TimeSync plugin:

Built-in methods:

| Method | Description |
| :-------- | :-------- |
| [versions](#method_versions) | Retrieves a list of JSON-RPC interfaces offered by this service |
| [exists](#method_exists) | Checks if a JSON-RPC method or property exists |
| [register](#method_register) | Registers for an asynchronous JSON-RPC notification |
| [unregister](#method_unregister) | Unregisters from an asynchronous JSON-RPC notification |

TimeSync interface methods:

| Method | Description |
| :-------- | :-------- |
| [synchronize](#method_synchronize) <sup>deprecated</sup> | Synchronizes time |

<a id="method_versions"></a>
## *versions [<sup>method</sup>](#head_Methods)*

Retrieves a list of JSON-RPC interfaces offered by this service.

### Parameters

This method takes no parameters.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | A list ofsinterfaces with their version numbers<br>*Array length must be at most 255 elements.* |
| result[#] | object | mandatory | *...* |
| result[#].name | string | mandatory | Name of the interface |
| result[#].major | integer | mandatory | Major part of version number |
| result[#].minor | integer | mandatory | Minor part of version number |
| result[#].patch | integer | mandatory | Patch part of version version number |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TimeSync.1.versions"
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "name": "JMyInterface",
      "major": 1,
      "minor": 0,
      "patch": 0
    }
  ]
}
```

<a id="method_exists"></a>
## *exists [<sup>method</sup>](#head_Methods)*

Checks if a JSON-RPC method or property exists.

### Description

This method will return *True* for the following methods/properties: *synctime, time, versions, exists, register, unregister, synchronize*.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.method | string | mandatory | Name of the method or property to look up |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | boolean | mandatory | Denotes if the method exists or not |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TimeSync.1.exists",
  "params": {
    "method": "synctime"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": false
}
```

<a id="method_register"></a>
## *register [<sup>method</sup>](#head_Methods)*

Registers for an asynchronous JSON-RPC notification.

### Description

This method supports the following event names: *[timechanged](#notification_timechanged)*.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.event | string | mandatory | Name of the notification to register for |
| params.id | string | mandatory | Client identifier |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_FAILED_REGISTERED``` | Failed to register for the notification (e.g. already registered) |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TimeSync.1.register",
  "params": {
    "event": "timechanged",
    "id": "myapp"
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

<a id="method_unregister"></a>
## *unregister [<sup>method</sup>](#head_Methods)*

Unregisters from an asynchronous JSON-RPC notification.

### Description

This method supports the following event names: *[timechanged](#notification_timechanged)*.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.event | string | mandatory | Name of the notification to register for |
| params.id | string | mandatory | Client identifier |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_FAILED_UNREGISTERED``` | Failed to unregister from the notification (e.g. not yet registered) |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TimeSync.1.unregister",
  "params": {
    "event": "timechanged",
    "id": "myapp"
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

<a id="method_synchronize"></a>
## *synchronize [<sup>method</sup>](#head_Methods)*

Synchronizes time.

> ``synchronize`` is an alternative name for this method. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Parameters

This method takes no parameters.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_INPROGRESS``` | Returned when the method is called while previously triggered synchronization is in progress |
| ```ERROR_INCOMPLETE_CONFIG``` | Returned when the source configuration is missing or invalid |

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

<a id="head_Properties"></a>
# Properties

The following properties are provided by the TimeSync plugin:

TimeSync interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [synctime](#property_synctime) | read-only | Time of the most recent synchronization |
| [time](#property_time) <sup>deprecated</sup> | read/write | Current system time |

<a id="property_synctime"></a>
## *synctime [<sup>property</sup>](#head_Properties)*

Provides access to the time of the most recent synchronization.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Time of the most recent synchronization |
| (property).time | string | mandatory | Synchronized time (in ISO8601 format); empty string if the time has never been synchronized |
| (property)?.source | string | optional | The synchronization source like an NTP server |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | There has not been any successful time synchronization yet |

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

<a id="property_time"></a>
## *time [<sup>property</sup>](#head_Properties)*

Provides access to the current system time.

> ``time`` is an alternative name for this property. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Current system time |
| (property).value | string | mandatory | *...* |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | System time in ISO8601 format |

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
  "params": {
    "value": "..."
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

The following events are provided by the TimeSync plugin:

TimeSync interface events:

| Notification | Description |
| :-------- | :-------- |
| [timechanged](#notification_timechanged) / [timechange](#notification_timechanged) | Signals a time change |

<a id="notification_timechanged"></a>
## *timechanged [<sup>notification</sup>](#head_Notifications)*

Signals a time change.

> ``timechange`` is an alternative name for this notification. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

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
    "event": "timechanged",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.timechanged"
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.timechanged``.

