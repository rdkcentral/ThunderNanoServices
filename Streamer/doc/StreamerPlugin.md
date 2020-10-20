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
- [Methods](#head.Methods)
- [Properties](#head.Properties)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the Streamer plugin. It includes detailed specification about its configuration, methods and properties provided, as well as notifications sent.

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

.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *Streamer*) |
| classname | string | Class name: *Streamer* |
| locator | string | Library name: *libWPEFrameworkStreamer.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |

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
## *create <sup>method</sup>*

Creates a stream instance.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.type | string | Stream type (must be one of the following: *undefined*, *cable*, *handheld*, *satellite*, *terrestrial*, *dab*, *rf*, *unicast*, *multicast*, *ip*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | number | Stream ID |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 30 | ```ERROR_BAD_REQUEST``` | Invalid stream type given |
| 2 | ```ERROR_UNAVAILABLE``` | Fronted of the selected stream type is not available |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
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
    "id": 1234567890,
    "result": 0
}
```

<a name="method.destroy"></a>
## *destroy <sup>method</sup>*

Destroys a stream instance.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.id | number | Stream ID |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
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
    "id": 1234567890,
    "result": null
}
```

<a name="method.load"></a>
## *load <sup>method</sup>*

Loads a source into a stream.

Also see: [statechange](#event.statechange)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.id | number | Stream ID |
| params.location | string | Location of the source to load |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| 15 | ```ERROR_INCORRECT_URL``` | Invalid location given |
| 1 | ```ERROR_GENERAL``` | Undefined loading error |
| 5 | ```ERROR_ILLEGAL_STATE``` | Stream is not in a valid state |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
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
    "id": 1234567890,
    "result": null
}
```

<a name="method.attach"></a>
## *attach <sup>method</sup>*

Attaches a decoder to the streamer.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.id | number | Stream ID |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| 12 | ```ERROR_INPROGRESS``` | Decoder already attached |
| 5 | ```ERROR_ILLEGAL_STATE``` | Stream is not in a valid state |
| 2 | ```ERROR_UNAVAILABLE``` | No free decoders available |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
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
    "id": 1234567890,
    "result": null
}
```

<a name="method.detach"></a>
## *detach <sup>method</sup>*

Detaches a decoder from the streamer.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.id | number | Stream ID |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| 5 | ```ERROR_ILLEGAL_STATE``` | Stream is not in a valid state or decoder not attached |
| 12 | ```ERROR_INPROGRESS``` | Decoder is in use |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
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
    "id": 1234567890,
    "result": null
}
```

<a name="head.Properties"></a>
# Properties

The following properties are provided by the Streamer plugin:

Streamer interface properties:

| Property | Description |
| :-------- | :-------- |
| [speed](#property.speed) | Playback speed |
| [position](#property.position) | Stream position |
| [window](#property.window) | Stream playback window |
| [speeds](#property.speeds) <sup>RO</sup> | Speeds supported by the stream player |
| [streams](#property.streams) <sup>RO</sup> | All created stream instance IDs |
| [type](#property.type) <sup>RO</sup> | Type of a stream |
| [drm](#property.drm) <sup>RO</sup> | DRM type associated with a stream |
| [state](#property.state) <sup>RO</sup> | Current state of a stream |
| [metadata](#property.metadata) <sup>RO</sup> | Metadata associated with the stream |
| [error](#property.error) <sup>RO</sup> | Most recent error code |
| [elements](#property.elements) <sup>RO</sup> | Stream elements |


<a name="property.speed"></a>
## *speed <sup>property</sup>*

Provides access to the playback speed.

### Description

Speed (in percentage)

Also see: [statechange](#event.statechange)

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | number | Speed percentage; e.g.: 0 - pause, 100 - normal playback, -100 - rewind, -200 - reverse at twice the normal speed, 50 - forward at half speed, etc. Must be one of the speeds supported by the player |

> The *stream id* shall be passed as the index to the property, e.g. *Streamer.1.speed@0*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| 5 | ```ERROR_ILLEGAL_STATE``` | Player is not in a valid state or decoder not attached |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Streamer.1.speed@0"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": 100
}
```

#### Set Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Streamer.1.speed@0",
    "params": 100
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

<a name="property.position"></a>
## *position <sup>property</sup>*

Provides access to the stream position.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | number | Position (in milliseconds) |

> The *stream id* shall be passed as the index to the property, e.g. *Streamer.1.position@0*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| 5 | ```ERROR_ILLEGAL_STATE``` | Player is not in a valid state or decoder not attached |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Streamer.1.position@0"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": 60000
}
```

#### Set Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Streamer.1.position@0",
    "params": 60000
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

<a name="property.window"></a>
## *window <sup>property</sup>*

Provides access to the stream playback window.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Geometry of the window |
| (property).x | number | Horizontal position of the window (in pixels) |
| (property).y | number | Vertical position of the window (in pixels) |
| (property).width | number | Width of the window (in pixels) |
| (property).height | number | Height of the window (in pixels) |

> The *stream id* shall be passed as the index to the property, e.g. *Streamer.1.window@0*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| 5 | ```ERROR_ILLEGAL_STATE``` | Player is not in a valid state or decoder not attached |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Streamer.1.window@0"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
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
    "id": 1234567890,
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
    "id": 1234567890,
    "result": "null"
}
```

<a name="property.speeds"></a>
## *speeds <sup>property</sup>*

