<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Bluetooth_Audio_Sink_Plugin"></a>
# Bluetooth Audio Sink Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

BluetoothAudioSink plugin for Thunder framework.

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

This document describes purpose and functionality of the BluetoothAudioSink plugin. It includes detailed specification about its configuration,
         methods and properties as well as sent notifications.

<a name="head.Case_Sensitivity"></a>
## Case Sensitivity

All identifiers of the interfaces described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties,
         relations and actions should be treated as such.

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
| <a name="term.callsign">callsign</a> | The name given to an instance of a plugin. One plugin can be instantiated multiple times,
         but each instance the instance name, callsign, must be unique. |

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

The Bluetooth Audio Sink plugin enables audio streaming to Bluetooth audio sink devices. The plugin is sink a from the host device stack perspective; in Bluetooth topology the host device becomes in fact an audio source. The plugin requires a Bluetooth controller service that will provide Bluetooth BR/EDR scanning, pairing and connection functionality; typically this is fulfiled by the *BluetoothControl* plugin.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *BluetoothAudioSink*) |
| classname | string | Class name: *BluetoothAudioSink* |
| locator | string | Library name: *libWPEFrameworkBluetoothAudioSink.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |
| configuration | object |  |
| configuration.controller | string | Callsign of the Bluetooth controller service (typically *BluetoothControl*) |
| configuration.codecs | object | Codec settings |
| configuration.codecs.LC-SBC | object | Settings for the LC-SBC codec |
| configuration.codecs.LC-SBC?.preset | string | <sup>*(optional)*</sup> Predefined audio quality setting (must be one of the following: *Compatible*, *LQ*,
         *MQ*, *HQ*, *XQ*) |
| configuration.codecs.LC-SBC?.bitpool | number | <sup>*(optional)*</sup> Custom audio quality based on bitpool value (used when *preset* is not specified) |
| configuration.codecs.LC-SBC?.channelmode | string | <sup>*(optional)*</sup> Channel mode for custom audio quality (used when *preset* is not specified) (must be one of the following: *Mono*, *Stereo*, *JointStereo*, *DualChannel*) |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- Exchange::IBluetoothAudioSink ([IBluetoothAudio.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IBluetoothAudio.h)) (version 1.0.0) (compliant format)

<a name="head.Methods"></a>
# Methods

The following methods are provided by the BluetoothAudioSink plugin:

BluetoothAudioSink interface methods:

