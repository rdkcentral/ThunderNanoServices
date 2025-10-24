<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_Test_Utility_Plugin"></a>
# Test Utility Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

TestUtility plugin for Thunder framework.

### Table of Contents

- [Introduction](#head_Introduction)
- [Description](#head_Description)
- [Configuration](#head_Configuration)
- [Interfaces](#head_Interfaces)
- [Methods](#head_Methods)
- [Properties](#head_Properties)

<a id="head_Introduction"></a>
# Introduction

<a id="head_Scope"></a>
## Scope

This document describes purpose and functionality of the TestUtility plugin. It includes detailed specification about its configuration, methods and properties provided.

<a id="head_Case_Sensitivity"></a>
## Case Sensitivity

All identifiers of the interfaces described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a id="head_Acronyms,_Abbreviations_and_Terms"></a>
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

<a id="head_References"></a>
## References

| Ref ID | Description |
| :-------- | :-------- |
| <a name="ref.HTTP">[HTTP](http://www.w3.org/Protocols)</a> | HTTP specification |
| <a name="ref.JSON-RPC">[JSON-RPC](https://www.jsonrpc.org/specification)</a> | JSON-RPC 2.0 specification |
| <a name="ref.JSON">[JSON](http://www.json.org/)</a> | JSON specification |
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20Thunder.docx)</a> | Thunder API Reference |

<a id="head_Description"></a>
# Description

The TestUtility plugin enables to execute embedded test commands on the platform.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a id="head_Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *TestUtility*) |
| classname | string | mandatory | Class name: *TestUtility* |
| locator | string | mandatory | Library name: *libThunderTestUtility.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- [TestUtility.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/TestUtility.json) (version 1.0.0) (uncompliant-extended format)

<a id="head_Methods"></a>
# Methods

The following methods are provided by the TestUtility plugin:

Built-in methods:

| Method | Description |
| :-------- | :-------- |
| [versions](#method_versions) | Retrieves a list of JSON-RPC interfaces offered by this service |
| [exists](#method_exists) | Checks if a JSON-RPC method or property exists |

TestUtility interface methods:

| Method | Description |
| :-------- | :-------- |
| [runmemory](#method_runmemory) | Runs a memory test command |
| [runcrash](#method_runcrash) | Runs a crash test command |

<a id="method_versions"></a>
## *versions [<sup>method</sup>](#head_Methods)*

Retrieves a list of JSON-RPC interfaces offered by this service.

### Parameters

This method takes no parameters.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | A list ofsinterfaces with their version numbers<br>*Array length must be at most 255 elements.* |
| result[#] | object | mandatory | *...* |
| result[#].name | string | mandatory | Name of the interface |
| result[#].major | integer | mandatory | Major part of version number |
| result[#].minor | integer | mandatory | Minor part of version number |
| result[#].patch | integer | mandatory | Patch part of version version number |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestUtility.1.versions"
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "name": "JMyInterface",
      "major": 1,
      "minor": 0,
      "patch": 0
    }
  ]
}
```

<a id="method_exists"></a>
## *exists [<sup>method</sup>](#head_Methods)*

Checks if a JSON-RPC method or property exists.

### Description

This method will return *True* for the following methods/properties: *commands, description, parameters, shutdowntimeout, versions, exists, runmemory, runcrash*.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.method | string | mandatory | Name of the method or property to look up |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | boolean | mandatory | Denotes if the method exists or not |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestUtility.1.exists",
  "params": {
    "method": "commands"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": false
}
```

<a id="method_runmemory"></a>
## *runmemory [<sup>method</sup>](#head_Methods)*

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

<a id="method_runcrash"></a>
## *runcrash [<sup>method</sup>](#head_Methods)*

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

<a id="head_Properties"></a>
# Properties

The following properties are provided by the TestUtility plugin:

TestUtility interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [commands](#property_commands) | read-only | List of test commands |
| [description](#property_description) | read-only | Description of a test command |
| [parameters](#property_parameters) | read-only | Parameters of a test command |
| [shutdowntimeout](#property_shutdowntimeout) | write-only | Timeout to be waited before deactivating the plugin |

<a id="property_commands"></a>
## *commands [<sup>property</sup>](#head_Properties)*

Provides access to the list of test commands.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | List of test commands |
| (property)[#] | string | mandatory | Available test commands |

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

<a id="property_description"></a>
## *description [<sup>property</sup>](#head_Properties)*

Provides access to the description of a test command.

> This property is **read-only**.

> The *command* parameter shall be passed as the index to the property, i.e. ``description@<command>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| command | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Description of a test command |
| (property).description | string | mandatory | Test command description |

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

<a id="property_parameters"></a>
## *parameters [<sup>property</sup>](#head_Properties)*

Provides access to the parameters of a test command.

> This property is **read-only**.

> The *command* parameter shall be passed as the index to the property, i.e. ``parameters@<command>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| command | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Parameters of a test command |
| (property)?.input | array | optional | Input parameter list |
| (property)?.input[#] | object | mandatory | *...* |
| (property)?.input[#].name | string | mandatory | Test command parameter |
| (property)?.input[#].type | string | mandatory | Test command parameter type (must be one of the following: *Boolean, Number, Object, String, Symbol*) |
| (property)?.input[#].comment | string | mandatory | Test command parameter description |
| (property).output | object | mandatory | Output parameter list |
| (property).output.name | string | mandatory | Test command parameter |
| (property).output.type | string | mandatory | Test command parameter type (must be one of the following: *Boolean, Number, Object, String, Symbol*) |
| (property).output.comment | string | mandatory | Test command parameter description |

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

<a id="property_shutdowntimeout"></a>
## *shutdowntimeout [<sup>property</sup>](#head_Properties)*

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

