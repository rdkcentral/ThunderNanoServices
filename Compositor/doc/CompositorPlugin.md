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

This document describes purpose and functionality of the Compositor plugin. It includes detailed specification about its configuration, methods and properties provided.

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

Compositor gives you control over what is displayed on screen.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *Compositor*) |
| classname | string | Class name: *Compositor* |
| locator | string | Library name: *libWPEFrameworkCompositor.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |
| configuration | object | <sup>*(optional)*</sup>  |
| configuration?.hardwareready | number | <sup>*(optional)*</sup> Hardware delay (Nexus) |
| configuration?.resolution | string | <sup>*(optional)*</sup> Screen resolution (Nexus) |
| configuration?.allowedclients | array | <sup>*(optional)*</sup> List of allowed clients (Nexus) |
| configuration?.allowedclients[#] | string | <sup>*(optional)*</sup>  |
| configuration?.connector | enum | <sup>*(optional)*</sup> Resolution (Wayland) |
| configuration?.join | boolean | <sup>*(optional)*</sup> Enable join (Wayland) |
| configuration?.display | string | <sup>*(optional)*</sup> Display (Westeros) |
| configuration?.renderer | string | <sup>*(optional)*</sup> Path of renderer (Westeros) |
| configuration?.glname | string | <sup>*(optional)*</sup> Name of GL-library (Westeros) |
| configuration?.width | string | <sup>*(optional)*</sup> Screen width (Westeros) |
| configuration?.height | string | <sup>*(optional)*</sup> Screen height (Westeros) |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the Compositor plugin:

Compositor interface methods:

| Method | Description |
| :-------- | :-------- |
| [putontop](#method.putontop) | Puts client surface on top in z-order |
| [putbelow](#method.putbelow) | Puts client surface below another surface |
| [select](#method.select) | Directs the input to the given client, disabling all the others |


<a name="method.putontop"></a>
## *putontop <sup>method</sup>*

Puts client surface on top in z-order.

### Description

Use this method to get a client's surface to the top position.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.client | string | Client name |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Client not found |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Compositor.1.putontop",
    "params": {
        "client": "Netflix"
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

Puts client surface below another surface.

### Description

Use this method to reorder client surfaces in the z-order list.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.client | string | Client name to change z-order position |
| params.relative | string | Client to put the other surface below |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Client(s) not found |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Compositor.1.putbelow",
    "params": {
        "client": "Netflix",
        "relative": "WebKitBrowser"
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

<a name="method.select"></a>
## *select <sup>method</sup>*

Directs the input to the given client, disabling all the others.

### Description

Use this method to direct all inputs to this client. The client that is receiving the inputs prior to this call will nolonger receive it anymore after this call.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.client | string | Client name |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Client not found |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Compositor.1.select",
    "params": {
        "client": "Netflix"
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
| [resolution](#property.resolution) | Screen resolution |
| [zorder](#property.zorder) <sup>RO</sup> | List of compositor clients sorted by z-order |
| [geometry](#property.geometry) | Client surface geometry |
| [visiblity](#property.visiblity) <sup>WO</sup> | Client surface visibility |
| [opacity](#property.opacity) <sup>WO</sup> | Client surface opacity |


<a name="property.resolution"></a>
## *resolution <sup>property</sup>*

Provides access to the screen resolution.

### Description

Use this property to set or retrieve the current resolution of the screen.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | Screen resolution (must be one of the following: *unknown*, *480i*, *480p*, *720p50*, *720p60*, *1080p24*, *1080i50*, *1080p50*, *1080p60*, *2160p50*, *2160p60*) |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown resolution |
| 2 | ```ERROR_UNAVAILABLE``` | Set resolution is not supported or failed |
| 1 | ```ERROR_GENERAL``` | Failed to set resolution |

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
    "result": "1080p24"
}
```

#### Set Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Compositor.1.resolution",
    "params": "1080p24"
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

### Description

Use this property to retrieve the list of all clients in z-order. Each client has an z-order-value that determines its position with respect to the screen. The ordering is that the top position is closest to the screen, the next z-order-value first behind the top, and so on.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | List of compositor clients sorted by z-order |
| (property)[#] | string | Client name |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 1 | ```ERROR_GENERAL``` | Failed to get z-order |

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

Provides access to the client surface geometry.

### Description

Use this property to update or retrieve the geometry of a client's surface.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Client surface geometry |
| (property).x | number | Horizontal coordinate of the surface |
| (property).y | number | Vertical coordinate of the surface |
| (property).width | number | Surface width |
| (property).height | number | Surface height |

> The *client* shall be passed as the index to the property, e.g. *Compositor.1.geometry@Netflix*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Client not found |

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

<a name="property.visiblity"></a>
## *visiblity <sup>property</sup>*

Provides access to the client surface visibility.

> This property is **write-only**.

### Description

Use this property to set the client's surface visibility.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | Client surface visibility (must be one of the following: *visible*, *hidden*) |

> The *client* shall be passed as the index to the property, e.g. *Compositor.1.visiblity@Netflix*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Client not found |

### Example

#### Set Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Compositor.1.visiblity@Netflix",
    "params": "visible"
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

Provides access to the client surface opacity.

> This property is **write-only**.

### Description

Use this property to set the client's surface opacity level.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | number | Opacity level (0 to 255; 0: fully transparent, 255: fully opaque) |

> The *client* shall be passed as the index to the property, e.g. *Compositor.1.opacity@Netflix*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Client not found |

### Example

#### Set Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Compositor.1.opacity@Netflix",
    "params": 127
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

