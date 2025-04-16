<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Streamer_Plugin"></a>
# Streamer Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

Streamer plugin for Thunder framework.

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

This document describes purpose and functionality of the Streamer plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

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

.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *Streamer*) |
| classname | string | mandatory | Class name: *Streamer* |
| locator | string | mandatory | Library name: *libThunderStreamer.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- [Streamer.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/Streamer.json) (version 1.0.0) (uncompliant-extended format)

<a name="head.Methods"></a>
# Methods

The following methods are provided by the Streamer plugin:

Streamer interface methods:

| Method | Description |
| :-------- | :-------- |
| [create](#method.create) | Creates a stream instance |
| [destroy](#method.destroy) | Destroys a stream instance |
| [load](#method.load) | Loads a source into a stream |
| [attach](#method.attach) | Attaches a decoder to the streamer |
| [detach](#method.detach) | Detaches a decoder from the streamer |

<a name="method.create"></a>
## *create [<sup>method</sup>](#head.Methods)*

Creates a stream instance.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.type | string | mandatory | Stream type (must be one of the following: *cable, dab, handheld, ip, multicast, rf, satellite, terrestrial, undefined, unicast*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | integer | mandatory | Stream ID |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_BAD_REQUEST``` | Invalid stream type given |
| ```ERROR_UNAVAILABLE``` | Fronted of the selected stream type is not available |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.create",
  "params": {
    "type": "cable"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": 0
}
```

<a name="method.destroy"></a>
## *destroy [<sup>method</sup>](#head.Methods)*

Destroys a stream instance.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.id | integer | mandatory | Stream ID |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.destroy",
  "params": {
    "id": 0
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

<a name="method.load"></a>
## *load [<sup>method</sup>](#head.Methods)*

Loads a source into a stream.

Also see: [statechange](#event.statechange)

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.id | integer | mandatory | Stream ID |
| params.location | string | mandatory | Location of the source to load |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| ```ERROR_INCORRECT_URL``` | Invalid location given |
| ```ERROR_GENERAL``` | Undefined loading error |
| ```ERROR_ILLEGAL_STATE``` | Stream is not in a valid state |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.load",
  "params": {
    "id": 0,
    "location": "http://example.com/sample.m3u8"
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

<a name="method.attach"></a>
## *attach [<sup>method</sup>](#head.Methods)*

Attaches a decoder to the streamer.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.id | integer | mandatory | Stream ID |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| ```ERROR_INPROGRESS``` | Decoder already attached |
| ```ERROR_ILLEGAL_STATE``` | Stream is not in a valid state |
| ```ERROR_UNAVAILABLE``` | No free decoders available |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.attach",
  "params": {
    "id": 0
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

<a name="method.detach"></a>
## *detach [<sup>method</sup>](#head.Methods)*

Detaches a decoder from the streamer.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.id | integer | mandatory | Stream ID |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| ```ERROR_ILLEGAL_STATE``` | Stream is not in a valid state or decoder not attached |
| ```ERROR_INPROGRESS``` | Decoder is in use |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.detach",
  "params": {
    "id": 0
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

The following properties are provided by the Streamer plugin:

Streamer interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [speed](#property.speed) | read/write | Playback speed |
| [position](#property.position) | read/write | Stream position |
| [window](#property.window) | read/write | Stream playback window |
| [speeds](#property.speeds) | read-only | Speeds supported by the stream player |
| [streams](#property.streams) | read-only | All created stream instance IDs |
| [type](#property.type) | read-only | Type of a stream |
| [drm](#property.drm) | read-only | DRM type associated with a stream |
| [state](#property.state) | read-only | Current state of a stream |
| [metadata](#property.metadata) | read-only | Metadata associated with the stream |
| [error](#property.error) | read-only | Most recent error code |
| [elements](#property.elements) | read-only | Stream elements |

<a name="property.speed"></a>
## *speed [<sup>property</sup>](#head.Properties)*

Provides access to the playback speed.

### Description

Speed (in percentage)

Also see: [statechange](#event.statechange)

> The *stream id* parameter shall be passed as the index to the property, e.g. ``Streamer.1.speed@<stream-id>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| stream-id | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | integer | mandatory | Speed percentage; e.g.: 0 - pause, 100 - normal playback, -100 - rewind, -200 - reverse at twice the normal speed, 50 - forward at half speed, etc. Must be one of the speeds supported by the player |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | integer | mandatory | Speed percentage; e.g.: 0 - pause, 100 - normal playback, -100 - rewind, -200 - reverse at twice the normal speed, 50 - forward at half speed, etc. Must be one of the speeds supported by the player |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| ```ERROR_ILLEGAL_STATE``` | Player is not in a valid state or decoder not attached |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.speed@0"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": 100
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.speed@0",
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

<a name="property.position"></a>
## *position [<sup>property</sup>](#head.Properties)*

Provides access to the stream position.

> The *stream id* parameter shall be passed as the index to the property, e.g. ``Streamer.1.position@<stream-id>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| stream-id | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | integer | mandatory | Position (in milliseconds) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | integer | mandatory | Position (in milliseconds) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| ```ERROR_ILLEGAL_STATE``` | Player is not in a valid state or decoder not attached |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.position@0"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": 60000
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.position@0",
  "params": 60000
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

<a name="property.window"></a>
## *window [<sup>property</sup>](#head.Properties)*

Provides access to the stream playback window.

> The *stream id* parameter shall be passed as the index to the property, e.g. ``Streamer.1.window@<stream-id>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| stream-id | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Geometry of the window |
| (property).x | integer | mandatory | Horizontal position of the window (in pixels) |
| (property).y | integer | mandatory | Vertical position of the window (in pixels) |
| (property).width | integer | mandatory | Width of the window (in pixels) |
| (property).height | integer | mandatory | Height of the window (in pixels) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | Geometry of the window |
| result.x | integer | mandatory | Horizontal position of the window (in pixels) |
| result.y | integer | mandatory | Vertical position of the window (in pixels) |
| result.width | integer | mandatory | Width of the window (in pixels) |
| result.height | integer | mandatory | Height of the window (in pixels) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| ```ERROR_ILLEGAL_STATE``` | Player is not in a valid state or decoder not attached |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.window@0"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "x": 0,
    "y": 0,
    "width": 1080,
    "height": 720
  }
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.window@0",
  "params": {
    "x": 0,
    "y": 0,
    "width": 1080,
    "height": 720
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

<a name="property.speeds"></a>
## *speeds [<sup>property</sup>](#head.Properties)*

Provides access to the speeds supported by the stream player.

> This property is **read-only**.

> The *stream id* parameter shall be passed as the index to the property, e.g. ``Streamer.1.speeds@<stream-id>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| stream-id | string | mandatory | *...* |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | Supported speeds (in percentage) |
| result[#] | integer | mandatory | (speeds in percentage) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| ```ERROR_ILLEGAL_STATE``` | Player is not in a valid state or decoder not attached |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.speeds@0"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    0,
    100,
    -100,
    200,
    -200,
    400,
    -400
  ]
}
```

<a name="property.streams"></a>
## *streams [<sup>property</sup>](#head.Properties)*

Provides access to the all created stream instance IDs.

> This property is **read-only**.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | Stream IDs |
| result[#] | integer | mandatory | (a stream ID) |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.streams"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    0,
    1,
    2,
    3
  ]
}
```

<a name="property.type"></a>
## *type [<sup>property</sup>](#head.Properties)*

Provides access to the type of a stream.

> This property is **read-only**.

> The *stream id* parameter shall be passed as the index to the property, e.g. ``Streamer.1.type@<stream-id>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| stream-id | string | mandatory | *...* |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | Stream type (must be one of the following: *cable, dab, handheld, ip, multicast, rf, satellite, terrestrial, undefined, unicast*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.type@0"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "cable"
}
```

<a name="property.drm"></a>
## *drm [<sup>property</sup>](#head.Properties)*

Provides access to the DRM type associated with a stream.

> This property is **read-only**.

Also see: [drmchange](#event.drmchange)

> The *stream id* parameter shall be passed as the index to the property, e.g. ``Streamer.1.drm@<stream-id>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| stream-id | string | mandatory | *...* |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | DRM used (must be one of the following: *clearkey, none, playready, unknown, widevine*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.drm@0"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "clearkey"
}
```

<a name="property.state"></a>
## *state [<sup>property</sup>](#head.Properties)*

Provides access to the current state of a stream.

> This property is **read-only**.

Also see: [statechange](#event.statechange)

> The *stream id* parameter shall be passed as the index to the property, e.g. ``Streamer.1.state@<stream-id>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| stream-id | string | mandatory | *...* |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | Stream state (must be one of the following: *controlled, error, idle, loading, prepared*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.state@0"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "controlled"
}
```

<a name="property.metadata"></a>
## *metadata [<sup>property</sup>](#head.Properties)*

Provides access to the metadata associated with the stream.

> This property is **read-only**.

> The *stream id* parameter shall be passed as the index to the property, e.g. ``Streamer.1.metadata@<stream-id>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| stream-id | string | mandatory | *...* |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | Custom implementation-specific metadata |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.metadata@0"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "..."
}
```

<a name="property.error"></a>
## *error [<sup>property</sup>](#head.Properties)*

Provides access to the most recent error code.

> This property is **read-only**.

> The *stream id* parameter shall be passed as the index to the property, e.g. ``Streamer.1.error@<stream-id>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| stream-id | string | mandatory | *...* |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | integer | mandatory | Custom implementation-specific error code value |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.error@0"
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

<a name="property.elements"></a>
## *elements [<sup>property</sup>](#head.Properties)*

Provides access to the stream elements.

> This property is **read-only**.

> The *stream id* parameter shall be passed as the index to the property, e.g. ``Streamer.1.elements@<stream-id>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| stream-id | string | mandatory | *...* |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | List of stream elements |
| result[#] | object | mandatory | Stream element |
| result[#].type | string | mandatory | Stream element type (must be one of the following: *audio, data, subtitles, teletext, video*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| ```ERROR_UNAVAILABLE``` | Stream elements retrieval not supported |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.elements@0"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "type": "video"
    }
  ]
}
```

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the Streamer plugin:

Streamer interface events:

| Notification | Description |
| :-------- | :-------- |
| [statechange](#notification.statechange) | Notifies of stream state change |
| [timeupdate](#notification.timeupdate) | Notifies of stream position change |
| [stream](#notification.stream) | Notifies of a custom stream incident |
| [player](#notification.player) | Notifies of a custom player incident |
| [drm](#notification.drm) | Notifies of a custom DRM-related incident |

<a name="notification.statechange"></a>
## *statechange [<sup>notification</sup>](#head.Notifications)*

Notifies of stream state change.

### Parameters

> The *Stream ID* parameter shall be passed within the client ID during registration, e.g. *0.myid*

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.state | string | mandatory | Stream state (must be one of the following: *controlled, error, idle, loading, prepared*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.register",
  "params": {
    "event": "statechange",
    "id": "0.myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "0.myid.statechange",
  "params": {
    "state": "controlled"
  }
}
```

> The *Stream ID* parameter is passed within the designator, e.g. *0.myid.statechange*.

<a name="notification.timeupdate"></a>
## *timeupdate [<sup>notification</sup>](#head.Notifications)*

Notifies of stream position change. This event is fired every second to indicate the current stream position. It does not fire if the stream is paused (i.e. speed is set to 0).

### Parameters

> The *Stream ID* parameter shall be passed within the client ID during registration, e.g. *0.myid*

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.time | integer | mandatory | Stream position in miliseconds |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.register",
  "params": {
    "event": "timeupdate",
    "id": "0.myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "0.myid.timeupdate",
  "params": {
    "time": 30000
  }
}
```

> The *Stream ID* parameter is passed within the designator, e.g. *0.myid.timeupdate*.

<a name="notification.stream"></a>
## *stream [<sup>notification</sup>](#head.Notifications)*

Notifies of a custom stream incident.

### Parameters

> The *Stream ID* parameter shall be passed within the client ID during registration, e.g. *0.myid*

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.code | integer | mandatory | Implementation-specific incident code |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.register",
  "params": {
    "event": "stream",
    "id": "0.myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "0.myid.stream",
  "params": {
    "code": 1
  }
}
```

> The *Stream ID* parameter is passed within the designator, e.g. *0.myid.stream*.

<a name="notification.player"></a>
## *player [<sup>notification</sup>](#head.Notifications)*

Notifies of a custom player incident.

### Parameters

> The *Stream ID* parameter shall be passed within the client ID during registration, e.g. *0.myid*

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.code | integer | mandatory | Implementation-specific incident code |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.register",
  "params": {
    "event": "player",
    "id": "0.myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "0.myid.player",
  "params": {
    "code": 1
  }
}
```

> The *Stream ID* parameter is passed within the designator, e.g. *0.myid.player*.

<a name="notification.drm"></a>
## *drm [<sup>notification</sup>](#head.Notifications)*

Notifies of a custom DRM-related incident.

### Parameters

> The *Stream ID* parameter shall be passed within the client ID during registration, e.g. *0.myid*

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.code | integer | mandatory | Implementation-specific incident code |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Streamer.1.register",
  "params": {
    "event": "drm",
    "id": "0.myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "0.myid.drm",
  "params": {
    "code": 1
  }
}
```

> The *Stream ID* parameter is passed within the designator, e.g. *0.myid.drm*.

