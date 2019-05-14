<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Streamer_Plugin"></a>
# Streamer Plugin

**Version: 1.0**

Streamer functionality for WPEFramework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the Streamer plugin. It includes detailed specification of its configuration, methods provided and notifications sent.

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

The Streamer plugin provides player functionality for live and vod streams.

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
| configuration | object | <sup>*(optional)*</sup>  |
| configuration?.frontends | string | <sup>*(optional)*</sup> number of frontends to be attached |
| configuration?.decoders | number | <sup>*(optional)*</sup> number of decoders to be attached |
| configuration?.scan | boolean | <sup>*(optional)*</sup> Scanning to be done for live stream |

<a name="head.Methods"></a>
# Methods

The following API is provided by the plugin via JSON-RPC:

- [Count](#method.Count)
- [Type](#method.Type)
- [DRM](#method.DRM)
- [State](#method.State)
- [Metadata](#method.Metadata)
- [Speed](#method.Speed)
- [Position](#method.Position)
- [Window](#method.Window)
- [Load](#method.Load)
- [Attach](#method.Attach)
- [Detach](#method.Detach)
- [CreateStream](#method.CreateStream)
- [DeleteStream](#method.DeleteStream)

This API follows the JSON-RPC 2.0 specification. Refer to [[JSON-RPC](#ref.JSON-RPC)] for more information.


<a name="method.Count"></a>
## *Count*

Returns the player numbers in use

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | number | number of player instances are running |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Streamer.1.Count"
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
<a name="method.Type"></a>
## *Type*

Returns the streame type - DVB or VOD

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | number | index of the streamer instance |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | string | Type of the stream (must be one of the following: *Stubbed*, *DVB*, *VOD*) |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Streamer.1.Type", 
    "params": 0
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": "DVB"
}
```
<a name="method.DRM"></a>
## *DRM*

Returns the DRM Type attached with stream

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | number | index of the streamer instance |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | string | DRM/CAS attached with stream (must be one of the following: *UnKnown*, *ClearKey*, *PlayReady*, *Widevine*) |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Streamer.1.DRM", 
    "params": 0
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": "PlayeReady"
}
```
<a name="method.State"></a>
## *State*

Returns the current state of Player

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | number | index of the streamer instance |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | string | Player State (must be one of the following: *Idle*, *Loading*, *Prepared*, *Paused*, *Playing*, *Error*) |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Streamer.1.State", 
    "params": 0
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": "Playing"
}
```
<a name="method.Metadata"></a>
## *Metadata*

Return stream metadata

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | number | index of the streamer instance |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | string |  |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Streamer.1.Metadata", 
    "params": 0
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": "It is based on the stream"
}
```
<a name="method.Speed"></a>
## *Speed*

Set speed of the stream

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params?.index | number | <sup>*(optional)*</sup> index of the streamer instance |
| params?.speed | number | <sup>*(optional)*</sup> speed value to be set |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Streamer.1.Speed", 
    "params": {
        "index": 0, 
        "speed": 0
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
<a name="method.Position"></a>
## *Position*

Set position of the stream

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params?.index | number | <sup>*(optional)*</sup> index of the streamer instance |
| params?.position | number | <sup>*(optional)*</sup> absolute position value to be set |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 5 | ```ERROR_ILLEGAL_STATE``` | Player is invalid state |
| 2 | ```ERROR_UNAVAILABLE``` | Player instance not avaialbe |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Streamer.1.Position", 
    "params": {
        "index": 0, 
        "position": 0
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
<a name="method.Window"></a>
## *Window*

Set geometry value of the screen

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params?.index | number | <sup>*(optional)*</sup> index of the streamer instance |
| params?.geometry | object | <sup>*(optional)*</sup> geometry value of the window |
| params?.geometry?.x | number | <sup>*(optional)*</sup> X co-ordinate |
| params?.geometry?.y | number | <sup>*(optional)*</sup> Y co-ordinate |
| params?.geometry?.width | number | <sup>*(optional)*</sup> Width of the window |
| params?.geometry?.height | number | <sup>*(optional)*</sup> Height of the window |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Streamer.1.Window", 
    "params": {
        "index": 0, 
        "geometry": {
            "x": 0, 
            "y": 0, 
            "width": 0, 
            "height": 0
        }
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
<a name="method.Load"></a>
## *Load*

Load the URL given in the body onto this stream

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | string |  |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 5 | ```ERROR_ILLEGAL_STATE``` | Player is invalid state |
| 23 | ```ERROR_INCOMPLETE_CONFIG``` | Passed in config is invalid |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Streamer.1.Load", 
    "params": ""
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
<a name="method.Attach"></a>
## *Attach*

Attach a decoder to the primer of stream <Number>

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | number | index of the streamer instance |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Streamer.1.Attach", 
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
<a name="method.Detach"></a>
## *Detach*

Detach a decoder to the primer of stream <Number>

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | number | index of the streamer instance |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Streamer.1.Detach", 
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
<a name="method.CreateStream"></a>
## *CreateStream*

Create an instance of a stream of type <Type>, Body return the stream index for reference in the other calls.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | string | Type of the streamer to be created |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | number | index of the streamer |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Streamer.1.CreateStream", 
    "params": "DVB"
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
<a name="method.DeleteStream"></a>
## *DeleteStream*

Delete the given streamer instance

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | number | index of the streamer instance |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Streamer.1.DeleteStream", 
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
<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the plugin, and broadcasted via JSON-RPC to all registered observers. Refer to [[WPEF](#ref.WPEF)] for information on how to register for a notification.

The following notifications are provided by the plugin:


