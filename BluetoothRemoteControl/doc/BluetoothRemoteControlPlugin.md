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
- [Methods](#head.Methods)
- [Properties](#head.Properties)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the BluetoothRemoteControl plugin. It includes detailed specification about its configuration, methods and properties provided, as well as notifications sent.

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

The Bluetooth Remote Control plugin allows configuring and enabling Bluetooth remote control units.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *BluetoothRemoteControl*) |
| classname | string | Class name: *BluetoothRemoteControl* |
| locator | string | Library name: *libWPEFrameworkBluetoothRemoteControl.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |
| configuration | object | <sup>*(optional)*</sup>  |
| configuration?.controller | string | <sup>*(optional)*</sup> Controller |
| configuration?.keymap | string | <sup>*(optional)*</sup> Keymap |
| configuration?.keyingest | boolean | <sup>*(optional)*</sup> Enable keyingest |
| configuration?.recorder | enum | <sup>*(optional)*</sup> Recorder |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the BluetoothRemoteControl plugin:

BluetoothRemoteControl interface methods:

| Method | Description |
| :-------- | :-------- |
| [assign](#method.assign) | Assigns a bluetooth device as a remote control unit |
| [revoke](#method.revoke) | Revokes the current remote control assignment |


<a name="method.assign"></a>
## *assign <sup>method</sup>*

Assigns a bluetooth device as a remote control unit.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Bluetooth unavailable |
| 22 | ```ERROR_UNKNOWN_KEY``` | Device unknown |
| 1 | ```ERROR_GENERAL``` | Failed to assign the device |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "BluetoothRemoteControl.1.assign",
    "params": {
        "address": "81:6F:B0:91:9B:FE"
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

<a name="method.revoke"></a>
## *revoke <sup>method</sup>*

Revokes the current remote control assignment.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 5 | ```ERROR_ILLEGAL_STATE``` | Remote not assigned |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "BluetoothRemoteControl.1.revoke"
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

The following properties are provided by the BluetoothRemoteControl plugin:

BluetoothRemoteControl interface properties:

| Property | Description |
| :-------- | :-------- |
| [name](#property.name) <sup>RO</sup> | Unit name |
| [address](#property.address) <sup>RO</sup> | Bluetooth address of the unit |
| [info](#property.info) <sup>RO</sup> | Unit auxiliary information |
| [batterylevel](#property.batterylevel) <sup>RO</sup> | Battery level |
| [voice](#property.voice) | Enable or Disable the flow of Voice data fragments from the remote |
| [audioprofile](#property.audioprofile) <sup>RO</sup> | Audio profile details |


<a name="property.name"></a>
## *name <sup>property</sup>*

Provides access to the unit name.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | Name of the remote control unit |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 5 | ```ERROR_ILLEGAL_STATE``` | No remote has been assigned |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "BluetoothRemoteControl.1.name"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": "Acme Bluetooth RCU"
}
```

<a name="property.address"></a>
## *address <sup>property</sup>*

Provides access to the bluetooth address of the unit.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | Bluetooth address |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 5 | ```ERROR_ILLEGAL_STATE``` | No remote has been assigned |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "BluetoothRemoteControl.1.address"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": "81:6F:B0:91:9B:FE"
}
```

<a name="property.info"></a>
## *info <sup>property</sup>*

Provides access to the unit auxiliary information.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Unit auxiliary information |
| (property)?.model | string | <sup>*(optional)*</sup> Unit model name or number |
| (property)?.serial | string | <sup>*(optional)*</sup> Unit serial number |
| (property)?.firmware | string | <sup>*(optional)*</sup> Unit firmware revision |
| (property)?.software | string | <sup>*(optional)*</sup> Unit software revision |
| (property)?.manufacturer | string | <sup>*(optional)*</sup> Unit manufacturer name |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 5 | ```ERROR_ILLEGAL_STATE``` | No remote has been assigned |
| 1 | ```ERROR_GENERAL``` | Failed to retrieve information |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "BluetoothRemoteControl.1.info"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": {
        "model": "Acme 1500 Plus",
        "serial": "1234567890",
        "firmware": "1.0",
        "software": "1.0",
        "manufacturer": "Acme Inc."
    }
}
```

<a name="property.batterylevel"></a>
## *batterylevel <sup>property</sup>*

Provides access to the battery level.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | number | Remote control unit's battery level in percentage |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 5 | ```ERROR_ILLEGAL_STATE``` | No remote has been assigned |
| 1 | ```ERROR_GENERAL``` | Failed to retrieve battery level |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "BluetoothRemoteControl.1.batterylevel"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": 50
}
```

<a name="property.voice"></a>
## *voice <sup>property</sup>*

Provides access to the enable or Disable the flow of Voice data fragments from the remote.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | boolean | enable (true) or disable (false) flow of voice data |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 1 | ```ERROR_GENERAL``` | Failed to set the voice flow |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "BluetoothRemoteControl.1.voice"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": false
}
```

#### Set Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "BluetoothRemoteControl.1.voice",
    "params": false
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

<a name="property.audioprofile"></a>
## *audioprofile <sup>property</sup>*

Provides access to the audio profile details.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Audio profile details |
| (property).codec | string | Name of the audio codec (*pcm* for uncompressed audio) (must be one of the following: *PCM*, *ADPCM*) |
| (property).channels | number | Number of audio channels (1: mono, 2: stereo, etc.) |
| (property).rate | number | Sample rate (in Hz) |
| (property).resolution | number | Sample resolution (in bits per sample) |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 5 | ```ERROR_ILLEGAL_STATE``` | No remote has been assigned |
| 22 | ```ERROR_UNKNOWN_KEY``` | The supplied audio profile is unknown |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "BluetoothRemoteControl.1.audioprofile"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": {
        "codec": "ADPCM",
        "channels": 1,
        "rate": 16000,
        "resolution": 16
    }
}
```

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the implementation, and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the BluetoothRemoteControl plugin:

BluetoothRemoteControl interface events:

| Event | Description |
| :-------- | :-------- |
| [audiotransmission](#event.audiotransmission) | Notifies about audio data transmission |
| [audioframe](#event.audioframe) | Notifies about new audio data available |
| [batterylevelchange](#event.batterylevelchange) | Notifies about battery level changes |


<a name="event.audiotransmission"></a>
## *audiotransmission <sup>event</sup>*

Notifies about audio data transmission.

### Description

Register to this event to be notified about audio transmission status

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params?.profile | string | <sup>*(optional)*</sup> Type of audio profile, marking start of transmission; empty in case of end of transmission |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.audiotransmission",
    "params": {
        "profile": "pcm"
    }
}
```

<a name="event.audioframe"></a>
## *audioframe <sup>event</sup>*

Notifies about new audio data available.

### Description

Register to this event to be notified about audio data

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params?.seq | number | <sup>*(optional)*</sup> Sequence number of the audio frame within current audio transmission |
| params.data | string | Base64 representation of the binary data of the audio frame; format of the data is specified by the audio profile denoted by the most recent *audiotransmission* notification |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.audioframe",
    "params": {
        "seq": 1,
        "data": "yKMHiYh6qJiDspB6S7ihlKOAbivApYEQDCgY0aECCQhpkAqZogP1ECk9GbHGEAkwG8Ax8wArgaAtEMjGQIoYCKKgGCuzBSA/iuWkKEwamLKzOKoCeR2hwQQZKqgBKKqELoGQwQ=="
    }
}
```

<a name="event.batterylevelchange"></a>
## *batterylevelchange <sup>event</sup>*

Notifies about battery level changes.

### Description

Register to this event to be notified about battery level drops

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.level | number | Battery level (in percentage) |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.batterylevelchange",
    "params": {
        "level": 50
    }
}
```

