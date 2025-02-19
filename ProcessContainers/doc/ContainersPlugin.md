<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Containers_Plugin"></a>
# Containers Plugin

**Version: 1.0**

**Status: :white_circle::white_circle::white_circle:**

Containers plugin for Thunder framework.

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

This document describes purpose and functionality of the Containers plugin. It includes detailed specification about its configuration, methods and properties provided.

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

The Containers plugin provides informations about process containers running on system.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *Containers*) |
| classname | string | mandatory | Class name: *Containers* |
| locator | string | mandatory | Library name: *libWPEContainers.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- [Containers.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/Containers.json) (version 1.0.0) (compliant format)

<a name="head.Methods"></a>
# Methods

The following methods are provided by the Containers plugin:

Containers interface methods:

| Method | Description |
| :-------- | :-------- |
| [start](#method.start) | Starts a new container |
| [stop](#method.stop) | Stops a container |

<a name="method.start"></a>
## *start [<sup>method</sup>](#head.Methods)*

Starts a new container.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params?.name | string | optional | Name of container |
| params?.command | string | optional | Command that will be started in the container |
| params?.parameters | array | optional | List of parameters supplied to command |
| params?.parameters[#] | string | optional | *...* |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Container not found |
| ```ERROR_GENERAL``` | Failed to start container |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Containers.1.start",
  "params": {
    "name": "ContainerName",
    "command": "lsof",
    "parameters": [
      "-i"
    ]
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

<a name="method.stop"></a>
## *stop [<sup>method</sup>](#head.Methods)*

Stops a container.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.name | string | mandatory | Name of container |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Container not found |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
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
  "id": 42,
  "result": null
}
```

<a name="head.Properties"></a>
# Properties

The following properties are provided by the Containers plugin:

Containers interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [containers](#property.containers) | read-only | List of active containers |
| [networks](#property.networks) | read-only | List of network interfaces of the container |
| [memory](#property.memory) | read-only | Memory taken by container |
| [cpu](#property.cpu) | read-only | CPU time |

<a name="property.containers"></a>
## *containers [<sup>property</sup>](#head.Properties)*

Provides access to the list of active containers.

> This property is **read-only**.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | List of names of all containers |
| result[#] | string | mandatory | *...* |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Containers.1.containers"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    "ContainerName"
  ]
}
```

<a name="property.networks"></a>
## *networks [<sup>property</sup>](#head.Properties)*

Provides access to the list of network interfaces of the container.

> This property is **read-only**.

> The *name* parameter shall be passed as the index to the property, e.g. ``Containers.1.networks@<name>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| name | string | mandatory | *...* |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | List of all network interfaces related to the container |
| result[#] | object | mandatory | Returns networks associated with the container |
| result[#]?.interface | string | optional | Interface name |
| result[#]?.ips | array | optional | List of ip addresses |
| result[#]?.ips[#] | string | optional | IP address |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Container not found |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Containers.1.networks@ContainerName"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "interface": "veth3NF06K",
      "ips": [
        "192.168.0.12"
      ]
    }
  ]
}
```

<a name="property.memory"></a>
## *memory [<sup>property</sup>](#head.Properties)*

Provides access to the memory taken by container.

> This property is **read-only**.

> The *name* parameter shall be passed as the index to the property, e.g. ``Containers.1.memory@<name>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| name | string | mandatory | *...* |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | Memory allocated by the container, in bytes |
| result?.allocated | integer | optional | Memory allocated by container |
| result?.resident | integer | optional | Resident memory of the container |
| result?.shared | integer | optional | Shared memory in the container |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Container not found |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Containers.1.memory@ContainerName"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "allocated": 0,
    "resident": 0,
    "shared": 0
  }
}
```

<a name="property.cpu"></a>
## *cpu [<sup>property</sup>](#head.Properties)*

Provides access to the CPU time.

> This property is **read-only**.

> The *name* parameter shall be passed as the index to the property, e.g. ``Containers.1.cpu@<name>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| name | string | mandatory | *...* |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | CPU time spent on running the container, in nanoseconds |
| result?.total | integer | optional | CPU-time spent on container, in nanoseconds |
| result?.cores | array | optional | Time spent on each cpu core, in nanoseconds |
| result?.cores[#] | integer | optional | *...* |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Container not found |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Containers.1.cpu@ContainerName"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "total": 2871287421,
    "cores": [
      2871287421
    ]
  }
}
```

