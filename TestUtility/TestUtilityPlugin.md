<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Test_Utility_Plugin"></a>
# Test Utility Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

TestUtility plugin for WPEFramework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the TestUtility plugin. It includes detailed specification of its configuration and methods provided.

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
| <a name="ref.WPEF">[WPEF](https://github.com/WebPlatformForEmbedded/WPEFramework/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | WPEFramework API Reference |

<a name="head.Description"></a>
# Description

The TestUtility plugin enables to execute embedded test commands on the platform.

The plugin is designed to be loaded and executed within the WPEFramework. For more information on WPEFramework refer to [[WPEF](#ref.WPEF)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *TestUtility*) |
| classname | string | Class name: *TestUtility* |
| locator | string | Library name: *libWPEFrameworkTestUtility.so* |
| autostart | boolean | Determines if the plugin is to be started automatically along with the framework |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the TestUtility plugin:

TestUtility interface methods:

| Method | Description |
| :-------- | :-------- |
| [commands](#method.commands) | Retrieves the list of test commands |
| [description](#method.description) | Retrieves the description of selected test command |
| [parameters](#method.parameters) | Retrieves parameters of the selected test command |
| [runmemory](#method.runmemory) | Runs a memory test command |
| [runcrash](#method.runcrash) | Runs a crash test command |

<a name="method.commands"></a>
## *commands <sup>method</sup>*

Retrieves the list of test commands

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array |  |
| result[#] | string | Available test commands |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "TestUtility.1.commands"
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        "Malloc"
    ]
}
```
<a name="method.description"></a>
## *description <sup>method</sup>*

Retrieves the description of selected test command

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.command | string | The test command name |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.description | string | Test command description |

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
    "method": "TestUtility.1.description", 
    "params": {
        "command": "Malloc"
    }
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": {
        "description": "Allocates desired amount of memory (in KB) and holds it"
    }
}
```
<a name="method.parameters"></a>
## *parameters <sup>method</sup>*

Retrieves parameters of the selected test command

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.command | string | The test command name |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result?.input | array | <sup>*(optional)*</sup>  |
| result?.input[#] | object | <sup>*(optional)*</sup>  |
| result?.input[#].name | string | The test command input parameter |
| result?.input[#].type | string | The test command input parameter type (must be one of the following: *Number*, *String*, *Boolean*, *Object*, *Symbol*) |
| result?.input[#].comment | string | The test command input parameter description |
| result.output | object |  |
| result.output.name | string | The test command output parameter |
| result.output.type | string | The test command output parameter type |
| result.output.comment | string | The test command output parameter description |

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
    "method": "TestUtility.1.parameters", 
    "params": {
        "command": "Malloc"
    }
}
```
#### Response

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
<a name="method.runmemory"></a>
## *runmemory <sup>method</sup>*

Runs a memory test command

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.command | string | The test command name |
| params?.size | number | <sup>*(optional)*</sup> The amount of memory in KB for allocation (applicable for "Malloc" commands) |

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

Runs a crash test command

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.command | string | The test command name |
| params?.delay | number | <sup>*(optional)*</sup> Delay timeout |
| params?.count | number | <sup>*(optional)*</sup> How many times a Crash command will be executed consecutively (applicable for "CrashNTimes" command) |

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