| Method | Description |
| :-------- | :-------- |
| [assign](#method.assign) | Assigns a Bluetooth device for audio playback |
| [revoke](#method.revoke) | Revokes a Bluetooth device from audio playback |

<a name="method.assign"></a>
## *assign [<sup>method</sup>](#head.Methods)*

Assigns a Bluetooth device for audio playback.

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
|  | ```ERROR_ALREADY_CONNECTED``` | A device is already assigned |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudioSink.1.assign",
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

<a name="method.revoke"></a>
## *revoke [<sup>method</sup>](#head.Methods)*

Revokes a Bluetooth device from audio playback.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ALREADY_RELEASED``` | No device is currently assigned |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudioSink.1.revoke"
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

The following properties are provided by the BluetoothAudioSink plugin:

BluetoothAudioSink interface properties:

| Property | Description |
| :-------- | :-------- |
| [latency](#property.latency) | Sink audio latency |
| [state](#property.state) <sup>RO</sup> | Current audio sink state |
| [type](#property.type) <sup>RO</sup> | Audio sink type |
| [supportedcodecs](#property.supportedcodecs) <sup>RO</sup> | Audio codecs supported by the sink |
| [supporteddrms](#property.supporteddrms) <sup>RO</sup> | DRM schemes supported by the sink |
| [codec](#property.codec) <sup>RO</sup> | Properites of the currently used codec |
| [drm](#property.drm) <sup>RO</sup> | Properties of the currently used DRM scheme |
| [stream](#property.stream) <sup>RO</sup> | Properties of the current output stream |

<a name="property.latency"></a>
## *latency [<sup>property</sup>](#head.Properties)*

Provides access to the sink audio latency.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Sink audio latency |
| (property).value | integer | Audio latency of the sink in milliseconds |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | integer |  |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | No device is currently assigned |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudioSink.1.latency"
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
  "method": "BluetoothAudioSink.1.latency",
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

<a name="property.state"></a>
## *state [<sup>property</sup>](#head.Properties)*

Provides access to the current audio sink state.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | string | Current audio sink state (must be one of the following: *Unassigned*, *Disconnected*, *ConnectedBadDevice*, *ConnectedRestricted*,
         *Connected*, *Ready*, *Streaming*) |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudioSink.1.state"
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

<a name="property.type"></a>
## *type [<sup>property</sup>](#head.Properties)*

Provides access to the audio sink type.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | string | Audio sink type (must be one of the following: *Unknown*, *Headphone*, *Speaker*, *Recorder*, *Amplifier*) |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | The sink device is not connected |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudioSink.1.type"
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

<a name="property.supportedcodecs"></a>
## *supportedcodecs [<sup>property</sup>](#head.Properties)*

Provides access to the audio codecs supported by the sink.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Audio codecs supported by the sink |
| result[#] | string |  (must be one of the following: *LC-SBC*) |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | The sink device is not connected |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudioSink.1.supportedcodecs"
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

<a name="property.supporteddrms"></a>
## *supporteddrms [<sup>property</sup>](#head.Properties)*

Provides access to the DRM schemes supported by the sink.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | DRM schemes supported by the sink |
| result[#] | string |  (must be one of the following: *None*, *DTCP*, *SCMS-T*) |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | The sink device is not connected |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudioSink.1.supporteddrms"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    "None"
  ]
}
```

<a name="property.codec"></a>
## *codec [<sup>property</sup>](#head.Properties)*

Provides access to the properites of the currently used codec.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object | Properites of the currently used codec |
| result.codec | string | Audio codec used (must be one of the following: *LC-SBC*) |
| result.settings | string | Codec-specific audio quality preset, compression profile, etc |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | Currently not streaming |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudioSink.1.codec"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "codec": "LC-SBC",
    "settings": "..."
  }
}
```

<a name="property.drm"></a>
## *drm [<sup>property</sup>](#head.Properties)*

Provides access to the properties of the currently used DRM scheme.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object | Properties of the currently used DRM scheme |
| result.drm | string | Content protection scheme used (must be one of the following: *None*, *DTCP*, *SCMS-T*) |
| result.settings | string | DRM-specific content protection level, encoding rules, etc |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | Currently not streaming |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudioSink.1.drm"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "drm": "None",
    "settings": "..."
  }
}
```

<a name="property.stream"></a>
## *stream [<sup>property</sup>](#head.Properties)*

Provides access to the properties of the current output stream.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object | Properties of the current output stream |
| result.samplerate | integer | Sample rate in Hz |
| result.bitrate | integer | Target bitrate in bits per second (eg. 320000) |
| result.channels | integer | Number of audio channels |
| result.resolution | integer | Sampling resolution in bits per sample |
| result.isresampling | boolean | Indicates if the sink is resampling the input stream |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_ILLEGAL_STATE``` | Currently not streaming |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothAudioSink.1.stream"
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
        
    "isresampling": false
  }
}
```

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the BluetoothAudioSink plugin:

BluetoothAudioSink interface events:

| Event | Description |
| :-------- | :-------- |
| [updated](#event.updated) | Signals audio sink state change or stream properties update |

<a name="event.updated"></a>
## *updated [<sup>event</sup>](#head.Notifications)*

Signals audio sink state change or stream properties update.

### Parameters

This event carries no parameters.

### Example

```json
{
  "jsonrpc": "2.0",
  "method": "client.events.1.updated"
}
```

