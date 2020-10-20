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
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | Thunder API Reference |

<a name="head.Description"></a>
# Description

The TestUtility plugin enables to execute embedded test commands on the platform.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *TestUtility*) |
| classname | string | Class name: *TestUtility* |
| locator | string | Library name: *libWPEFrameworkTestUtility.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the TestUtility plugin:

TestUtility interface methods:

| Method | Description |
| :-------- | :-------- |
| [runmemory](#method.runmemory) | Runs a memory test command |
| [runcrash](#method.runcrash) | Runs a crash test command |


<a name="method.runmemory"></a>
## *runmemory <sup>method</sup>*

Runs a memory test command.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.command | string | Test command name |
| params?.size | number | <sup>*(optional)*</sup> The amount of memory in KB for allocation (applicable for *Malloc* commands) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.allocated | number | Already allocated memory in KB |
| result.size | number | Current allocation in KB |
| result.resident | number | Resident memory in KB |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unknown category |
| 30 | ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
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
    "id": 1234567890,
    "result": {
        "allocated": 0,
        "size": 0,
        "resident": 0
    }
}
```

<a name="method.runcrash"></a>
## *runcrash <sup>method</sup>*

Runs a crash test command.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.command | string | Test command name |
| params?.delay | number | <sup>*(optional)*</sup> Delay (in seconds) before the crash attempt (applicable for *Crash* command) |
| params?.count | number | <sup>*(optional)*</sup> How many times a Crash command will be executed consecutively (applicable for *CrashNTimes* command) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unknown category |
| 30 | ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
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
    "id": 1234567890,
    "result": null
}
```

<a name="head.Properties"></a>
# Properties

The following properties are provided by the TestUtility plugin:

TestUtility interface properties:

| Property | Description |
| :-------- | :-------- |
| [commands](#property.commands) <sup>RO</sup> | List of test commands |
| [description](#property.description) <sup>RO</sup> | Description of a test command |
| [parameters](#property.parameters) <sup>RO</sup> | Parameters of a test command |
| [shutdowntimeout](#property.shutdowntimeout) <sup>WO</sup> | Timeout to be waited before deactivating the plugin |


<a name="property.commands"></a>
## *commands <sup>property</sup>*

Provides access to the list of test commands.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | List of test commands |
| (property)[#] | string | Available test commands |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "TestUtility.1.commands"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": [
        "Malloc"
    ]
}
```

<a name="property.description"></a>
## *description <sup>property</sup>*

Provides access to the description of a test command.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Description of a test command |
| (property).description | string | Test command description |

> The *command* shall be passed as the index to the property, e.g. *TestUtility.1.description@Malloc*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unknown category |
| 30 | ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "TestUtility.1.description@Malloc"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": {
        "description": "Allocates desired amount of memory (in KB) and holds it"
    }
}
```

<a name="property.parameters"></a>
## *parameters <sup>property</sup>*

Provides access to the parameters of a test command.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Parameters of a test command |
| (property)?.input | array | <sup>*(optional)*</sup>  |
| (property)?.input[#] | object | <sup>*(optional)*</sup>  |
| (property)?.input[#].name | string | Test command input parameter |
| (property)?.input[#].type | string | Test command input parameter type (must be one of the following: *Number*, *String*, *Boolean*, *Object*, *Symbol*) |
| (property)?.input[#].comment | string | Test command input parameter description |
| (property).output | object |  |
| (property).output.name | string | Test command output parameter |
| (property).output.type | string | Test command output parameter type (must be one of the following: *Number*, *String*, *Boolean*, *Object*, *Symbol*) |
| (property).output.comment | string | Test command output parameter description |

> The *command* shall be passed as the index to the property, e.g. *TestUtility.1.parameters@Malloc*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unknown category |
| 30 | ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "TestUtility.1.parameters@Malloc"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
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
## *shutdowntimeout <sup>property</sup>*

Provides access to the timeout to be waited before deactivating the plugin.

> This property is **write-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | number | Timeout in milli seconds |

### Example

#### Set Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "TestUtility.1.shutdowntimeout",
    "params": 5000
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

