<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Spark_Plugin"></a>
# Spark Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

A Spark plugin for Thunder framework.

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

This document describes purpose and functionality of the Spark plugin. It includes detailed specification about its configuration, methods and properties provided, as well as notifications sent.

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

The Spark plugin provides web browsing functionality based on the Spark engine.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *Spark*) |
| classname | string | Class name: *Spark* |
| locator | string | Library name: *libWPEFrameworkSpark.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |
| configuration | object | <sup>*(optional)*</sup>  |
| configuration?.url | string | <sup>*(optional)*</sup> The URL that is loaded upon starting the browser |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- [Spark.json](https://github.com/rdkcentral/ThunderInterfaces/tree/master/jsonrpc/Spark.json)
- [Browser.json](https://github.com/rdkcentral/ThunderInterfaces/tree/master/jsonrpc/Browser.json)
- [StateControl.json](https://github.com/rdkcentral/ThunderInterfaces/tree/master/jsonrpc/StateControl.json)

<a name="head.Methods"></a>
# Methods

The following methods are provided by the Spark plugin:

Browser interface methods:

| Method | Description |
| :-------- | :-------- |
| [delete](#method.delete) | Removes contents of a directory from the persistent storage |


<a name="method.delete"></a>
## *delete [<sup>method</sup>](#head.Methods)*

Removes contents of a directory from the persistent storage.

### Description

Use this method to recursively delete contents of a directory

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.path | string | Path to directory (within the persistent storage) to delete contents of |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | The given path was incorrect |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "Spark.1.delete",
    "params": {
        "path": ".cache/wpe/disk-cache"
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

The following properties are provided by the Spark plugin:

Browser interface properties:

| Property | Description |
| :-------- | :-------- |
| [url](#property.url) | URL loaded in the browser |
| [visibility](#property.visibility) | Current browser visibility |
| [fps](#property.fps) <sup>RO</sup> | Current number of frames per second the browser is rendering |

StateControl interface properties:

| Property | Description |
| :-------- | :-------- |
| [state](#property.state) | Running state of the service |


<a name="property.url"></a>
## *url [<sup>property</sup>](#head.Properties)*

Provides access to the URL loaded in the browser.

Also see: [urlchange](#event.urlchange)

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | URL loaded in the browser |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 15 | ```ERROR_INCORRECT_URL``` | Incorrect URL given |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "Spark.1.url"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": "https://www.google.com"
}
```

#### Set Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "Spark.1.url",
    "params": "https://www.google.com"
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

<a name="property.visibility"></a>
## *visibility [<sup>property</sup>](#head.Properties)*

Provides access to the current browser visibility.

Also see: [visibilitychange](#event.visibilitychange)

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | Current browser visibility (must be one of the following: *visible*, *hidden*) |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Returned when the operation is unavailable |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "Spark.1.visibility"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": "visible"
}
```

#### Set Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "Spark.1.visibility",
    "params": "visible"
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

<a name="property.fps"></a>
## *fps [<sup>property</sup>](#head.Properties)*

Provides access to the current number of frames per second the browser is rendering.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | number | Current number of frames per second the browser is rendering |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "Spark.1.fps"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": 30
}
```

<a name="property.state"></a>
## *state [<sup>property</sup>](#head.Properties)*

Provides access to the running state of the service.

Also see: [statechange](#event.statechange)

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | Running state of the service (must be one of the following: *resumed*, *suspended*) |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "Spark.1.state"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": "resumed"
}
```

#### Set Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "Spark.1.state",
    "params": "resumed"
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

The following events are provided by the Spark plugin:

Browser interface events:

| Event | Description |
| :-------- | :-------- |
| [urlchange](#event.urlchange) | Signals a URL change in the browser |
| [visibilitychange](#event.visibilitychange) | Signals a visibility change of the browser |

StateControl interface events:

| Event | Description |
| :-------- | :-------- |
| [statechange](#event.statechange) | Signals a state change of the service |


<a name="event.urlchange"></a>
## *urlchange [<sup>event</sup>](#head.Notifications)*

Signals a URL change in the browser.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.url | string | The URL that has been loaded or requested |
| params.loaded | boolean | Determines if the URL has just been loaded (true) or if URL change has been requested (false) |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.urlchange",
    "params": {
        "url": "https://www.google.com",
        "loaded": false
    }
}
```

<a name="event.visibilitychange"></a>
## *visibilitychange [<sup>event</sup>](#head.Notifications)*

Signals a visibility change of the browser.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.hidden | boolean | Determines if the browser has been hidden (true) or made visible (false) |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.visibilitychange",
    "params": {
        "hidden": false
    }
}
```

<a name="event.statechange"></a>
## *statechange [<sup>event</sup>](#head.Notifications)*

Signals a state change of the service.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.suspended | boolean | Determines if the service has entered suspended state (true) or resumed state (false) |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.statechange",
    "params": {
        "suspended": false
    }
}
```

