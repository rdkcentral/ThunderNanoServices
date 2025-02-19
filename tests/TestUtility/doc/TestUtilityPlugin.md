<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Test_Utility_Plugin"></a>
# Test Utility Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

TestUtility plugin for Thunder framework.

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

This document describes purpose and functionality of the TestUtility plugin. It includes detailed specification about its configuration, methods and properties provided.

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

The TestUtility plugin enables to execute embedded test commands on the platform.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *TestUtility*) |
| classname | string | mandatory | Class name: *TestUtility* |
| locator | string | mandatory | Library name: *libThunderTestUtility.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- [TestUtility.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/TestUtility.json) (version 1.0.0) (uncompliant-extended format)

<a name="head.Methods"></a>
# Methods

The following methods are provided by the TestUtility plugin:

TestUtility interface methods:

| Method | Description |
| :-------- | :-------- |
| [runmemory](#method.runmemory) | Runs a memory test command |
| [runcrash](#method.runcrash) | Runs a crash test command |

<a name="method.runmemory"></a>
## *runmemory [<sup>method</sup>](#head.Methods)*

Runs a memory test command.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.command | string | mandatory | Test command name |
| params?.size | integer | optional | The amount of memory in KB for allocation (applicable for *Malloc* commands) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | *...* |
| result.allocated | integer | mandatory | Already allocated memory in KB |
| result.size | integer | mandatory | Current allocation in KB |
| result.resident | integer | mandatory | Resident memory in KB |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Unknown category |
| ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestUtility.1.runmemory",
  "params": {
    "command": "Malloc",
    "size": 0
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "allocated": 0,
    "size": 0,
    "resident": 0
  }
}
```

<a name="method.runcrash"></a>
## *runcrash [<sup>method</sup>](#head.Methods)*

Runs a crash test command.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.command | string | mandatory | Test command name |
| params?.delay | integer | optional | Delay (in seconds) before the crash attempt (applicable for *Crash* command) |
| params?.count | integer | optional | How many times a Crash command will be executed consecutively (applicable for *CrashNTimes* command) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Unknown category |
| ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestUtility.1.runcrash",
  "params": {
    "command": "Crash",
    "delay": 1,
    "count": 1
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

The following properties are provided by the TestUtility plugin:

TestUtility interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [commands](#property.commands) | read-only | List of test commands |
| [description](#property.description) | read-only | Description of a test command |
| [parameters](#property.parameters) | read-only | Parameters of a test command |
| [shutdowntimeout](#property.shutdowntimeout) | write-only | Timeout to be waited before deactivating the plugin |

<a name="property.commands"></a>
## *commands [<sup>property</sup>](#head.Properties)*

Provides access to the list of test commands.

> This property is **read-only**.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | List of test commands |
| result[#] | string | mandatory | Available test commands |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestUtility.1.commands"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    "Malloc"
  ]
}
```

<a name="property.description"></a>
## *description [<sup>property</sup>](#head.Properties)*

Provides access to the description of a test command.

> This property is **read-only**.

> The *command* parameter shall be passed as the index to the property, e.g. ``TestUtility.1.description@<command>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| command | string | mandatory | *...* |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | Description of a test command |
| result.description | string | mandatory | Test command description |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Unknown category |
| ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestUtility.1.description@Malloc"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "description": "Allocates desired amount of memory (in KB) and holds it"
  }
}
```

<a name="property.parameters"></a>
## *parameters [<sup>property</sup>](#head.Properties)*

Provides access to the parameters of a test command.

> This property is **read-only**.

> The *command* parameter shall be passed as the index to the property, e.g. ``TestUtility.1.parameters@<command>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| command | string | mandatory | *...* |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | Parameters of a test command |
| result?.input | array | optional | Input parameter list |
| result?.input[#] | object | optional | *...* |
| result?.input[#].name | string | mandatory | Test command parameter |
| result?.input[#].type | string | mandatory | Test command parameter type (must be one of the following: *Boolean, Number, Object, String, Symbol*) |
| result?.input[#].comment | string | mandatory | Test command parameter description |
| result.output | object | mandatory | Output parameter list |
| result.output.name | string | mandatory | Test command parameter |
| result.output.type | string | mandatory | Test command parameter type (must be one of the following: *Boolean, Number, Object, String, Symbol*) |
| result.output.comment | string | mandatory | Test command parameter description |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Unknown category |
| ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestUtility.1.parameters@Malloc"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "input": [
      {
        "name": "memory",
        "type": "Number",
        "comment": "Memory statistics in KB"
      }
    ],
    "output": {
      "name": "memory",
      "type": "Number",
      "comment": "Memory statistics in KB"
    }
  }
}
```

<a name="property.shutdowntimeout"></a>
## *shutdowntimeout [<sup>property</sup>](#head.Properties)*

Provides access to the timeout to be waited before deactivating the plugin.

> This property is **write-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | integer | mandatory | Timeout in milli seconds |

### Example

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestUtility.1.shutdowntimeout",
  "params": 5000
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

