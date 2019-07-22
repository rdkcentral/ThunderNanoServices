<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Compositor_Plugin"></a>
# Compositor Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

Compositor plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)
- [Properties](#head.Properties)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the Compositor plugin. It includes detailed specification of its configuration, methods and properties provided.

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

Compositor gives you controll over what is displayed on screen.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *Compositor*) |
| classname | string | Class name: *Compositor* |
| locator | string | Library name: *libWPEFrameworkCompositor.so* |
| autostart | boolean | Determines if the plugin is to be started automatically along with the framework |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the Compositor plugin:

Compositor interface methods:

| Method | Description |
| :-------- | :-------- |
| [totop](#method.totop) | Put client on top |
| [putbelow](#method.putbelow) | Slide <below> just behind <above> |
| [kill](#method.kill) | Use this method to kill the client |

<a name="method.totop"></a>
## *totop <sup>method</sup>*

Put client on top.

### Description

Whenever you put client on top, it will shown on the screen and it will be in focus.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string | Client's callsign |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 34 | ```ERROR_FIRST_RESOURCE_NOT_FOUND``` | Client not found |
| 1 | ```ERROR_GENERAL``` | Compositor couldn't put service on top |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Compositor.1.totop", 
    "params": {
        "callsign": ""
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
<a name="method.putbelow"></a>
## *putbelow <sup>method</sup>*

Slide <below> just behind <above>

### Description

Whenever you want to reorder the client-list, you can slide a <client-a> just below a <client-b> in the client-list.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params?.below | string | <sup>*(optional)*</sup> Client that will be put below second one |
| params?.above | string | <sup>*(optional)*</sup> Client that will be put above fist one |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 34 | ```ERROR_FIRST_RESOURCE_NOT_FOUND``` | At least one of clients not found |
| 1 | ```ERROR_GENERAL``` | Compositor failed to put service on top |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Compositor.1.putbelow", 
    "params": {
        "below": "Netflix", 
        "above": "WebKitBrowser"
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
<a name="method.kill"></a>
## *kill <sup>method</sup>*

Use this method to kill the client

### Description

Whenever a client is killed, the execution of the client is stopped and all its resources will be released..

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string | Callsign of killed client |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 34 | ```ERROR_FIRST_RESOURCE_NOT_FOUND``` | Client not found |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Compositor.1.kill", 
    "params": {
        "callsign": "Netflix"
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
<a name="head.Properties"></a>
# Properties

The following properties are provided by the Compositor plugin:

Compositor interface properties:

| Property | Description |
| :-------- | :-------- |
| [clients](#property.clients) <sup>RO</sup> | List of compositor clients |
| [resolution](#property.resolution) | Output resolution |
| [zorder](#property.zorder) <sup>RO</sup> | List of compositor clients sorted by z-order |
| [geometry](#property.geometry) | Property describing nanoservice window on screen |
| [visible](#property.visible) <sup>WO</sup> | Determines if client's surface is visible |
| [opacity](#property.opacity) <sup>WO</sup> | Opacity of client |

<a name="property.clients"></a>
## *clients <sup>property</sup>*

Provides access to the list of compositor clients.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | List of compositor clients |
| (property)[#] | string | Client callsign |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Compositor.1.clients"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        "Netflix"
    ]
}
```
<a name="property.resolution"></a>
## *resolution <sup>property</sup>*

Provides access to the output resolution.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | Output resolution (must be one of the following: *screenresolution_unknown*, *screenresolution_480i*, *screenresolution_480p*, *screenresolution_720p*, *screenresolution_720p50hz*, *screenresolution_1080p24hz*, *screenresolution_1080i50hz*, *screenresolution_1080p50hz *, *screenresolution_1080p60hz*, *screenresolution_2160p50hz*, *screenresolution_2160p60hz*) |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 28 | ```ERROR_UNKNOWN_TABLE``` | Unknown resolution |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Compositor.1.resolution"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": "screenresolution_480i"
}
```
#### Set Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Compositor.1.resolution", 
    "params": "screenresolution_480i"
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
<a name="property.zorder"></a>
## *zorder <sup>property</sup>*

Provides access to the list of compositor clients sorted by z-order.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | List of compositor clients sorted by z-order |
| (property)[#] | string | Client name |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Compositor.1.zorder"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        "Netflix"
    ]
}
```
<a name="property.geometry"></a>
## *geometry <sup>property</sup>*

Provides access to the property describing nanoservice window on screen.

### Description

Use this property to update or retrive the geometry of the client surface.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Property describing nanoservice window on screen |
| (property).x | number | X position on screen |
| (property).y | number | Y position on screen |
| (property).width | number | Width of space taken by nanoservice |
| (property).height | number | Height of space taken by nanoservice |

> The *Nanoservice callsign* shall be passed as the index to the property, e.g. *Compositor.1.geometry@Netflix*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 34 | ```ERROR_FIRST_RESOURCE_NOT_FOUND``` | Callsign not found |
| 1 | ```ERROR_GENERAL``` | Failed to set/get geometry |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Compositor.1.geometry@Netflix"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": {
        "x": 0, 
        "y": 0, 
        "width": 1280, 
        "height": 720
    }
}
```
#### Set Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Compositor.1.geometry@Netflix", 
    "params": {
        "x": 0, 
        "y": 0, 
        "width": 1280, 
        "height": 720
    }
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
<a name="property.visible"></a>
## *visible <sup>property</sup>*

Provides access to the determines if client's surface is visible.

> This property is **write-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | boolean | Determines if client's surface is visible |

> The *Nanoservice callsign* shall be passed as the index to the property, e.g. *Compositor.1.visible@Netflix*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 34 | ```ERROR_FIRST_RESOURCE_NOT_FOUND``` | Callsign not found |

### Example

#### Set Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Compositor.1.visible@Netflix", 
    "params": true
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
<a name="property.opacity"></a>
## *opacity <sup>property</sup>*

Provides access to the opacity of client.

> This property is **write-only**.

### Description

The opacity of a client surface can range from 0 till 255, that will represent an opacity of the surface from 0% till 100%.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | number | Opacity of client |

> The *Nanoservice callsign* shall be passed as the index to the property, e.g. *Compositor.1.opacity@Netflix*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 34 | ```ERROR_FIRST_RESOURCE_NOT_FOUND``` | Callsign not found |

### Example

#### Set Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Compositor.1.opacity@Netflix", 
    "params": 64
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
