<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Bluetooth_Remote_Control_Plugin"></a>
# Bluetooth Remote Control Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

BluetoothRemoteControl plugin for Thunder framework.

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

This document describes purpose and functionality of the BluetoothRemoteControl plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

<a name="head.Case_Sensitivity"></a>
## Case Sensitivity

All identifiers of the interfaces described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a name="head.Acronyms,_Abbreviations_and_Terms"></a>
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
| <a name="acronym.WAV">WAV</a> | Waveform Audio File Format |

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

The Bluetooth Remote Control plugin allows configuring and enabling Bluetooth LE remote control units.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
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

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- IBluetoothRemoteControlLegacy ([IBluetoothRemoteControl.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IBluetoothRemoteControl.h)) (version 1.0.0) (compliant format)
- IBluetoothRemoteControl ([IBluetoothRemoteControl.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IBluetoothRemoteControl.h)) (version 1.0.0) (compliant format)

<a name="head.Methods"></a>
# Methods

The following methods are provided by the BluetoothRemoteControl plugin:

BluetoothRemoteControl interface methods:

| Method | Description |
| :-------- | :-------- |
| [assign](#method.assign) | Assigns a Bluetooth device as a RCU |
| [revoke](#method.revoke) | Revokes a Bluetooth device from RCU operation |

<a name="method.assign"></a>
## *assign [<sup>method</sup>](#head.Methods)*

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

<a name="method.revoke"></a>
## *revoke [<sup>method</sup>](#head.Methods)*

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

<a name="head.Properties"></a>
# Properties

The following properties are provided by the BluetoothRemoteControl plugin:

BluetoothRemoteControlLegacy interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [name](#property.name) <sup>deprecated</sup> | read-only | Name of the RCU device |

BluetoothRemoteControl interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [device](#property.device) / [address](#property.device) | read-only | Bluetooth address |
| [metadata](#property.metadata) / [info](#property.metadata) | read-only | Device metadata |
| [batterylevel](#property.batterylevel) | read-only | Battery level |
| [voicecontrol](#property.voicecontrol) | read/write | Toggle voice control |
| [audioprofile](#property.audioprofile) | read-only | Details of used audio format |

<a name="property.name"></a>
## *name [<sup>property</sup>](#head.Properties)*

Provides access to the name of the RCU device.

> This property is **read-only**.

> ``name`` is an alternative name for this property. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | Name of the RCU device |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | The RCU device is not assigned |

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

<a name="property.device"></a>
## *device [<sup>property</sup>](#head.Properties)*

Provides access to the bluetooth address.

> This property is **read-only**.

> ``address`` is an alternative name for this property. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | Bluetooth address |

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

<a name="property.metadata"></a>
## *metadata [<sup>property</sup>](#head.Properties)*

Provides access to the device metadata.

> This property is **read-only**.

> ``info`` is an alternative name for this property. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | Device metadata |
| result.name | string | mandatory | Name of the unit |
| result?.model | string | optional | Model name |
| result?.serial | string | optional | Serial number |
| result?.firmware | string | optional | Firmware version |
| result?.software | string | optional | Software version |
| result?.manufacturer | string | optional | Vendor/manufacturer name |

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

<a name="property.batterylevel"></a>
## *batterylevel [<sup>property</sup>](#head.Properties)*

Provides access to the battery level.

> This property is **read-only**.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | integer | mandatory | Battery level |

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

<a name="property.voicecontrol"></a>
## *voicecontrol [<sup>property</sup>](#head.Properties)*

Provides access to the toggle voice control.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Toggle voice control |
| (property).value | boolean | mandatory | *...* |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | boolean | mandatory | Toggle voice control |

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

<a name="property.audioprofile"></a>
## *audioprofile [<sup>property</sup>](#head.Properties)*

Provides access to the details of used audio format.

> This property is **read-only**.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | Details of used audio format |
| result.codec | string | mandatory | Compression method (pcm: uncompressed) (must be one of the following: *ima-adpcm, pcm*) |
| result.channels | integer | mandatory | Number of audio channels |
| result.resolution | integer | mandatory | Sample resultion in bits |
| result.samplerate | integer | mandatory | Sample rate in hertz |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_ILLEGAL_STATE``` | The RCU device currently is not assigned |
| ```ERROR_NOT_SUPPORTED``` | The device does not support voice input |

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
    "codec": "pcm",
    "channels": 1,
    "resolution": 16,
    "samplerate": 16000
  }
}
```

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the BluetoothRemoteControl plugin:

BluetoothRemoteControl interface events:

| Notification | Description |
| :-------- | :-------- |
| [audiotransmission](#notification.audiotransmission) | Signals beginning end of audio transmission |
| [audioframe](#notification.audioframe) | Provides audio frame data |
| [batterylevelchange](#notification.batterylevelchange) | Signals battery level change |

<a name="notification.audiotransmission"></a>
## *audiotransmission [<sup>notification</sup>](#head.Notifications)*

Signals beginning end of audio transmission.

> If applicable, this notification may be sent out during registration, reflecting the current status.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.state | string | mandatory | New state of the voice transmission (must be one of the following: *Started, Stopped*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothRemoteControl.1.register",
  "params": {
    "event": "audiotransmission",
    "id": "client"
  }
}
```

#### Message

```json
{
  "jsonrpc": "2.0",
  "method": "client.audiotransmission",
  "params": {
    "state": "Stopped"
  }
}
```

<a name="notification.audioframe"></a>
## *audioframe [<sup>notification</sup>](#head.Notifications)*

Provides audio frame data.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.seq | integer | mandatory | Frame number in current transmission |
| params.size | integer | mandatory | Size of the raw data frame in bytes |
| params.data | string (base64) | mandatory | Raw audio data |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothRemoteControl.1.register",
  "params": {
    "event": "audioframe",
    "id": "client"
  }
}
```

#### Message

```json
{
  "jsonrpc": "2.0",
  "method": "client.audioframe",
  "params": {
    "seq": 1,
    "size": 400,
    "data": "..."
  }
}
```

<a name="notification.batterylevelchange"></a>
## *batterylevelchange [<sup>notification</sup>](#head.Notifications)*

Signals battery level change.

> If applicable, this notification may be sent out during registration, reflecting the current status.

### Parameters

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
    "id": "client"
  }
}
```

#### Message

```json
{
  "jsonrpc": "2.0",
  "method": "client.batterylevelchange",
  "params": {
    "level": 75
  }
}
```