Provides access to the speeds supported by the stream player.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | Supported speeds (in percentage) |
| (property)[#] | integer | (speeds in percentage) |

> The *stream id* shall be passed as the index to the property, e.g. *Streamer.1.speeds@0*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| 5 | ```ERROR_ILLEGAL_STATE``` | Player is not in a valid state or decoder not attached |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Streamer.1.speeds@0"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
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
## *streams <sup>property</sup>*

Provides access to the all created stream instance IDs.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | Stream IDs |
| (property)[#] | number | (a stream ID) |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Streamer.1.streams"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": [
        0,
        1,
        2,
        3
    ]
}
```

<a name="property.type"></a>
## *type <sup>property</sup>*

Provides access to the type of a stream.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | Stream type (must be one of the following: *undefined*, *cable*, *handheld*, *satellite*, *terrestrial*, *dab*, *rf*, *unicast*, *multicast*, *ip*) |

> The *stream id* shall be passed as the index to the property, e.g. *Streamer.1.type@0*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Streamer.1.type@0"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": "cable"
}
```

<a name="property.drm"></a>
## *drm <sup>property</sup>*

Provides access to the DRM type associated with a stream.

> This property is **read-only**.

Also see: [drmchange](#event.drmchange)

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | DRM used (must be one of the following: *none*, *clearkey*, *playready*, *widevine*, *unknown*) |

> The *stream id* shall be passed as the index to the property, e.g. *Streamer.1.drm@0*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Streamer.1.drm@0"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": "clearkey"
}
```

<a name="property.state"></a>
## *state <sup>property</sup>*

Provides access to the current state of a stream.

> This property is **read-only**.

Also see: [statechange](#event.statechange)

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | Stream state (must be one of the following: *idle*, *loading*, *prepared*, *controlled*, *error*) |

> The *stream id* shall be passed as the index to the property, e.g. *Streamer.1.state@0*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Streamer.1.state@0"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": "controlled"
}
```

<a name="property.metadata"></a>
## *metadata <sup>property</sup>*

Provides access to the metadata associated with the stream.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | Custom implementation-specific metadata |

> The *stream id* shall be passed as the index to the property, e.g. *Streamer.1.metadata@0*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Streamer.1.metadata@0"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": ""
}
```

<a name="property.error"></a>
## *error <sup>property</sup>*

Provides access to the most recent error code.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | number | Custom implementation-specific error code value |

> The *stream id* shall be passed as the index to the property, e.g. *Streamer.1.error@0*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Streamer.1.error@0"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": 0
}
```

<a name="property.elements"></a>
## *elements <sup>property</sup>*

Provides access to the stream elements.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | List of stream elements |
| (property)[#] | object | Stream element |
| (property)[#].type | string | Stream element type (must be one of the following: *video*, *audio*, *subtitles*, *teletext*, *data*) |

> The *stream id* shall be passed as the index to the property, e.g. *Streamer.1.elements@0*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| 2 | ```ERROR_UNAVAILABLE``` | Stream elements retrieval not supported |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Streamer.1.elements@0"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": [
        {
            "type": "video"
        }
    ]
}
```

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the implementation, and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the Streamer plugin:

Streamer interface events:

| Event | Description |
| :-------- | :-------- |
| [statechange](#event.statechange) | Notifies of stream state change |
| [timeupdate](#event.timeupdate) | Notifies of stream position change |
| [stream](#event.stream) | Notifies of a custom stream incident |
| [player](#event.player) | Notifies of a custom player incident |
| [drm](#event.drm) | Notifies of a custom DRM-related incident |


<a name="event.statechange"></a>
## *statechange <sup>event</sup>*

Notifies of stream state change.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.state | string | Stream state (must be one of the following: *idle*, *loading*, *prepared*, *controlled*, *error*) |

> The *Stream ID* shall be passed within the designator, e.g. *0.client.events.1*.

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "0.client.events.1.statechange",
    "params": {
        "state": "controlled"
    }
}
```

<a name="event.timeupdate"></a>
## *timeupdate <sup>event</sup>*

Notifies of stream position change. This event is fired every second to indicate the current stream position. It does not fire if the stream is paused (i.e. speed is set to 0).

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.time | number | Stream position in miliseconds |

> The *Stream ID* shall be passed within the designator, e.g. *0.client.events.1*.

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "0.client.events.1.timeupdate",
    "params": {
        "time": 30000
    }
}
```

<a name="event.stream"></a>
## *stream <sup>event</sup>*

Notifies of a custom stream incident.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.code | number | Implementation-specific incident code |

> The *Stream ID* shall be passed within the designator, e.g. *0.client.events.1*.

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "0.client.events.1.stream",
    "params": {
        "code": 1
    }
}
```

<a name="event.player"></a>
## *player <sup>event</sup>*

Notifies of a custom player incident.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.code | number | Implementation-specific incident code |

> The *Stream ID* shall be passed within the designator, e.g. *0.client.events.1*.

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "0.client.events.1.player",
    "params": {
        "code": 1
    }
}
```

<a name="event.drm"></a>
## *drm <sup>event</sup>*

Notifies of a custom DRM-related incident.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.code | number | Implementation-specific incident code |

> The *Stream ID* shall be passed within the designator, e.g. *0.client.events.1*.

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "0.client.events.1.drm",
    "params": {
        "code": 1
    }
}
```

