<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Process_Containers_Plugin"></a>
# Process Containers Plugin

**Version: 1.0**

**Status: :white_circle::white_circle::white_circle:**

Containers plugin for Thunder framework.

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

This document describes purpose and functionality of the Containers plugin. It includes detailed specification of its configuration, methods and properties provided.

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

The Containers plugin provides informations about process containers running on system.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *Containers*) |
| classname | string | Class name: *Containers* |
| locator | string | Library name: *libWPEContainers.so* |
| autostart | boolean | Determines if the plugin is to be started automatically along with the framework |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the Containers plugin:

Containers interface methods:

| Method | Description |
| :-------- | :-------- |
| [stop](#method.stop) | Stops a container |

<a name="method.stop"></a>
## *stop <sup>method</sup>*

Stops a container.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.name | string | Name of container |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Container not found |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Containers.1.stop", 
    "params": {
        "name": "ContainerName"
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

The following properties are provided by the Containers plugin:

Containers interface properties:

| Property | Description |
| :-------- | :-------- |
| [containers](#property.containers) <sup>RO</sup> | List of active containers |
| [networks](#property.networks) <sup>RO</sup> | List of networks in the container |
| [ip](#property.ip) <sup>RO</sup> | List of all ip addresses of the container |
| [memory](#property.memory) <sup>RO</sup> | Operating memory allocated to the container |
| [status](#property.status) <sup>RO</sup> | Operational status of the container |
| [cpu](#property.cpu) <sup>RO</sup> | CPU time allocated to running the container |
| [log](#property.log) <sup>RO</sup> | Containers log |
| [config](#property.config) <sup>RO</sup> | Container's configuration |

<a name="property.containers"></a>
## *containers <sup>property</sup>*

Provides access to the list of active containers.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | List of names of all containers |
| (property)[#] | string |  |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Containers.1.containers"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        "ContainerName"
    ]
}
```
<a name="property.networks"></a>
## *networks <sup>property</sup>*

Provides access to the list of networks in the container.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | List of all networks related to conainer |
| (property)[#] | string |  |

> The *name* shall be passed as the index to the property, e.g. *Containers.1.networks@ContainerName*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Container not found |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Containers.1.networks@ContainerName"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        "veth3NF06K"
    ]
}
```
<a name="property.ip"></a>
## *ip <sup>property</sup>*

Provides access to the list of all ip addresses of the container.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | List of all ip addresses of the container |
| (property)[#] | string |  |

> The *name* shall be passed as the index to the property, e.g. *Containers.1.ip@ContainerName*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Container not found |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Containers.1.ip@ContainerName"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        "192.168.0.123"
    ]
}
```
<a name="property.memory"></a>
## *memory <sup>property</sup>*

Provides access to the operating memory allocated to the container.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | Memory taken by container, in bytes |

> The *name* shall be passed as the index to the property, e.g. *Containers.1.memory@ContainerName*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Container not found |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Containers.1.memory@ContainerName"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": "{memory: 123456789; kmem: 112233445}"
}
```
<a name="property.status"></a>
## *status <sup>property</sup>*

Provides access to the operational status of the container.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | Status of the container (must be one of the following: *stopped*, *starting*, *running*, *aborting*, *stopping*) |

> The *name* shall be passed as the index to the property, e.g. *Containers.1.status@ContainerName*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Container not found |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Containers.1.status@ContainerName"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": "stopped"
}
```
<a name="property.cpu"></a>
## *cpu <sup>property</sup>*

Provides access to the CPU time allocated to running the container.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | CPU time, in nanoseconds |

> The *name* shall be passed as the index to the property, e.g. *Containers.1.cpu@ContainerName*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Container not found |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Containers.1.cpu@ContainerName"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": "{total:21873781248,thread1:284129812,thread2:2138921}"
}
```
<a name="property.log"></a>
## *log <sup>property</sup>*

Provides access to the containers log.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | logs from the container |

> The *name* shall be passed as the index to the property, e.g. *Containers.1.log@ContainerName*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Container not found |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Containers.1.log@ContainerName"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": ""
}
```
<a name="property.config"></a>
## *config <sup>property</sup>*

Provides access to the container's configuration.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | configuraiton of the container |

> The *name* shall be passed as the index to the property, e.g. *Containers.1.config@ContainerName*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Container not found |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Containers.1.config@ContainerName"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": ""
}
```
