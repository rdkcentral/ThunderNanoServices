<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Spark_Plugin"></a>
# Spark Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

Spark plugin for Thunder framework.

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

This document describes purpose and functionality of the Spark plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

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

The Spark plugin provides web browsing functionality based on the Spark engine.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *Spark*) |
| classname | string | mandatory | Class name: *Spark* |
| locator | string | mandatory | Library name: *libThunderSpark.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| configuration | object | optional | *...* |
| configuration?.url | string | optional | The URL that is loaded upon starting the browser |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- [Browser.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/Browser.json) (version 1.0.0) (uncompliant-extended format)

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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.path | string | mandatory | Path to directory (within the persistent storage) to delete contents of |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | The given path was incorrect |

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

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [url](#property.url) | read/write | URL loaded in the browser |
| [visibility](#property.visibility) | read/write | Current browser visibility |
| [fps](#property.fps) | read-only | Current number of frames per second the browser is rendering |

<a name="property.url"></a>
## *url [<sup>property</sup>](#head.Properties)*

Provides access to the URL loaded in the browser.

Also see: [urlchange](#event.urlchange)

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | URL loaded in the browser |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | URL loaded in the browser |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_INCORRECT_URL``` | Incorrect URL given |

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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Current browser visibility (must be one of the following: *hidden, visible*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | Current browser visibility (must be one of the following: *hidden, visible*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Returned when the operation is unavailable |

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

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | integer | mandatory | Current number of frames per second the browser is rendering |

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

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the Spark plugin:

Browser interface events:

| Notification | Description |
| :-------- | :-------- |
| [urlchange](#notification.urlchange) | Signals a URL change in the browser |
| [visibilitychange](#notification.visibilitychange) | Signals a visibility change of the browser |
| [pageclosure](#notification.pageclosure) | Notifies that the web page requests to close its window |

<a name="notification.urlchange"></a>
## *urlchange [<sup>notification</sup>](#head.Notifications)*

Signals a URL change in the browser.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.url | string | mandatory | The URL that has been loaded or requested |
| params.loaded | boolean | mandatory | Determines if the URL has just been loaded (true) or if URL change has been requested (false) (default: *True*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Spark.1.register",
  "params": {
    "event": "urlchange",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.urlchange",
  "params": {
    "url": "https://www.google.com",
    "loaded": false
  }
}
```

<a name="notification.visibilitychange"></a>
## *visibilitychange [<sup>notification</sup>](#head.Notifications)*

Signals a visibility change of the browser.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.hidden | boolean | mandatory | Determines if the browser has been hidden (true) or made visible (false) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Spark.1.register",
  "params": {
    "event": "visibilitychange",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.visibilitychange",
  "params": {
    "hidden": false
  }
}
```

<a name="notification.pageclosure"></a>
## *pageclosure [<sup>notification</sup>](#head.Notifications)*

Notifies that the web page requests to close its window.

### Notification Parameters

This notification carries no parameters.

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Spark.1.register",
  "params": {
    "event": "pageclosure",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.pageclosure"
}
```

