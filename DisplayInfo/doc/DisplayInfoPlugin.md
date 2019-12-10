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

DisplayInfo interface properties:

| Property | Description |
| :-------- | :-------- |
| [displayinfo](#property.displayinfo) <sup>RO</sup> | Display general information |

<a name="property.displayinfo"></a>
## *displayinfo <sup>property</sup>*

Provides access to the display general information.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Display general information |
| (property).totalgpuram | number | Total GPU DRAM memory (in bytes) |
| (property).freegpuram | number | Free GPU DRAM memory (in bytes) |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "DisplayInfo.1.displayinfo"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": {
        "totalgpuram": 381681664, 
        "freegpuram": 358612992
    }
}
```
