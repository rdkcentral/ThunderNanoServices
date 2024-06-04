<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Bluetooth_Audio_Plugin"></a>
# Bluetooth Audio Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

BluetoothAudio plugin for Thunder framework.

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

This document describes purpose and functionality of the BluetoothAudio plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

<a name="head.Case_Sensitivity"></a>
## Case Sensitivity

All identifiers of the interfaces described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a name="head.Acronyms,_Abbreviations_and_Terms"></a>
## Acronyms, Abbreviations and Terms

The table below provides and overview of acronyms used in this document and their definitions.

| Acronym | Description |
| :-------- | :-------- |
| <a name="acronym.API">API</a> | Application Programming Interface |
| <a name="acronym.BR/EDR">BR/EDR</a> | Basic Rate/Enhanced Data Rate |
| <a name="acronym.HTTP">HTTP</a> | Hypertext Transfer Protocol |
| <a name="acronym.JSON">JSON</a> | JavaScript Object Notation; a data interchange format |
| <a name="acronym.JSON-RPC">JSON-RPC</a> | A remote procedure call protocol encoded in JSON |
| <a name="acronym.LC-SBC">LC-SBC</a> | Low-Complexity SubBand Coding |

The table below provides and overview of terms and abbreviations used in this document and their definitions.

| Term | Description |
| :-------- | :-------- |
| <a name="term.bitpool">bitpool</a> | A parameter to the LC-SBC codec that changes the encoding bitrate; the higher it is the higher the bitrate and thus the audio quality |
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

The Bluetooth Audio Sink plugin enables audio streaming to Bluetooth audio sink devices. The plugin is sink a from the host device stack perspective; in Bluetooth topology the host device becomes in fact an audio source. The plugin requires a Bluetooth controller service that will provide Bluetooth BR/EDR scanning, pairing and connection functionality; typically this is fulfiled by the *BluetoothControl* plugin.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *BluetoothAudio*) |
| classname | string | Class name: *BluetoothAudio* |
| locator | string | Library name: *libThunderBluetoothAudio.so* |
| startmode | string | Determines if the plugin shall be started automatically along with the framework |
| controller | string | <sup>*(optional)*</sup> Callsign of the Bluetooth controller service (typically *BluetoothControl*) |
| server | object | <sup>*(optional)*</sup> BluetoothAudio server configuration |
| server?.interface | number | <sup>*(optional)*</sup> Bluetooth interface to listen to for incomming connections |
| server?.inactivitytimeout | number | <sup>*(optional)*</sup> Timeout to drop inactive connections (in ms) |
| server?.psm | number | <sup>*(optional)*</sup> Port to listen to for incomming conections (typically 25) |
| sink | object | <sup>*(optional)*</sup> BluetoothAudio sink configuration |
| sink.codecs | object | Codec settings |
| sink.codecs.LC-SBC | object | Settings for the LC-SBC codec |
| sink.codecs.LC-SBC?.preset | string | <sup>*(optional)*</sup> Predefined audio quality setting (must be one of the following: *Compatible*, *LQ*, *MQ*, *HQ*, *XQ*) |
| sink.codecs.LC-SBC?.bitpool | number | <sup>*(optional)*</sup> Custom audio quality based on bitpool value (used when *preset* is not specified) |
| sink.codecs.LC-SBC?.channelmode | string | <sup>*(optional)*</sup> Channel mode for custom audio quality (used when *preset* is not specified) (must be one of the following: *Mono*, *Stereo*, *JointStereo*, *DualChannel*) |
| source | object | <sup>*(optional)*</sup> BluetoothAudio source configuration |
| source.codecs | object | Codec settings |
| source.codecs?.LC-SBC | object | <sup>*(optional)*</sup> Settings for the LC-SBC codec |
| source.codecs?.LC-SBC.maxbitpool | number | Maximum accepted bitpool value |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- IBluetoothAudio::ISink ([IBluetoothAudio.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IBluetoothAudio.h)) (version 1.0.0) (compliant format)
- IBluetoothAudio::ISource ([IBluetoothAudio.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IBluetoothAudio.h)) (version 1.0.0) (compliant format)

<a name="head.Methods"></a>
# Methods

The following methods are provided by the BluetoothAudio plugin:

BluetoothAudio Sink interface methods:

