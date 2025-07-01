<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_Switch_Board_Plugin"></a>
# Switch Board Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

SwitchBoard plugin for Thunder framework.

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

This document describes purpose and functionality of the SwitchBoard plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

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

This plugin is configured to manage a group of plugins, within which only one plugin can be active.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a id="head_Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *SwitchBoard*) |
| classname | string | mandatory | Class name: *SwitchBoard* |
| locator | string | mandatory | Library name: *libThunderSwitchBoard.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- ISwitchBoard ([ISwitchBoard.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/ISwitchBoard.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

<a id="head_Methods"></a>
# Methods

The following methods are provided by the SwitchBoard plugin:

SwitchBoard interface methods:

| Method | Description |
| :-------- | :-------- |
| [activate](#method_activate) | Activate a plugin by its callsign |
| [deactivate](#method_deactivate) | Deactivate a plugin by its callsign |

<a id="method_activate"></a>
## *activate [<sup>method</sup>](#head_Methods)*

Activate a plugin by its callsign.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.callsign | string | mandatory | The callsign of the plugin to activate |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | Not in a state to allow activation |
| ```ERROR_INPROGRESS``` | Currently processing another request |
| ```ERROR_UNAVAILABLE``` | The plugin is not available for activation |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "SwitchBoard.1.activate",
  "params": {
    "callsign": "WebServer"
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

<a id="method_deactivate"></a>
## *deactivate [<sup>method</sup>](#head_Methods)*

Deactivate a plugin by its callsign.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.callsign | string | mandatory | The callsign of the plugin to deactivate |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | Not in a state to allow deactivation |
| ```ERROR_INPROGRESS``` | Currently processing another request |
| ```ERROR_UNAVAILABLE``` | The plugin is not available for deactivation |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "SwitchBoard.1.deactivate",
  "params": {
    "callsign": "MessageControl"
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

The following properties are provided by the SwitchBoard plugin:

SwitchBoard interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [isactive](#property_isactive) | read-only | Check if a plugin is active |

<a id="property_isactive"></a>
## *isactive [<sup>property</sup>](#head_Properties)*

Provides access to the check if a plugin is active.

> This property is **read-only**.

> The *callsign* parameter shall be passed as the index to the property, i.e. ``isactive@<callsign>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | The callsign of the plugin to check |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | boolean | mandatory | The state of the plugin, true if it is active, false if not |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Callsign not found |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "SwitchBoard.1.isactive@DeviceInfo"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": true
}
```

<a id="head_Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the SwitchBoard plugin:

SwitchBoard interface events:

| Notification | Description |
| :-------- | :-------- |
| [activated](#notification_activated) | Signal which callsign has been switched on |

<a id="notification_activated"></a>
## *activated [<sup>notification</sup>](#head_Notifications)*

Signal which callsign has been switched on.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.callsign | string | mandatory | The callsign of the plugin that has been activated |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "SwitchBoard.1.register",
  "params": {
    "event": "activated",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.activated",
  "params": {
    "callsign": "WebKitBrowser"
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.activated``.

