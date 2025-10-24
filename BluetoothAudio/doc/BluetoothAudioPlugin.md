<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_Bluetooth_Audio_Plugin"></a>
# Bluetooth Audio Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

BluetoothAudio plugin for Thunder framework.

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

This document describes purpose and functionality of the BluetoothAudio plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

<a id="head_Case_Sensitivity"></a>
## Case Sensitivity

All identifiers of the interfaces described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a id="head_Acronyms,_Abbreviations_and_Terms"></a>
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

The Bluetooth Audio Sink plugin enables audio streaming to Bluetooth audio sink devices. The plugin is sink a from the host device stack perspective; in Bluetooth topology the host device becomes in fact an audio source. The plugin requires a Bluetooth controller service that will provide Bluetooth BR/EDR scanning, pairing and connection functionality; typically this is fulfiled by the *BluetoothControl* plugin.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a id="head_Configuration"></a>
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

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- IBluetoothAudio::ISink ([IBluetoothAudio.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IBluetoothAudio.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

- IBluetoothAudio::ISource ([IBluetoothAudio.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IBluetoothAudio.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

<a id="head_Methods"></a>
# Methods

The following methods are provided by the BluetoothAudio plugin:

Built-in methods:

| Method | Description |
| :-------- | :-------- |
| [versions](#method_versions) | Retrieves a list of JSON-RPC interfaces offered by this service |
| [exists](#method_exists) | Checks if a JSON-RPC method or property exists |
| [register](#method_register) | Registers for an asynchronous JSON-RPC notification |
| [unregister](#method_unregister) | Unregisters from an asynchronous JSON-RPC notification |

BluetoothAudio Sink interface methods:

| Method | Description |
| :-------- | :-------- |
| [sink::assign](#method_sink__assign) | Assigns a Bluetooth sink device for audio playback |
| [sink::revoke](#method_sink__revoke) | Revokes a Bluetooth sink device from audio playback |

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
  "method": "BluetoothAudio.1.versions"
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

This method will return *True* for the following methods/properties: *sink::state, sink::device, sink::type, sink::latency, sink::supportedcodecs, sink::supporteddrms, sink::codec, sink::drm, sink::stream, source::state, source::device, source::type, source::codec, source::drm, source::stream, versions, exists, register, unregister, sink::assign, sink::revoke*.

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
  "method": "BluetoothAudio.1.exists",
  "params": {
    "method": "sink::state"
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

This method supports the following event names: *[sink::statechanged](#notification_sink::statechanged), [source::statechanged](#notification_source::statechanged)*.

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
  "method": "BluetoothAudio.1.register",
  "params": {
    "event": "sink::statechanged",
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

This method supports the following event names: *[sink::statechanged](#notification_sink::statechanged), [source::statechanged](#notification_source::statechanged)*.

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
  "method": "BluetoothAudio.1.unregister",
  "params": {
    "event": "sink::statechanged",
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

<a id="method_sink__assign"></a>
## *sink::assign [<sup>method</sup>](#head_Methods)*

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

<a id="method_sink__revoke"></a>
## *sink::revoke [<sup>method</sup>](#head_Methods)*

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

<a id="head_Properties"></a>
# Properties

The following properties are provided by the BluetoothAudio plugin:

BluetoothAudio Sink interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [sink::state](#property_sink__state) | read-only | Current state o the audio sink device |
| [sink::device](#property_sink__device) | read-only | Bluetooth address of the audio sink device |
| [sink::type](#property_sink__type) | read-only | Type of the audio sink device |
| [sink::latency](#property_sink__latency) | read/write | Latency of the audio sink device |
| [sink::supportedcodecs](#property_sink__supportedcodecs) | read-only | Audio codecs supported by the audio sink device |
| [sink::supporteddrms](#property_sink__supporteddrms) | read-only | DRM schemes supported by the audio sink device |
| [sink::codec](#property_sink__codec) | read-only | Properites of the currently used audio codec |
| [sink::drm](#property_sink__drm) | read-only | Properites of the currently used DRM scheme |
| [sink::stream](#property_sink__stream) | read-only | Properties of the currently transmitted audio stream |

BluetoothAudio Source interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [source::state](#property_source__state) | read-only | Current state of the source device |
| [source::device](#property_source__device) | read-only | Bluetooth address of the source device |
| [source::type](#property_source__type) | read-only | Type of the audio source device |
| [source::codec](#property_source__codec) | read-only | Properites of the currently used codec |
| [source::drm](#property_source__drm) | read-only | Properties of the currently used DRM scheme |
| [source::stream](#property_source__stream) | read-only | Properites of the currently transmitted audio stream |

<a id="property_sink__state"></a>
## *sink::state [<sup>property</sup>](#head_Properties)*

Provides access to the current state o the audio sink device.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Current state o the audio sink device (must be one of the following: *Connected, ConnectedBad, ConnectedRestricted, Connecting, Disconnected, Ready, Streaming, Unassigned*) |

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
  "result": "Disconnected"
}
```

<a id="property_sink__device"></a>
## *sink::device [<sup>property</sup>](#head_Properties)*

Provides access to the bluetooth address of the audio sink device.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Bluetooth address of the audio sink device |

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

<a id="property_sink__type"></a>
## *sink::type [<sup>property</sup>](#head_Properties)*

Provides access to the type of the audio sink device.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Type of the audio sink device (must be one of the following: *Amplifier, Headphone, Recorder, Speaker, Unknown*) |

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
  "result": "Headphone"
}
```

<a id="property_sink__latency"></a>
## *sink::latency [<sup>property</sup>](#head_Properties)*

Provides access to the latency of the audio sink device.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Latency of the audio sink device |
| (property).value | integer | mandatory | Audio latency in milliseconds |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | integer | mandatory | Latency of the audio sink device |

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

<a id="property_sink__supportedcodecs"></a>
## *sink::supportedcodecs [<sup>property</sup>](#head_Properties)*

Provides access to the audio codecs supported by the audio sink device.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | Audio codecs supported by the audio sink device |
| (property)[#] | string | mandatory | *...* (must be one of the following: *LC-SBC*) |

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

<a id="property_sink__supporteddrms"></a>
## *sink::supporteddrms [<sup>property</sup>](#head_Properties)*

Provides access to the DRM schemes supported by the audio sink device.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | DRM schemes supported by the audio sink device |
| (property)[#] | string | mandatory | *...* (must be one of the following: *DTCP, SCMS-T*) |

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
    "SCMS-T"
  ]
}
```

<a id="property_sink__codec"></a>
## *sink::codec [<sup>property</sup>](#head_Properties)*

Provides access to the properites of the currently used audio codec.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Properites of the currently used audio codec |
| (property).codec | string | mandatory | Audio codec used (must be one of the following: *LC-SBC*) |
| (property).settings | opaque object | mandatory | Codec-specific audio quality preset, compression profile, etc |

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

<a id="property_sink__drm"></a>
## *sink::drm [<sup>property</sup>](#head_Properties)*

Provides access to the properites of the currently used DRM scheme.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Properites of the currently used DRM scheme |
| (property).drm | string | mandatory | Content protection scheme used (must be one of the following: *DTCP, SCMS-T*) |
| (property).settings | opaque object | mandatory | DRM-specific content protection level, encoding rules, etc |

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
    "drm": "SCMS-T",
    "settings": {}
  }
}
```

<a id="property_sink__stream"></a>
## *sink::stream [<sup>property</sup>](#head_Properties)*

Provides access to the properties of the currently transmitted audio stream.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Properties of the currently transmitted audio stream |
| (property).samplerate | integer | mandatory | Sample rate in Hz |
| (property).bitrate | integer | mandatory | Target bitrate in bits per second (eg. 320000) |
| (property).channels | integer | mandatory | Number of audio channels |
| (property).resolution | integer | mandatory | Sampling resolution in bits per sample |
| (property).isresampled | boolean | mandatory | Indicates if the source stream is being resampled by the stack to match sink capabilities |

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

<a id="property_source__state"></a>
## *source::state [<sup>property</sup>](#head_Properties)*

Provides access to the current state of the source device.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Current state of the source device (must be one of the following: *Connected, ConnectedBad, ConnectedRestricted, Connecting, Disconnected, Ready, Streaming, Unassigned*) |

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
  "result": "Disconnected"
}
```

<a id="property_source__device"></a>
## *source::device [<sup>property</sup>](#head_Properties)*

Provides access to the bluetooth address of the source device.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Bluetooth address of the source device |

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

<a id="property_source__type"></a>
## *source::type [<sup>property</sup>](#head_Properties)*

Provides access to the type of the audio source device.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Type of the audio source device (must be one of the following: *Microphone, Mixer, Player, Tuner, Unknown*) |

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
  "result": "Player"
}
```

<a id="property_source__codec"></a>
## *source::codec [<sup>property</sup>](#head_Properties)*

Provides access to the properites of the currently used codec.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Properites of the currently used codec |
| (property).codec | string | mandatory | Audio codec used (must be one of the following: *LC-SBC*) |
| (property).settings | opaque object | mandatory | Codec-specific audio quality preset, compression profile, etc |

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

<a id="property_source__drm"></a>
## *source::drm [<sup>property</sup>](#head_Properties)*

Provides access to the properties of the currently used DRM scheme.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Properties of the currently used DRM scheme |
| (property).drm | string | mandatory | Content protection scheme used (must be one of the following: *DTCP, SCMS-T*) |
| (property).settings | opaque object | mandatory | DRM-specific content protection level, encoding rules, etc |

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
    "drm": "SCMS-T",
    "settings": {}
  }
}
```

<a id="property_source__stream"></a>
## *source::stream [<sup>property</sup>](#head_Properties)*

Provides access to the properites of the currently transmitted audio stream.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Properites of the currently transmitted audio stream |
| (property).samplerate | integer | mandatory | Sample rate in Hz |
| (property).bitrate | integer | mandatory | Target bitrate in bits per second (eg. 320000) |
| (property).channels | integer | mandatory | Number of audio channels |
| (property).resolution | integer | mandatory | Sampling resolution in bits per sample |
| (property).isresampled | boolean | mandatory | Indicates if the source stream is being resampled by the stack to match sink capabilities |

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

<a id="head_Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the BluetoothAudio plugin:

BluetoothAudio Sink interface events:

| Notification | Description |
| :-------- | :-------- |
| [sink::statechanged](#notification_sink__statechanged) | Signals audio sink state change |

BluetoothAudio Source interface events:

| Notification | Description |
| :-------- | :-------- |
| [source::statechanged](#notification_source__statechanged) | Signals audio source state change |

<a id="notification_sink__statechanged"></a>
## *sink::statechanged [<sup>notification</sup>](#head_Notifications)*

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
    "state": "Disconnected"
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.sink::statechanged``.

<a id="notification_source__statechanged"></a>
## *source::statechanged [<sup>notification</sup>](#head_Notifications)*

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
    "state": "Disconnected"
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.source::statechanged``.

