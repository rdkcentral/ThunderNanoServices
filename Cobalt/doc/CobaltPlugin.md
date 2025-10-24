<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_Cobalt_Plugin"></a>
# Cobalt Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

Cobalt plugin for Thunder framework.

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

This document describes purpose and functionality of the Cobalt plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

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

The Cobalt plugin provides web browsing functionality based on the Cobalt engine.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a id="head_Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *Cobalt*) |
| classname | string | mandatory | Class name: *Cobalt* |
| locator | string | mandatory | Library name: *libThunderCobalt.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| configuration | object | optional | *...* |
| configuration?.url | string | optional | The URL that is loaded upon starting the browser |
| configuration?.width | integer | optional | The width in pixels of the surface to be used by the application |
| configuration?.height | integer | optional | The height in pixels of the surface to be used by the application |
| configuration?.repeatstart | integer | optional | The number of milliseconds a key should be pressed to start reapeating (set max to adhere to Thunder) |
| configuration?.repeatinterval | integer | optional | The number of milliseconds the repeated key is send after it started repeating (set max to adhere to Thunder) |
| configuration?.clientidentifier | string | optional | An identifier, used during the surface creation as additional information |
| configuration?.operatorname | string | optional | The name of the operator that owns the infrastructure on which this device is running |
| configuration?.language | string | optional | The language to be used to for user interaction |
| configuration?.connection | string | optional | The type of connection that is used for internet connectivity (must be one of the following: *cable, wireless*) |
| configuration?.playbackrates | boolean | optional | If enabled, Cobalt supports different rates, otherwise, it supports only 0 and 1 (default: true) |

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- [Cobalt.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/Cobalt.json) (version 1.0.0) (uncompliant-extended format)
- [Browser.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/Browser.json) (version 1.0.0) (uncompliant-extended format)
- [StateControl.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/StateControl.json) (version 1.0.0) (uncompliant-extended format)

<a id="head_Methods"></a>
# Methods

The following methods are provided by the Cobalt plugin:

Built-in methods:

| Method | Description |
| :-------- | :-------- |
| [versions](#method_versions) | Retrieves a list of JSON-RPC interfaces offered by this service |
| [exists](#method_exists) | Checks if a JSON-RPC method or property exists |
| [register](#method_register) | Registers for an asynchronous JSON-RPC notification |
| [unregister](#method_unregister) | Unregisters from an asynchronous JSON-RPC notification |

Browser interface methods:

| Method | Description |
| :-------- | :-------- |
| [delete](#method_delete) | Removes contents of a directory from the persistent storage |

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
  "method": "Cobalt.1.versions"
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "name": "JController",
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

This method will return *True* for the following methods/properties: *url, visibility, fps, state, versions, exists, register, unregister, delete*.

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
  "method": "Cobalt.1.exists",
  "params": {
    "method": "url"
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

This method supports the following event names: *[pageclosure](#notification_pageclosure), [urlchange](#notification_urlchange), [visibilitychange](#notification_visibilitychange), [statechange](#notification_statechange)*.

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
  "method": "Cobalt.1.register",
  "params": {
    "event": "pageclosure",
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

This method supports the following event names: *[pageclosure](#notification_pageclosure), [urlchange](#notification_urlchange), [visibilitychange](#notification_visibilitychange), [statechange](#notification_statechange)*.

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
  "method": "Cobalt.1.unregister",
  "params": {
    "event": "pageclosure",
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

<a id="method_delete"></a>
## *delete [<sup>method</sup>](#head_Methods)*

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
  "method": "Cobalt.1.delete",
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

<a id="head_Properties"></a>
# Properties

The following properties are provided by the Cobalt plugin:

Browser interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [url](#property_url) | read/write | URL loaded in the browser |
| [visibility](#property_visibility) | read/write | Current browser visibility |
| [fps](#property_fps) | read-only | Current number of frames per second the browser is rendering |

StateControl interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [state](#property_state) | read/write | Running state of the service |

<a id="property_url"></a>
## *url [<sup>property</sup>](#head_Properties)*

Provides access to the URL loaded in the browser.

Also see: [urlchange](#event_urlchange)

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | URL loaded in the browser |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | URL loaded in the browser |

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
  "method": "Cobalt.1.url"
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
  "method": "Cobalt.1.url",
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

<a id="property_visibility"></a>
## *visibility [<sup>property</sup>](#head_Properties)*

Provides access to the current browser visibility.

Also see: [visibilitychange](#event_visibilitychange)

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Current browser visibility (must be one of the following: *hidden, visible*) |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Current browser visibility (must be one of the following: *hidden, visible*) |

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
  "method": "Cobalt.1.visibility"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "hidden"
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Cobalt.1.visibility",
  "params": "hidden"
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

<a id="property_fps"></a>
## *fps [<sup>property</sup>](#head_Properties)*

Provides access to the current number of frames per second the browser is rendering.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | integer | mandatory | Current number of frames per second the browser is rendering |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Cobalt.1.fps"
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

<a id="property_state"></a>
## *state [<sup>property</sup>](#head_Properties)*

Provides access to the running state of the service.

Also see: [statechange](#event_statechange)

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Running state of the service (must be one of the following: *resumed, suspended*) |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Running state of the service (must be one of the following: *resumed, suspended*) |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Cobalt.1.state"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "suspended"
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Cobalt.1.state",
  "params": "suspended"
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

The following events are provided by the Cobalt plugin:

Browser interface events:

| Notification | Description |
| :-------- | :-------- |
| [urlchange](#notification_urlchange) | Signals a URL change in the browser |
| [visibilitychange](#notification_visibilitychange) | Signals a visibility change of the browser |

StateControl interface events:

| Notification | Description |
| :-------- | :-------- |
| [statechange](#notification_statechange) | Signals a state change of the service |

<a id="notification_pageclosure"></a>
## *pageclosure [<sup>notification</sup>](#head_Notifications)*

Notifies that the web page requests to close its window.

### Notification Parameters

This notification carries no parameters.

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Cobalt.1.register",
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

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.pageclosure``.

<a id="notification_urlchange"></a>
## *urlchange [<sup>notification</sup>](#head_Notifications)*

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
  "method": "Cobalt.1.register",
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

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.urlchange``.

<a id="notification_visibilitychange"></a>
## *visibilitychange [<sup>notification</sup>](#head_Notifications)*

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
  "method": "Cobalt.1.register",
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

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.visibilitychange``.

<a id="notification_statechange"></a>
## *statechange [<sup>notification</sup>](#head_Notifications)*

Signals a state change of the service.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.suspended | boolean | mandatory | Determines if the service has entered suspended state (true) or resumed state (false) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Cobalt.1.register",
  "params": {
    "event": "statechange",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.statechange",
  "params": {
    "suspended": false
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.statechange``.

