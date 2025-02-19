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
- [Interfaces](#head.Interfaces)
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
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20Thunder.docx)</a> | Thunder API Reference |

<a name="head.Description"></a>
# Description

Compositor gives you control over what is displayed on screen.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *Compositor*) |
| classname | string | mandatory | Class name: *Compositor* |
| locator | string | mandatory | Library name: *libThunderCompositor.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| configuration | object | optional | *...* |
| configuration?.hardwareready | integer | optional | Hardware delay (Nexus) |
| configuration?.resolution | string | optional | Screen resolution (Nexus) |
| configuration?.allowedclients | array | optional | List of allowed clients (Nexus) |
| configuration?.allowedclients[#] | string | optional | *...* |
| configuration?.connector | enum | optional | Resolution (Wayland) |
| configuration?.join | boolean | optional | Enable join (Wayland) |
| configuration?.display | string | optional | Display (Westeros) |
| configuration?.renderer | string | optional | Path of renderer (Westeros) |
| configuration?.glname | string | optional | Name of GL-library (Westeros) |
| configuration?.width | string | optional | Screen width (Westeros) |
| configuration?.height | string | optional | Screen height (Westeros) |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- [Compositor.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/Compositor.json) (version 1.0.0) (uncompliant-extended format)

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
## *putontop [<sup>method</sup>](#head.Methods)*

Puts client surface on top in z-order.

### Description

Use this method to get a client's surface to the top position.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.client | string | mandatory | Client name |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Client not found |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
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
  "id": 42,
  "result": null
}
```

<a name="method.putbelow"></a>
## *putbelow [<sup>method</sup>](#head.Methods)*

Puts client surface below another surface.

### Description

Use this method to reorder client surfaces in the z-order list.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.client | string | mandatory | Client name to change z-order position |
| params.relative | string | mandatory | Client to put the other surface below |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Client(s) not found |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
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
  "id": 42,
  "result": null
}
```

<a name="method.select"></a>
## *select [<sup>method</sup>](#head.Methods)*

Directs the input to the given client, disabling all the others.

### Description

Use this method to direct all inputs to this client. The client that is receiving the inputs prior to this call will nolonger receive it anymore after this call.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.client | string | mandatory | Client name |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Client not found |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
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
  "id": 42,
  "result": null
}
```

<a name="head.Properties"></a>
# Properties

The following properties are provided by the Compositor plugin:

Compositor interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [resolution](#property.resolution) | read/write | Screen resolution |
| [zorder](#property.zorder) | read-only | List of compositor clients sorted by z-order |
| [brightness](#property.brightness) | read/write | Brightness of SDR graphics in HDR display |
| [geometry](#property.geometry) | read/write | Client surface geometry |
| [visiblity](#property.visiblity) | write-only | Client surface visibility |
| [opacity](#property.opacity) | write-only | Client surface opacity |

<a name="property.resolution"></a>
## *resolution [<sup>property</sup>](#head.Properties)*

Provides access to the screen resolution.

### Description

Use this property to set or retrieve the current resolution of the screen.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Screen resolution (must be one of the following: *1080i50, 1080p24, 1080p50, 1080p60, 2160p50, 2160p60, 480i, 480p, 720p50, 720p60, unknown*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | Screen resolution (must be one of the following: *1080i50, 1080p24, 1080p50, 1080p60, 2160p50, 2160p60, 480i, 480p, 720p50, 720p60, unknown*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown resolution |
| ```ERROR_UNAVAILABLE``` | Set resolution is not supported or failed |
| ```ERROR_GENERAL``` | Failed to set resolution |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Compositor.1.resolution"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "1080p24"
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Compositor.1.resolution",
  "params": "1080p24"
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

<a name="property.zorder"></a>
## *zorder [<sup>property</sup>](#head.Properties)*

Provides access to the list of compositor clients sorted by z-order.

> This property is **read-only**.

### Description

Use this property to retrieve the list of all clients in z-order. Each client has an z-order-value that determines its position with respect to the screen. The ordering is that the top position is closest to the screen, the next z-order-value first behind the top, and so on.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | List of compositor clients sorted by z-order |
| result[#] | string | mandatory | Client name |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_GENERAL``` | Failed to get z-order |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Compositor.1.zorder"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    "Netflix"
  ]
}
```

<a name="property.brightness"></a>
## *brightness [<sup>property</sup>](#head.Properties)*

Provides access to the brightness of SDR graphics in HDR display.

### Description

Use this property to set or retrieve the brightness of the SDR graphics.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Brightness of SDR graphics in HDR display (must be one of the following: *default, match_video, max*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | Brightness of SDR graphics in HDR display (must be one of the following: *default, match_video, max*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown brightness |
| ```ERROR_UNAVAILABLE``` | Set brightness is not supported or failed |
| ```ERROR_GENERAL``` | Failed to set brightness |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Compositor.1.brightness"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "match_video"
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Compositor.1.brightness",
  "params": "match_video"
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

<a name="property.geometry"></a>
## *geometry [<sup>property</sup>](#head.Properties)*

Provides access to the client surface geometry.

### Description

Use this property to update or retrieve the geometry of a client's surface.

> The *client* parameter shall be passed as the index to the property, e.g. ``Compositor.1.geometry@<client>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| client | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Client surface geometry |
| (property).x | integer | mandatory | Horizontal coordinate of the surface |
| (property).y | integer | mandatory | Vertical coordinate of the surface |
| (property).width | integer | mandatory | Surface width |
| (property).height | integer | mandatory | Surface height |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | Client surface geometry |
| result.x | integer | mandatory | Horizontal coordinate of the surface |
| result.y | integer | mandatory | Vertical coordinate of the surface |
| result.width | integer | mandatory | Surface width |
| result.height | integer | mandatory | Surface height |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Client not found |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Compositor.1.geometry@Netflix"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
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
  "id": 42,
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
    "id": 42,
    "result": "null"
}
```

<a name="property.visiblity"></a>
## *visiblity [<sup>property</sup>](#head.Properties)*

Provides access to the client surface visibility.

> This property is **write-only**.

### Description

Use this property to set the client's surface visibility.

> The *client* parameter shall be passed as the index to the property, e.g. ``Compositor.1.visiblity@<client>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| client | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Client surface visibility (must be one of the following: *hidden, visible*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Client not found |

### Example

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Compositor.1.visiblity@Netflix",
  "params": "visible"
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

<a name="property.opacity"></a>
## *opacity [<sup>property</sup>](#head.Properties)*

Provides access to the client surface opacity.

> This property is **write-only**.

### Description

Use this property to set the client's surface opacity level.

> The *client* parameter shall be passed as the index to the property, e.g. ``Compositor.1.opacity@<client>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| client | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | integer | mandatory | Opacity level (0 to 255; 0: fully transparent, 255: fully opaque) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Client not found |

### Example

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Compositor.1.opacity@Netflix",
  "params": 127
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

