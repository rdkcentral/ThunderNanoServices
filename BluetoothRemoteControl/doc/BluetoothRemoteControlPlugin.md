<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_Bluetooth_Remote_Control_Plugin"></a>
# Bluetooth Remote Control Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

BluetoothRemoteControl plugin for Thunder framework.

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

This document describes purpose and functionality of the BluetoothRemoteControl plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

<a id="head_Case_Sensitivity"></a>
## Case Sensitivity

All identifiers of the interfaces described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a id="head_Acronyms,_Abbreviations_and_Terms"></a>
## Acronyms, Abbreviations and Terms

The table below provides and overview of acronyms used in this document and their definitions.

| Acronym | Description |
| :-------- | :-------- |
| <a name="acronym.ADPCM">ADPCM</a> | Adaptive Pulse-code Modulation |
| <a name="acronym.API">API</a> | Application Programming Interface |
| <a name="acronym.BLE">BLE</a> | Bluetooth Low Energy |
| <a name="acronym.GATT">GATT</a> | Generic Attribute Profile |
| <a name="acronym.HTTP">HTTP</a> | Hypertext Transfer Protocol |
| <a name="acronym.JSON">JSON</a> | JavaScript Object Notation; a data interchange format |
| <a name="acronym.JSON-RPC">JSON-RPC</a> | A remote procedure call protocol encoded in JSON |
| <a name="acronym.PCM">PCM</a> | Pulse-code Modulation |
| <a name="acronym.UUID">UUID</a> | Universally Unique Identifier |
| <a name="acronym.WAV">WAV</a> | Waveform Audio File Format |

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

