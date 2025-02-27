<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Test_Controller_Plugin"></a>
# Test Controller Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

TestController plugin for Thunder framework.

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

This document describes purpose and functionality of the TestController plugin. It includes detailed specification about its configuration, methods and properties provided.

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

The TestController plugin enables executing of embedded test cases on the platform.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *TestController*) |
| classname | string | mandatory | Class name: *TestController* |
| locator | string | mandatory | Library name: *libThunderTestController.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- [TestController.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/TestController.json) (version 1.0.0) (compliant format)

<a name="head.Methods"></a>
# Methods

The following methods are provided by the TestController plugin:

TestController interface methods:

| Method | Description |
| :-------- | :-------- |
| [run](#method.run) | Runs a single test or multiple tests |

<a name="method.run"></a>
## *run [<sup>method</sup>](#head.Methods)*

Runs a single test or multiple tests.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params?.category | string | optional | Test category name, if omitted: all tests are executed |
| params?.test | string | optional | Test name, if omitted: all tests of category are executed |
| params?.args | string | optional | The test arguments in JSON format |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | List of test results |
| result[#] | object | mandatory | *...* |
| result[#].test | string | mandatory | Test name |
| result[#].status | string | mandatory | Test status |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Unknown category/test |
| ```ERROR_BAD_REQUEST``` | Bad json param data format |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestController.1.run",
  "params": {
    "category": "JSONRPC",
    "test": "JSONRPCTest",
    "args": "{ }"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "test": "JSONRPCTest",
      "status": "Success"
    }
  ]
}
```

<a name="head.Properties"></a>
# Properties

The following properties are provided by the TestController plugin:

TestController interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [categories](#property.categories) | read-only | List of test categories |
| [tests](#property.tests) | read-only | List of tests for a category |
| [description](#property.description) | read-only | Description of a test |

<a name="property.categories"></a>
## *categories [<sup>property</sup>](#head.Properties)*

Provides access to the list of test categories.

> This property is **read-only**.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | List of test categories |
| result[#] | string | mandatory | Test category name |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestController.1.categories"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    "JSONRPC"
  ]
}
```

<a name="property.tests"></a>
## *tests [<sup>property</sup>](#head.Properties)*

Provides access to the list of tests for a category.

> This property is **read-only**.

> The *category* parameter shall be passed as the index to the property, e.g. ``TestController.1.tests@<category>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| category | string | mandatory | *...* |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | List of tests for a category |
| result[#] | string | mandatory | Test name |

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
  "method": "TestController.1.tests@JSONRPC"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    "JSONRPCTest"
  ]
}
```

<a name="property.description"></a>
## *description [<sup>property</sup>](#head.Properties)*

Provides access to the description of a test.

> This property is **read-only**.

> The *test* parameter shall be passed as the index to the property, e.g. ``TestController.1.description@<test>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| test | string | mandatory | *...* |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | Description of a test |
| result.description | string | mandatory | Test description |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Unknown category/test |
| ```ERROR_BAD_REQUEST``` | Bad JSON param data format |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestController.1.description@JSONRPC"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "description": "Tests JSONRPC functionality"
  }
}
```

