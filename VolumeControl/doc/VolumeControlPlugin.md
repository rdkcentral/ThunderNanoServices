<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_Volume_Control_Plugin"></a>
# Volume Control Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::white_circle:**

VolumeControl plugin for Thunder framework.

### Table of Contents

- [Introduction](#head_Introduction)
- [Description](#head_Description)
- [Configuration](#head_Configuration)
- [Interfaces](#head_Interfaces)
- [Properties](#head_Properties)
- [Notifications](#head_Notifications)

<a id="head_Introduction"></a>
# Introduction

<a id="head_Scope"></a>
## Scope

This document describes purpose and functionality of the VolumeControl plugin. It includes detailed specification about its configuration, properties provided and notifications sent.

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

The Volume Control plugin allows to manage system's audio volume.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a id="head_Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *VolumeControl*) |
| classname | string | mandatory | Class name: *VolumeControl* |
| locator | string | mandatory | Library name: *libWPEVolumeControl.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- IVolumeControl ([IVolumeControl.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IVolumeControl.h)) (version 1.0.0) (uncompliant-extended format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

<a id="head_Properties"></a>
# Properties

The following properties are provided by the VolumeControl plugin:

VolumeControl interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [muted](#property_muted) | read/write | Audio mute state |
| [volume](#property_volume) | read/write | Audio volume level |

<a id="property_muted"></a>
## *muted [<sup>property</sup>](#head_Properties)*

Provides access to the audio mute state.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | boolean | mandatory | Mute state (true: muted, false: un-muted) |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | boolean | mandatory | Mute state (true: muted, false: un-muted) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_GENERAL``` | Failed to set/retrieve muting state |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "VolumeControl.1.muted"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": false
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "VolumeControl.1.muted",
  "params": false
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

<a id="property_volume"></a>
## *volume [<sup>property</sup>](#head_Properties)*

Provides access to the audio volume level.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | integer | mandatory | Volume level in percent |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | integer | mandatory | Volume level in percent |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_GENERAL``` | Failed to set/retrieve audio volume |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "VolumeControl.1.volume"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": 0
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "VolumeControl.1.volume",
  "params": 100
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

The following events are provided by the VolumeControl plugin:

VolumeControl interface events:

| Notification | Description |
| :-------- | :-------- |
| [volume](#notification_volume) | Signals volume change |
| [muted](#notification_muted) | Signals mute state change |

<a id="notification_volume"></a>
## *volume [<sup>notification</sup>](#head_Notifications)*

Signals volume change.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.volume | integer | mandatory | New bolume level in percent |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "VolumeControl.1.register",
  "params": {
    "event": "volume",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.volume",
  "params": {
    "volume": 100
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.volume``.

<a id="notification_muted"></a>
## *muted [<sup>notification</sup>](#head_Notifications)*

Signals mute state change.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.muted | boolean | mandatory | New mute state (true: muted, false: un-muted) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "VolumeControl.1.register",
  "params": {
    "event": "muted",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.muted",
  "params": {
    "muted": false
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.muted``.

