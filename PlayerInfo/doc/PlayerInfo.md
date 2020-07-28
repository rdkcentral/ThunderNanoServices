<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.PlayerInfo_Plugin"></a>
# PlayerInfo Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

PlayerInfo plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Configuration](#head.Configuration)
- [Properties](#head.Properties)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the PlayerInfo plugin. It includes detailed specification of its configuration and properties provided.

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
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | Thunder API Reference |

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *PlayerInfo*) |
| classname | string | Class name: *PlayerInfo* |
| locator | string | Library name: *libWPEFrameworkPlayerInfo.so* |
| autostart | boolean | Determines if the plugin is to be started automatically along with the framework |

<a name="head.Properties"></a>
# Properties

The following properties are provided by the PlayerInfo plugin:

PlayerInfo interface properties:

| Property | Description |
| :-------- | :-------- |
| [playerinfo](#property.playerinfo) <sup>RO</sup> | Player general information |
| [dolbymode](#property.dolbymode) | Dolby output mode |

<a name="property.playerinfo"></a>
## *playerinfo <sup>property</sup>*

Provides access to the player general information.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Player general information |
| (property)?.audio | array | <sup>*(optional)*</sup>  |
| (property)?.audio[#] | string | <sup>*(optional)*</sup> Audio Codec supported by the platform (must be one of the following: *Undefined*, *AAC*, *AC3*, *AC3Plus*, *DTS*, *MPEG1*, *MPEG2*, *MPEG3*, *MPEG4*, *OPUS*, *VorbisOGG*, *WAV*) |
| (property)?.video | array | <sup>*(optional)*</sup>  |
| (property)?.video[#] | string | <sup>*(optional)*</sup> Video Codec supported by the platform (must be one of the following: *Undefined*, *H263*, *H264*, *H265*, *H26510*, *MPEG*, *VP8*, *VP9*, *VP10*) |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "PlayerInfo.1.playerinfo"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": {
        "audio": [
            "AudioAAC"
        ],
        "video": [
            "VideoH264"
        ]
    }
}
```
<a name="property.dolbymode"></a>
## *dolbymode <sup>property</sup>*

Provides access to the dolby output mode.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | Dolby output mode (must be one of the following: *DIGITAL_PCM*, *DIGITAL_PASS_THROUGH*, *ATMOS_PASS_THROUGH*, *AUTO*) |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "PlayerInfo.1.dolbymode"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": "DIGITAL_PCM"
}
```
#### Set Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "PlayerInfo.1.dolbymode",
    "params": "DIGITAL_PCM"
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
