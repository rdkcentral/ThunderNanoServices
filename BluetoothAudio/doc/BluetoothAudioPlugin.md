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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *BluetoothAudio*) |
| classname | string | mandatory | Class name: *BluetoothAudio* |
| locator | string | mandatory | Library name: *libThunderBluetoothAudio.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| controller | string | optional | Callsign of the Bluetooth controller service (typically *BluetoothControl*) |
| server | object | optional | BluetoothAudio server configuration |
| server?.interface | integer | optional | Bluetooth interface to listen to for incomming connections |
| server?.inactivitytimeout | integer | optional | Timeout to drop inactive connections (in ms) |
| server?.psm | integer | optional | Port to listen to for incomming conections (typically 25) |
| sink | object | optional | BluetoothAudio sink configuration |
| sink.codecs | object | mandatory | Codec settings |
| sink.codecs.LC-SBC | object | mandatory | Settings for the LC-SBC codec |
| sink.codecs.LC-SBC?.preset | string | optional | Predefined audio quality setting (must be one of the following: *Compatible, HQ, LQ, MQ, XQ*) |
| sink.codecs.LC-SBC?.bitpool | integer | optional | Custom audio quality based on bitpool value (used when *preset* is not specified) |
| sink.codecs.LC-SBC?.channelmode | string | optional | Channel mode for custom audio quality (used when *preset* is not specified) (must be one of the following: *DualChannel, JointStereo, Mono, Stereo*) |
| source | object | optional | BluetoothAudio source configuration |
| source.codecs | object | mandatory | Codec settings |
| source.codecs?.LC-SBC | object | optional | Settings for the LC-SBC codec |
| source.codecs?.LC-SBC.maxbitpool | integer | mandatory | Maximum accepted bitpool value |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- IBluetoothAudio::ISink ([IBluetoothAudio.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IBluetoothAudio.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

- IBluetoothAudio::ISource ([IBluetoothAudio.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IBluetoothAudio.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | Address of the bluetooth device to assign |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_BAD_REQUEST``` | Device address value is invalid |
| ```ERROR_ALREADY_CONNECTED``` | A sink device is already assigned |

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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ALREADY_RELEASED``` | No device is currently assigned as sink |

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

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [sink::state](#property.sink::state) | read-only | Current state o the audio sink device |
| [sink::device](#property.sink::device) | read-only | Bluetooth address of the audio sink device |
| [sink::type](#property.sink::type) | read-only | Type of the audio sink device |
| [sink::latency](#property.sink::latency) | read/write | Latency of the audio sink device |
| [sink::supportedcodecs](#property.sink::supportedcodecs) | read-only | Audio codecs supported by the audio sink device |
| [sink::supporteddrms](#property.sink::supporteddrms) | read-only | DRM schemes supported by the audio sink device |
| [sink::codec](#property.sink::codec) | read-only | Properites of the currently used audio codec |
| [sink::drm](#property.sink::drm) | read-only | Properites of the currently used DRM scheme |
| [sink::stream](#property.sink::stream) | read-only | Properties of the currently transmitted audio stream |

BluetoothAudio Source interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [source::state](#property.source::state) | read-only | Current state of the source device |
| [source::device](#property.source::device) | read-only | Bluetooth address of the source device |
| [source::type](#property.source::type) | read-only | Type of the audio source device |
| [source::codec](#property.source::codec) | read-only | Properites of the currently used codec |
| [source::drm](#property.source::drm) | read-only | Properties of the currently used DRM scheme |
| [source::stream](#property.source::stream) | read-only | Properites of the currently transmitted audio stream |

<a name="property.sink::state"></a>
## *sink::state [<sup>property</sup>](#head.Properties)*

Provides access to the current state o the audio sink device.

> This property is **read-only**.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | Current state o the audio sink device (must be one of the following: *Connected, ConnectedBad, ConnectedRestricted, Connecting, Disconnected, Ready, Streaming, Unassigned*) |

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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | Bluetooth address of the audio sink device |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | The sink device currently is not assigned |

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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | Type of the audio sink device (must be one of the following: *Amplifier, Headphone, Recorder, Speaker, Unknown*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | The sink device currently is not connected |

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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Latency of the audio sink device |
| (property).value | integer | mandatory | Audio latency in milliseconds |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | integer | mandatory | Latency of the audio sink device |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_BAD_REQUEST``` | Latency value is invalid |

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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | Audio codecs supported by the audio sink device |
| result[#] | string | mandatory | *...* (must be one of the following: *LC-SBC*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | The sink device currently is not connected |

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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | DRM schemes supported by the audio sink device |
| result[#] | string | mandatory | *...* (must be one of the following: *DTCP, SCMS-T*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | The sink device currently is not connected |

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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | Properites of the currently used audio codec |
| result.codec | string | mandatory | Audio codec used (must be one of the following: *LC-SBC*) |
| result.settings | opaque object | mandatory | Codec-specific audio quality preset, compression profile, etc |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | The sink device currently is not configured |

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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | Properites of the currently used DRM scheme |
| result.drm | string | mandatory | Content protection scheme used (must be one of the following: *DTCP, SCMS-T*) |
| result.settings | opaque object | mandatory | DRM-specific content protection level, encoding rules, etc |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | The sink device currently is not connected or not yet configured |

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

Provides access to the properties of the currently transmitted audio stream.

> This property is **read-only**.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | Properties of the currently transmitted audio stream |
| result.samplerate | integer | mandatory | Sample rate in Hz |
| result.bitrate | integer | mandatory | Target bitrate in bits per second (eg. 320000) |
| result.channels | integer | mandatory | Number of audio channels |
| result.resolution | integer | mandatory | Sampling resolution in bits per sample |
| result.isresampled | boolean | mandatory | Indicates if the source stream is being resampled by the stack to match sink capabilities |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | The sink device currently is not connected or not yet configured |

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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | Current state of the source device (must be one of the following: *Connected, ConnectedBad, ConnectedRestricted, Connecting, Disconnected, Ready, Streaming, Unassigned*) |

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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | Bluetooth address of the source device |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | No source device is currently connected |

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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | Type of the audio source device (must be one of the following: *Microphone, Mixer, Player, Tuner, Unknown*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | No source device is currently connected |

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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | Properites of the currently used codec |
| result.codec | string | mandatory | Audio codec used (must be one of the following: *LC-SBC*) |
| result.settings | opaque object | mandatory | Codec-specific audio quality preset, compression profile, etc |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | No source device is not connected or sink is not yet configured |

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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | Properties of the currently used DRM scheme |
| result.drm | string | mandatory | Content protection scheme used (must be one of the following: *DTCP, SCMS-T*) |
| result.settings | opaque object | mandatory | DRM-specific content protection level, encoding rules, etc |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | No source device is connected or sink is not yet configured |

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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | Properites of the currently transmitted audio stream |
| result.samplerate | integer | mandatory | Sample rate in Hz |
| result.bitrate | integer | mandatory | Target bitrate in bits per second (eg. 320000) |
| result.channels | integer | mandatory | Number of audio channels |
| result.resolution | integer | mandatory | Sampling resolution in bits per sample |
| result.isresampled | boolean | mandatory | Indicates if the source stream is being resampled by the stack to match sink capabilities |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | No source device is connected or sink is not yet configured |

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

| Notification | Description |
| :-------- | :-------- |
| [sink::statechanged](#notification.sink::statechanged) | Signals audio sink state change |

BluetoothAudio Source interface events:

| Notification | Description |
| :-------- | :-------- |
| [source::statechanged](#notification.source::statechanged) | Signals audio source state change |

<a name="notification.sink::statechanged"></a>
## *sink::statechanged [<sup>notification</sup>](#head.Notifications)*

Signals audio sink state change.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.state | string | mandatory | Changed BluetoothAudio State (must be one of the following: *Connected, ConnectedBad, ConnectedRestricted, Connecting, Disconnected, Ready, Streaming, Unassigned*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.register",
  "params": {
    "event": "sink::statechanged",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.sink::statechanged",
  "params": {
    "state": "Unassigned"
  }
}
```

<a name="notification.source::statechanged"></a>
## *source::statechanged [<sup>notification</sup>](#head.Notifications)*

Signals audio source state change.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.state | string | mandatory | Changed BluetoothAudio State (must be one of the following: *Connected, ConnectedBad, ConnectedRestricted, Connecting, Disconnected, Ready, Streaming, Unassigned*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudio.1.register",
  "params": {
    "event": "source::statechanged",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.source::statechanged",
  "params": {
    "state": "Unassigned"
  }
}
```