| Method | Description |
| :-------- | :-------- |
| [sink::assign](#method.sink::assign) | Assigns a Bluetooth sink device for audio playback |
| [sink::revoke](#method.sink::revoke) | Revokes a Bluetooth sink device from audio playback |

<a name="method.sink::assign"></a>
## *sink::assign [<sup>method</sup>](#head.Methods)*

Assigns a Bluetooth sink device for audio playback.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Address of the bluetooth device to assign |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_BAD_REQUEST``` | Device address value is invalid |
|  | ```ERROR_ALREADY_CONNECTED``` | A sink device is already assigned |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.sink::assign",
  "params": {
    "address": "..."
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

<a name="method.sink::revoke"></a>
## *sink::revoke [<sup>method</sup>](#head.Methods)*

Revokes a Bluetooth sink device from audio playback.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ALREADY_RELEASED``` | No device is currently assigned as sink |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.sink::revoke"
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

The following properties are provided by the BluetoothAudio plugin:

BluetoothAudio Sink interface properties:

| Property | Description |
| :-------- | :-------- |
| [sink::state](#property.sink::state) <sup>RO</sup> | Current state o the audio sink device |
| [sink::device](#property.sink::device) <sup>RO</sup> | Bluetooth address of the audio sink device |
| [sink::type](#property.sink::type) <sup>RO</sup> | Type of the audio sink device |
| [sink::latency](#property.sink::latency) | Latency of the audio sink device |
| [sink::supportedcodecs](#property.sink::supportedcodecs) <sup>RO</sup> | Audio codecs supported by the audio sink device |
| [sink::supporteddrms](#property.sink::supporteddrms) <sup>RO</sup> | DRM schemes supported by the audio sink device |
| [sink::codec](#property.sink::codec) <sup>RO</sup> | Properites of the currently used audio codec |
| [sink::drm](#property.sink::drm) <sup>RO</sup> | Properites of the currently used DRM scheme |
| [sink::stream](#property.sink::stream) <sup>RO</sup> | v of the currently transmitted audio stream |

BluetoothAudio Source interface properties:

| Property | Description |
| :-------- | :-------- |
| [source::state](#property.source::state) <sup>RO</sup> | Current state of the source device |
| [source::device](#property.source::device) <sup>RO</sup> | Bluetooth address of the source device |
| [source::type](#property.source::type) <sup>RO</sup> | Type of the audio source device |
| [source::codec](#property.source::codec) <sup>RO</sup> | Properites of the currently used codec |
| [source::drm](#property.source::drm) <sup>RO</sup> | Properties of the currently used DRM scheme |
| [source::stream](#property.source::stream) <sup>RO</sup> | Properites of the currently transmitted audio stream |

<a name="property.sink::state"></a>
## *sink::state [<sup>property</sup>](#head.Properties)*

Provides access to the current state o the audio sink device.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | string | Current state o the audio sink device (must be one of the following: *Unassigned*, *Disconnected*, *Connecting*, *Connected*, *ConnectedBad*, *ConnectedRestricted*, *Ready*, *Streaming*) |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.sink::state"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "Unassigned"
}
```

<a name="property.sink::device"></a>
## *sink::device [<sup>property</sup>](#head.Properties)*

Provides access to the bluetooth address of the audio sink device.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | string | Bluetooth address of the audio sink device |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | The sink device currently is not assigned |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.sink::device"
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

<a name="property.sink::type"></a>
## *sink::type [<sup>property</sup>](#head.Properties)*

Provides access to the type of the audio sink device.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | string | Type of the audio sink device (must be one of the following: *Unknown*, *Headphone*, *Speaker*, *Recorder*, *Amplifier*) |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | The sink device currently is not connected |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.sink::type"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "Unknown"
}
```

<a name="property.sink::latency"></a>
## *sink::latency [<sup>property</sup>](#head.Properties)*

Provides access to the latency of the audio sink device.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Latency of the audio sink device |
| (property).value | integer | Audio latency in milliseconds |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | integer |  |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_BAD_REQUEST``` | Latency value is invalid |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.sink::latency"
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
  "method": "BluetoothAudio.1.sink::latency",
  "params": {
    "value": 20
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

<a name="property.sink::supportedcodecs"></a>
## *sink::supportedcodecs [<sup>property</sup>](#head.Properties)*

Provides access to the audio codecs supported by the audio sink device.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Audio codecs supported by the audio sink device |
| result[#] | string |  (must be one of the following: *LC-SBC*) |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | The sink device currently is not connected |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.sink::supportedcodecs"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    "LC-SBC"
  ]
}
```

<a name="property.sink::supporteddrms"></a>
## *sink::supporteddrms [<sup>property</sup>](#head.Properties)*

Provides access to the DRM schemes supported by the audio sink device.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | DRM schemes supported by the audio sink device |
| result[#] | string |  (must be one of the following: *DTCP*, *SCMS-T*) |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | The sink device currently is not connected |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.sink::supporteddrms"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    "DTCP"
  ]
}
```

<a name="property.sink::codec"></a>
## *sink::codec [<sup>property</sup>](#head.Properties)*

Provides access to the properites of the currently used audio codec.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object | Properites of the currently used audio codec |
| result.codec | string | Audio codec used (must be one of the following: *LC-SBC*) |
| result.settings | opaque object | Codec-specific audio quality preset, compression profile, etc |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | The sink device currently is not configured |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.sink::codec"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "codec": "LC-SBC",
    "settings": {}
  }
}
```

<a name="property.sink::drm"></a>
## *sink::drm [<sup>property</sup>](#head.Properties)*

Provides access to the properites of the currently used DRM scheme.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object | Properites of the currently used DRM scheme |
| result.drm | string | Content protection scheme used (must be one of the following: *DTCP*, *SCMS-T*) |
| result.settings | opaque object | DRM-specific content protection level, encoding rules, etc |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | The sink device currently is not connected or not yet configured |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.sink::drm"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "drm": "DTCP",
    "settings": {}
  }
}
```

<a name="property.sink::stream"></a>
## *sink::stream [<sup>property</sup>](#head.Properties)*

Provides access to the v of the currently transmitted audio stream.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object | v of the currently transmitted audio stream |
| result.samplerate | integer | Sample rate in Hz |
| result.bitrate | integer | Target bitrate in bits per second (eg. 320000) |
| result.channels | integer | Number of audio channels |
| result.resolution | integer | Sampling resolution in bits per sample |
| result.isresampled | boolean | Indicates if the source stream is being resampled by the stack to match sink capabilities |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | The sink device currently is not connected or not yet configured |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.sink::stream"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "samplerate": 44100,
    "bitrate": 0,
    "channels": 2,
    "resolution": 16,
    "isresampled": false
  }
}
```

<a name="property.source::state"></a>
## *source::state [<sup>property</sup>](#head.Properties)*

Provides access to the current state of the source device.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | string | Current state of the source device (must be one of the following: *Unassigned*, *Disconnected*, *Connecting*, *Connected*, *ConnectedBad*, *ConnectedRestricted*, *Ready*, *Streaming*) |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.source::state"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "Unassigned"
}
```

<a name="property.source::device"></a>
## *source::device [<sup>property</sup>](#head.Properties)*

Provides access to the bluetooth address of the source device.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | string | Bluetooth address of the source device |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | No source device is currently connected |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.source::device"
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

<a name="property.source::type"></a>
## *source::type [<sup>property</sup>](#head.Properties)*

Provides access to the type of the audio source device.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | string | Type of the audio source device (must be one of the following: *Unknown*, *Player*, *Microphone*, *Tuner*, *Mixer*) |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | No source device is currently connected |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.source::type"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "Unknown"
}
```

<a name="property.source::codec"></a>
## *source::codec [<sup>property</sup>](#head.Properties)*

Provides access to the properites of the currently used codec.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object | Properites of the currently used codec |
| result.codec | string | Audio codec used (must be one of the following: *LC-SBC*) |
| result.settings | opaque object | Codec-specific audio quality preset, compression profile, etc |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | No source device is not connected or sink is not yet configured |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.source::codec"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "codec": "LC-SBC",
    "settings": {}
  }
}
```

<a name="property.source::drm"></a>
## *source::drm [<sup>property</sup>](#head.Properties)*

Provides access to the properties of the currently used DRM scheme.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object | Properties of the currently used DRM scheme |
| result.drm | string | Content protection scheme used (must be one of the following: *DTCP*, *SCMS-T*) |
| result.settings | opaque object | DRM-specific content protection level, encoding rules, etc |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | No source device is connected or sink is not yet configured |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.source::drm"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "drm": "DTCP",
    "settings": {}
  }
}
```

<a name="property.source::stream"></a>
## *source::stream [<sup>property</sup>](#head.Properties)*

Provides access to the properites of the currently transmitted audio stream.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object | Properites of the currently transmitted audio stream |
| result.samplerate | integer | Sample rate in Hz |
| result.bitrate | integer | Target bitrate in bits per second (eg. 320000) |
| result.channels | integer | Number of audio channels |
| result.resolution | integer | Sampling resolution in bits per sample |
| result.isresampled | boolean | Indicates if the source stream is being resampled by the stack to match sink capabilities |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | No source device is connected or sink is not yet configured |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.source::stream"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "samplerate": 44100,
    "bitrate": 0,
    "channels": 2,
    "resolution": 16,
    "isresampled": false
  }
}
```

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the BluetoothAudio plugin:

BluetoothAudio Sink interface events:

| Event | Description |
| :-------- | :-------- |
| [sink::statechanged](#event.sink::statechanged) | Signals audio sink state change |

BluetoothAudio Source interface events:

| Event | Description |
| :-------- | :-------- |
| [source::statechanged](#event.source::statechanged) | Signals audio source state change |

<a name="event.sink::statechanged"></a>
## *sink::statechanged [<sup>event</sup>](#head.Notifications)*

Signals audio sink state change.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.state | string |  (must be one of the following: *Unassigned*, *Disconnected*, *Connecting*, *Connected*, *ConnectedBad*, *ConnectedRestricted*, *Ready*, *Streaming*) |

### Example

```json
{
  "jsonrpc": "2.0",
  "method": "client.events.1.sink::statechanged",
  "params": {
    "state": "Unassigned"
  }
}
```

<a name="event.source::statechanged"></a>
## *source::statechanged [<sup>event</sup>](#head.Notifications)*

Signals audio source state change.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.state | string |  (must be one of the following: *Unassigned*, *Disconnected*, *Connecting*, *Connected*, *ConnectedBad*, *ConnectedRestricted*, *Ready*, *Streaming*) |

### Example

```json
{
  "jsonrpc": "2.0",
  "method": "client.events.1.source::statechanged",
  "params": {
    "state": "Unassigned"
  }
}
```

