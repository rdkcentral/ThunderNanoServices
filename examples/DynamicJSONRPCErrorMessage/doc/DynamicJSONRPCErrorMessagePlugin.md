<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_DynamicJSONRPCErrorMessage_Plugin"></a>
# DynamicJSONRPCErrorMessage Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

DynamicJSONRPCErrorMessage plugin for Thunder framework.

### Table of Contents

- [Introduction](#head_Introduction)
- [Configuration](#head_Configuration)
- [Interfaces](#head_Interfaces)
- [Methods](#head_Methods)

<a id="head_Introduction"></a>
# Introduction

<a id="head_Scope"></a>
## Scope

This document describes purpose and functionality of the DynamicJSONRPCErrorMessage plugin. It includes detailed specification about its configuration and methods provided.

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

<a id="head_Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *DynamicJSONRPCErrorMessage*) |
| classname | string | mandatory | Class name: *DynamicJSONRPCErrorMessage* |
| locator | string | mandatory | Library name: *libThunderDynamicJSONRPCErrorMessage.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- IMath ([IMath.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IMath.h)) (version 1.0.0) (compliant format)

<a id="head_Methods"></a>
# Methods

The following methods are provided by the DynamicJSONRPCErrorMessage plugin:

Math interface methods:

| Method | Description |
| :-------- | :-------- |
| [add](#method_add) | Perform addition on given inputs |
| [sub](#method_sub) | Perform subtraction on given inputs |

<a id="method_add"></a>
## *add [<sup>method</sup>](#head_Methods)*

Perform addition on given inputs.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.a | integer | mandatory | First input |
| params.b | integer | mandatory | Second input |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | integer | mandatory | *...* |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "DynamicJSONRPCErrorMessage.1.add",
  "params": {
    "a": 0,
    "b": 0
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": 0
}
```

<a id="method_sub"></a>
## *sub [<sup>method</sup>](#head_Methods)*

Perform subtraction on given inputs.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.a | integer | mandatory | First input |
| params.b | integer | mandatory | Second input |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | integer | mandatory | *...* |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "DynamicJSONRPCErrorMessage.1.sub",
  "params": {
    "a": 0,
    "b": 0
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": 0
}
```

