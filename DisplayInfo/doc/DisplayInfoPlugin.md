<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Display_Info_Plugin"></a>
# Display Info Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

DisplayInfo plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Properties](#head.Properties)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the DisplayInfo plugin. It includes detailed specification of its configuration and properties provided.

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

<a name="head.Description"></a>
# Description

The DisplayInfo plugin allows retrieving of various display-related information.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *DisplayInfo*) |
| classname | string | Class name: *DisplayInfo* |
| locator | string | Library name: *libWPEFrameworkDisplayInfo.so* |
| autostart | boolean | Determines if the plugin is to be started automatically along with the framework |

<a name="head.Properties"></a>
# Properties

The following properties are provided by the DisplayInfo plugin:

GraphicsProperties interface properties:

| Property | Description |
| :-------- | :-------- |
| [totalgpuram](#property.totalgpuram) <sup>RO</sup> | Total GPU DRAM memory (in bytes) |
| [freegpuram](#property.freegpuram) <sup>RO</sup> | Free GPU DRAM memory (in bytes) |
| [audiopassthrough](#property.audiopassthrough)<sup>RO</sup> | Audio Pass through is support for this device |
| [connected](#property.connected)<sup>RO</sup> | HDMI display connection status |
| [width](#property.width)<sup>RO</sup> | Width of the connected HDMI display |
| [height](#property.height)<sup>RO</sup> | Height of the connected HDMI display |
| [hdcpprotection](#property.hdcpprotection)<sup>RO</sup> | HDCP Protection Type |
| [hdrsetting](#property.hdrsetting)<sup>RO</sup> | HDR Type used |

<a name="property.totalgpuram"></a>
## *totalgpuram <sup>property</sup>*

Provides access to the total GPU DRAM memory (in bytes).

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | integer | Total GPU DRAM memory (in bytes) |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "DisplayInfo.1.totalgpuram"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": 0
}
```
<a name="property.freegpuram"></a>
## *freegpuram <sup>property</sup>*

Provides access to the free GPU DRAM memory (in bytes).

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | integer | Free GPU DRAM memory (in bytes) |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "DisplayInfo.1.freegpuram"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": 0
}
```
<a name="property.audiopassthrough"></a>
## *audiopassthrough <sup>property</sup>*

Provides the status of the Audio Pass through is support for this device or not

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | boolean | Audio Pass through is support for this device |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "DisplayInfo.1.audiopassthrough"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": true
}
```
<a name="property.connected"></a>
## *totalgpuram <sup>property</sup>*

Provides status of HDMI display connected or not

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | boolean | HDMI display connection status |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "DisplayInfo.1.connected"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": true
}
```
<a name="property.width"></a>
## *totalgpuram <sup>property</sup>*

Provides width of the connected HDMI display.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | integer | Width of the connected HDMI display |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "DisplayInfo.1.width"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": 720
}
```
<a name="property.height"></a>
## *totalgpuram <sup>property</sup>*

Provides height of the connected HDMI display.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | integer | Height of the connected HDMI display |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "DisplayInfo.1.height"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": 1280
}
```
<a name="property.hdcpprotection"></a>
## *totalgpuram <sup>property</sup>*

Provides status of HDCP Protection

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string |HDCP Protection (must be one of the following: *Unencrypted*, *HDCP1x*, *HDCP2x*) |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "DisplayInfo.1.hdcpprotection"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": Unencrypted
}
```
<a name="property.hdrsetting"></a>
## *totalgpuram <sup>property</sup>*

Provides HDR Type used.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | HDR Type used (must be one of the following: *HDROff*, *HDR10*, *Resolution480P*, *HDR10Plus*, *HDRHLG*, *HDRDolbyVision*, *HDRTechnicolor*)) |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "DisplayInfo.1.hdrsetting"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": HDROff
}
`````