<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Streamer_Plugin"></a>
# Streamer Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

Streamer plugin for WPEFramework.

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

This document describes purpose and functionality of the Streamer plugin. It includes detailed specification of its configuration, methods and properties provided, as well as notifications sent.

<a name="head.Case_Sensitivity"></a>
## Case Sensitivity

All identifiers on the interface described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

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
| <a name="ref.WPEF">[WPEF](https://github.com/WebPlatformForEmbedded/WPEFramework/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | WPEFramework API Reference |

<a name="head.Description"></a>
# Description



The plugin is designed to be loaded and executed within the WPEFramework. For more information on WPEFramework refer to [[WPEF](#ref.WPEF)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *Streamer*) |
| classname | string | Class name: *Streamer* |
| locator | string | Library name: *libWPEFrameworkStreamer.so* |
| autostart | boolean | Determines if the plugin is to be started automatically along with the framework |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the Streamer plugin:

Streamer interface methods:

| Method | Description |
| :-------- | :-------- |
| [status](#method.status) | Retrieves the status of a stream |
| [create](#method.create) | Creates a stream instance |
| [destroy](#method.destroy) | Destroys a stream instance |
| [load](#method.load) | Loads a source into a stream |
| [attach](#method.attach) | Attaches a decoder to the streamer |
| [detach](#method.detach) | Detaches a decoder from the streamer |

<a name="method.status"></a>
## *status <sup>method</sup>*

Retrieves the status of a stream.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | number | ID of the streamer instance |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.type | string | Stream type (must be one of the following: *stubbed*, *dvb*, *atsc*, *vod*) |
| result.state | string | Stream state (must be one of the following: *idle*, *loading*, *prepared*, *paused*, *playing*, *error*) |
| result.metadata | string | Custom metadata associated with the stream |
| result.drm | string | DRM used (must be one of the following: *unknown*, *clearkey*, *playready*, *widevine*) |
| result?.position | number | <sup>*(optional)*</sup> Stream position (in milliseconds) |
| result?.window | object | <sup>*(optional)*</sup> Geometry of the window |
| result?.window.x | number | Horizontal position of the window (in pixels) |
| result?.window.y | number | Vertical position of the window (in pixels) |
| result?.window.width | number | Width of the window (in pixels) |
| result?.window.height | number | Height of the window (in pixels) |

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
    "method": "Streamer.1.status", 
    "params": 0
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": {
        "type": "vod", 
        "state": "playing", 
        "metadata": "", 
        "drm": "clearkey", 
        "position": 60000, 
        "window": {
            "x": 0, 
            "y": 0, 
            "width": 1080, 
            "height": 720
        }
    }
}
```
<a name="method.create"></a>
## *create <sup>method</sup>*

Creates a stream instance.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.type | string | Stream type (must be one of the following: *stubbed*, *dvb*, *atsc*, *vod*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | number | Streamer ID |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 30 | ```ERROR_BAD_REQUEST``` | Invalid stream type given |
| 2 | ```ERROR_UNAVAILABLE``` | Streamer instance is not available |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Streamer.1.create", 
    "params": {
        "type": "vod"
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
| params | number | ID of the streamer instance to be destroyed |

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
    "params": 0
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

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params?.index | number | <sup>*(optional)*</sup> ID of the streamer instance |
| params.location | string | Location of the source to load |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| 30 | ```ERROR_BAD_REQUEST``` | Invalid location given |
| 5 | ```ERROR_ILLEGAL_STATE``` | Player is not in a valid state |
| 2 | ```ERROR_UNAVAILABLE``` | Player instance is not available |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Streamer.1.load", 
    "params": {
        "index": 0, 
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
| params | number | ID of the streamer instance to attach a decoder to |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| 12 | ```ERROR_INPROGRESS``` | Decoder already attached |
| 5 | ```ERROR_ILLEGAL_STATE``` | Stream not prepared |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Streamer.1.attach", 
    "params": 0
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

Detaches a decoder from the streamer

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | number | ID of the streamer instance to detach the decoder from |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| 5 | ```ERROR_ILLEGAL_STATE``` | Decoder not attached to the stream |
| 12 | ```ERROR_INPROGRESS``` | Decoder is in use |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Streamer.1.detach", 
    "params": 0
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
| [speeds](#property.speeds) <sup>RO</sup> | Retrieves the speeds supported by the player |
| [streams](#property.streams) <sup>RO</sup> | Retrieves all created stream instance IDs |
| [type](#property.type) <sup>RO</sup> | Retrieves the streame type - DVB, ATSC or VOD |
| [drm](#property.drm) <sup>RO</sup> | Retrieves the DRM Type attached with stream |
| [state](#property.state) <sup>RO</sup> | Retrieves the current state of Player |

<a name="property.speed"></a>
## *speed <sup>property</sup>*

Provides access to the playback speed..

### Description

Speed in percentage, -200, -100, 0, 100, 200, 400 etc

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | number | Speed to set; 0 - pause, 100 - normal playback forward, -100 - normal playback back, -200 - reverse at twice the speed, 50 - forward at half speed |

> The *Streamer ID* shall be passed as the index to the property, e.g. *Streamer.1.speed@0*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| 30 | ```ERROR_BAD_REQUEST``` | Invalid speed given |
| 5 | ```ERROR_ILLEGAL_STATE``` | Player is not in a valid state or decoder not attached |
| 2 | ```ERROR_UNAVAILABLE``` | Player instance is not available |

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

Provides access to the stream position..

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | number | Position to set (in milliseconds) |

> The *Streamer ID* shall be passed as the index to the property, e.g. *Streamer.1.position@0*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| 30 | ```ERROR_BAD_REQUEST``` | Invalid position given |
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

Provides access to the stream playback window..

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Stream playback window. |
| (property).window | object | Geometry of the window |
| (property).window.x | number | Horizontal position of the window (in pixels) |
| (property).window.y | number | Vertical position of the window (in pixels) |
| (property).window.width | number | Width of the window (in pixels) |
| (property).window.height | number | Height of the window (in pixels) |

> The *Streamer ID* shall be passed as the index to the property, e.g. *Streamer.1.window@0*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| 30 | ```ERROR_BAD_REQUEST``` | Invalid window geometry given |
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
        "window": {
            "x": 0, 
            "y": 0, 
            "width": 1080, 
            "height": 720
        }
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
        "window": {
            "x": 0, 
            "y": 0, 
            "width": 1080, 
            "height": 720
        }
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

Provides access to the retrieves the speeds supported by the player.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | Supported streams in percentage, 100, 200, 400, .. |
| (property)[#] | integer | (Speeds in percentage) |

> The *Streamer ID* shall be passed as the index to the property, e.g. *Streamer.1.speeds@0*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown stream ID given |
| 5 | ```ERROR_ILLEGAL_STATE``` | Decoder not attached to the stream |

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

Provides access to the retrieves all created stream instance IDs..

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | Streamer IDs |
| (property)[#] | number | (a streamer ID) |

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

Provides access to the retrieves the streame type - DVB, ATSC or VOD.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Retrieves the streame type - DVB, ATSC or VOD |
| (property).stream | string | Stream type (must be one of the following: *stubbed*, *dvb*, *atsc*, *vod*) |

> The *Streamer ID* shall be passed as the index to the property, e.g. *Streamer.1.type@0*.

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
    "result": {
        "stream": "vod"
    }
}
```
<a name="property.drm"></a>
## *drm <sup>property</sup>*

Provides access to the retrieves the DRM Type attached with stream.

> This property is **read-only**.

Also see: [drmchange](#event.drmchange)

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Retrieves the DRM Type attached with stream |
| (property).drm | string | DRM used (must be one of the following: *unknown*, *clearkey*, *playready*, *widevine*) |

> The *Streamer ID* shall be passed as the index to the property, e.g. *Streamer.1.drm@0*.

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
    "result": {
        "drm": "clearkey"
    }
}
```
<a name="property.state"></a>
## *state <sup>property</sup>*

Provides access to the retrieves the current state of Player.

> This property is **read-only**.

Also see: [statechange](#event.statechange)

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Retrieves the current state of Player |
| (property).state | string | Stream state (must be one of the following: *idle*, *loading*, *prepared*, *paused*, *playing*, *error*) |

> The *Streamer ID* shall be passed as the index to the property, e.g. *Streamer.1.state@0*.

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
    "result": {
        "state": "playing"
    }
}
```
<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the plugin, and broadcasted via JSON-RPC to all registered observers. Refer to [[WPEF](#ref.WPEF)] for information on how to register for a notification.

The following events are provided by the Streamer plugin:

Streamer interface events:

| Event | Description |
| :-------- | :-------- |
| [statechange](#event.statechange) | Notifies of stream state change |
| [drmchange](#event.drmchange) | Notifies of stream DRM system change |
| [timeupdate](#event.timeupdate) | Event fired to indicate the position in the stream |

<a name="event.statechange"></a>
## *statechange <sup>event</sup>*

Notifies of stream state change. ID of the streamer instance shall be passed within the designator.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.state | string | Stream state (must be one of the following: *idle*, *loading*, *prepared*, *paused*, *playing*, *error*) |

> The *Streamer ID* shall be passed within the designator, e.g. *0.client.events.1*.

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "0.client.events.1.statechange", 
    "params": {
        "state": "playing"
    }
}
```
<a name="event.drmchange"></a>
## *drmchange <sup>event</sup>*

Notifies of stream DRM system change. ID of the streamer instance shall be passed within the designator.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.drm | string | DRM used (must be one of the following: *unknown*, *clearkey*, *playready*, *widevine*) |

> The *Streamer ID* shall be passed within the designator, e.g. *0.client.events.1*.

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "0.client.events.1.drmchange", 
    "params": {
        "drm": "clearkey"
    }
}
```
<a name="event.timeupdate"></a>
## *timeupdate <sup>event</sup>*

Event fired to indicate the position in the stream. This event is fired every second to indicate that the stream has progressed by a second, and event does not fire, if the stream is in paused state

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.time | number | Time in seconds |

> The *Streamer ID* shall be passed within the designator, e.g. *0.client.events.1*.

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "0.client.events.1.timeupdate", 
    "params": {
        "time": 30
    }
}
```