The Bluetooth Remote Control plugin allows configuring and enabling Bluetooth LE remote control units.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a id="head_Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *BluetoothRemoteControl*) |
| classname | string | mandatory | Class name: *BluetoothRemoteControl* |
| locator | string | mandatory | Library name: *libThunderBluetoothRemoteControl.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| configuration | object | optional | *...* |
| configuration?.controller | string | optional | Name of the Bluetooth controller service (default: *BluetoothControl*) |
| configuration?.keymap | string | optional | Keymap name |
| configuration?.keyingest | boolean | optional | Enable key ingestion |
| configuration?.serviceuuid | string | optional | UUID of the voice control GATT service |
| configuration?.commanduuid | string | optional | UUID of the voice control command GATT characteristic |
| configuration?.datauuid | string | optional | UUID of the voice control data GATT characteristic |
| configuration?.recorder | string | optional | Enable voice data recording (debug purposes) to WAV file (must be one of the following: *off, sequenced, sequenced_persist, single, single_persist*) |
| configuration?.audiobuffersize | integer | optional | Size of the audio buffer in miliseconds (if not set then determined automatically) |
| configuration?.firstaudiochunksize | integer | optional | Size of the first audio transmission notification in miliseconds |
| configuration?.audiochunksize | integer | optional | Size of the audio transmission notifications in miliseconds (if not set then audio data is not buffered) |
| configuration?.audioprofile | object | optional | *...* |
| configuration?.audioprofile?.samplerate | integer | optional | Audio data sample rate in Hz (e.g. 16000) |
| configuration?.audioprofile?.channels | integer | optional | Number of audio channels (e.g. 1 for mono stream) |
| configuration?.audioprofile?.resolution | integer | optional | Audio samples resolution in bits (e.g. 16) |

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- IBluetoothRemoteControl ([IBluetoothRemoteControl.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IBluetoothRemoteControl.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

- IAudioStream ([IAudioStream.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IAudioStream.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

<a id="head_Methods"></a>
# Methods

The following methods are provided by the BluetoothRemoteControl plugin:

Built-in methods:

| Method | Description |
| :-------- | :-------- |
| [versions](#method_versions) | Retrieves a list of JSON-RPC interfaces offered by this service |
| [exists](#method_exists) | Checks if a JSON-RPC method or property exists |
| [register](#method_register) | Registers for an asynchronous JSON-RPC notification |
| [unregister](#method_unregister) | Unregisters from an asynchronous JSON-RPC notification |

BluetoothRemoteControl interface methods:

| Method | Description |
| :-------- | :-------- |
| [assign](#method_assign) | Assigns a Bluetooth device as a RCU |
| [revoke](#method_revoke) | Revokes a Bluetooth device from RCU operation |

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
  "method": "BluetoothRemoteControl.1.versions"
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

This method will return *True* for the following methods/properties: *device, metadata, batterylevel, voicecontrol, name, state, capabilities, audioprofile, time, speed, versions, exists, register, unregister, assign, revoke*.

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
  "method": "BluetoothRemoteControl.1.exists",
  "params": {
    "method": "device"
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

This method supports the following event names: *[batterylevelchange](#notification_batterylevelchange), [audiotransmission](#notification_audiotransmission), [audioframe](#notification_audioframe)*.

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
  "method": "BluetoothRemoteControl.1.register",
  "params": {
    "event": "batterylevelchange",
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

This method supports the following event names: *[batterylevelchange](#notification_batterylevelchange), [audiotransmission](#notification_audiotransmission), [audioframe](#notification_audioframe)*.

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
  "method": "BluetoothRemoteControl.1.unregister",
  "params": {
    "event": "batterylevelchange",
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

<a id="method_assign"></a>
## *assign [<sup>method</sup>](#head_Methods)*

Assigns a Bluetooth device as a RCU.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | Address of the Bluetooth device to assign |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Device address value is invalid |
| ```ERROR_ALREADY_CONNECTED``` | A RCU device is already assigned |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothRemoteControl.1.assign",
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

<a id="method_revoke"></a>
## *revoke [<sup>method</sup>](#head_Methods)*

Revokes a Bluetooth device from RCU operation.

### Parameters

This method takes no parameters.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ALREADY_RELEASED``` | No device is currently assigned as RCU |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothRemoteControl.1.revoke"
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

The following properties are provided by the BluetoothRemoteControl plugin:

BluetoothRemoteControl interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [device](#property_device) / [address](#property_device) | read-only | Bluetooth address |
| [metadata](#property_metadata) / [info](#property_metadata) | read-only | Device metadata |
| [batterylevel](#property_batterylevel) | read-only | Battery level |
| [voicecontrol](#property_voicecontrol) | read/write | Toggle voice control |

AudioStream interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [name](#property_name) | read-only | Name of the stream |
| [state](#property_state) | read-only | Current state of the stream |
| [capabilities](#property_capabilities) | read-only | List of codecs supported by the stream |
| [audioprofile](#property_audioprofile) | read/write | Preferred profile of the stream |
| [time](#property_time) | read-only | Stream position |
| [speed](#property_speed) | read/write | Stream speed |

<a id="property_device"></a>
## *device [<sup>property</sup>](#head_Properties)*

Provides access to the bluetooth address.

> This property is **read-only**.

> ``address`` is an alternative name for this property. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Bluetooth address |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | The RCU device currently is not assigned |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothRemoteControl.1.device"
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

<a id="property_metadata"></a>
## *metadata [<sup>property</sup>](#head_Properties)*

Provides access to the device metadata.

> This property is **read-only**.

> ``info`` is an alternative name for this property. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Device metadata |
| (property).name | string | mandatory | Name of the unit |
| (property)?.model | string | optional | Model name |
| (property)?.serial | string | optional | Serial number |
| (property)?.firmware | string | optional | Firmware version |
| (property)?.software | string | optional | Software version |
| (property)?.manufacturer | string | optional | Vendor/manufacturer name |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | The RCU device currently is not assigned |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothRemoteControl.1.metadata"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "name": "...",
    "model": "...",
    "serial": "...",
    "firmware": "...",
    "software": "...",
    "manufacturer": "..."
  }
}
```

<a id="property_batterylevel"></a>
## *batterylevel [<sup>property</sup>](#head_Properties)*

Provides access to the battery level.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | integer | mandatory | Battery level |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | The device is not connected or does not support battery information |
| ```ERROR_ILLEGAL_STATE``` | The RCU device currently is not assigned |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothRemoteControl.1.batterylevel"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": 75
}
```

<a id="property_voicecontrol"></a>
## *voicecontrol [<sup>property</sup>](#head_Properties)*

Provides access to the toggle voice control.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Toggle voice control |
| (property).value | boolean | mandatory | *...* |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | boolean | mandatory | Toggle voice control |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_NOT_SUPPORTED``` | The device does not support voice input |
| ```ERROR_ILLEGAL_STATE``` | The RCU device currently is not assigned |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothRemoteControl.1.voicecontrol"
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
  "method": "BluetoothRemoteControl.1.voicecontrol",
  "params": {
    "value": false
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

<a id="property_name"></a>
## *name [<sup>property</sup>](#head_Properties)*

Provides access to the name of the stream.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Name of the stream |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | The stream is not ready for this operation |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothRemoteControl.1.name"
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

<a id="property_state"></a>
## *state [<sup>property</sup>](#head_Properties)*

Provides access to the current state of the stream.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Current state of the stream (must be one of the following: *Idle, Started, Unavailable*) |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothRemoteControl.1.state"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "Idle"
}
```

<a id="property_capabilities"></a>
## *capabilities [<sup>property</sup>](#head_Properties)*

Provides access to the list of codecs supported by the stream.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | List of codecs supported by the stream |
| (property)[#] | string | mandatory | *...* (must be one of the following: *IMA-ADPCM, PCM*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | The stream is not ready for this operation |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothRemoteControl.1.capabilities"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    "IMA-ADPCM"
  ]
}
```

<a id="property_audioprofile"></a>
## *audioprofile [<sup>property</sup>](#head_Properties)*

Provides access to the preferred profile of the stream.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Preferred profile of the stream |
| (property).value | object | mandatory | *...* |
| (property).value.codec | string | mandatory | Compression method (PCM: uncompressed) (must be one of the following: *IMA-ADPCM, PCM*) |
| (property).value?.codecparams | opaque object | optional | Additional parameters for codec |
| (property).value.channels | integer | mandatory | Number of audio channels |
| (property).value.resolution | integer | mandatory | Sample resultion in bits |
| (property).value.samplerate | integer | mandatory | Sample rate in hertz |
| (property).value?.bitrate | integer | optional | Data rate of the compressed stream in bits per second |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Preferred profile of the stream |
| (property).codec | string | mandatory | Compression method (PCM: uncompressed) (must be one of the following: *IMA-ADPCM, PCM*) |
| (property)?.codecparams | opaque object | optional | Additional parameters for codec |
| (property).channels | integer | mandatory | Number of audio channels |
| (property).resolution | integer | mandatory | Sample resultion in bits |
| (property).samplerate | integer | mandatory | Sample rate in hertz |
| (property)?.bitrate | integer | optional | Data rate of the compressed stream in bits per second |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_NOT_SUPPORTED``` | Profile change is not supported by this stream |
| ```ERROR_ILLEGAL_STATE``` | The stream is not ready for this operation |
| ```ERROR_BAD_REQUEST``` | The profile specified is invalid |
| ```ERROR_INPROGRESS``` | Stream is started, profile will be changed for the next streaming |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothRemoteControl.1.audioprofile"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "codec": "IMA-ADPCM",
    "codecparams": {},
    "channels": 1,
    "resolution": 16,
    "samplerate": 16000,
    "bitrate": 64000
  }
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothRemoteControl.1.audioprofile",
  "params": {
    "value": {
      "codec": "IMA-ADPCM",
      "codecparams": {},
      "channels": 1,
      "resolution": 16,
      "samplerate": 16000,
      "bitrate": 64000
    }
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

<a id="property_time"></a>
## *time [<sup>property</sup>](#head_Properties)*

Provides access to the stream position.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | integer | mandatory | Stream position |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_NOT_SUPPORTED``` | Time reporting is not supported by this stream |
| ```ERROR_ILLEGAL_STATE``` | The stream is not ready for this operation |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothRemoteControl.1.time"
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

<a id="property_speed"></a>
## *speed [<sup>property</sup>](#head_Properties)*

Provides access to the stream speed.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Stream speed |
| (property).value | integer | mandatory | *...* |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | integer | mandatory | Stream speed |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_NOT_SUPPORTED``` | Speed setting is not supported by this stream |
| ```ERROR_ILLEGAL_STATE``` | The stream is not ready for this operation |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothRemoteControl.1.speed"
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
  "method": "BluetoothRemoteControl.1.speed",
  "params": {
    "value": 0
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

<a id="head_Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the BluetoothRemoteControl plugin:

BluetoothRemoteControl interface events:

| Notification | Description |
| :-------- | :-------- |
| [batterylevelchange](#notification_batterylevelchange) | Signals battery level change |

AudioStream interface events:

| Notification | Description |
| :-------- | :-------- |
| [audiotransmission](#notification_audiotransmission) | Signals state of the stream |
| [audioframe](#notification_audioframe) | Provides audio data |

<a id="notification_batterylevelchange"></a>
## *batterylevelchange [<sup>notification</sup>](#head_Notifications)*

Signals battery level change.

> This notification may also be triggered by client registration.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.level | integer | mandatory | Battery level in percent |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothRemoteControl.1.register",
  "params": {
    "event": "batterylevelchange",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.batterylevelchange",
  "params": {
    "level": 75
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.batterylevelchange``.

<a id="notification_audiotransmission"></a>
## *audiotransmission [<sup>notification</sup>](#head_Notifications)*

Signals state of the stream.

> This notification may also be triggered by client registration.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.state | string | mandatory | New state of the stream (must be one of the following: *Idle, Started, Unavailable*) |
| params?.profile | object | optional | Details on the format used in the stream |
| params?.profile.codec | string | mandatory | Compression method (PCM: uncompressed) (must be one of the following: *IMA-ADPCM, PCM*) |
| params?.profile?.codecparams | opaque object | optional | Additional parameters for codec |
| params?.profile.channels | integer | mandatory | Number of audio channels |
| params?.profile.resolution | integer | mandatory | Sample resultion in bits |
| params?.profile.samplerate | integer | mandatory | Sample rate in hertz |
| params?.profile?.bitrate | integer | optional | Data rate of the compressed stream in bits per second |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothRemoteControl.1.register",
  "params": {
    "event": "audiotransmission",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.audiotransmission",
  "params": {
    "state": "Idle",
    "profile": {
      "codec": "IMA-ADPCM",
      "codecparams": {},
      "channels": 1,
      "resolution": 16,
      "samplerate": 16000,
      "bitrate": 64000
    }
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.audiotransmission``.

<a id="notification_audioframe"></a>
## *audioframe [<sup>notification</sup>](#head_Notifications)*

Provides audio data.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params?.seq | integer | optional | Frame number in current transmission |
| params?.timestamp | integer | optional | Timestamp of the frame |
| params.length | integer | mandatory | Size of the raw data frame in bytes |
| params.data | string (base64) | mandatory | Raw audio data, the format of the data is specified in the most recent *audiotransmission* notification |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothRemoteControl.1.register",
  "params": {
    "event": "audioframe",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.audioframe",
  "params": {
    "seq": 1,
    "timestamp": 0,
    "length": 400,
    "data": "..."
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.audioframe``.

