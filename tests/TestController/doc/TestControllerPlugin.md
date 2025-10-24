<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_Test_Controller_Plugin"></a>
# Test Controller Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

TestController plugin for Thunder framework.

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

This document describes purpose and functionality of the TestController plugin. It includes detailed specification about its configuration, methods and properties provided.

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

The TestController plugin enables executing of embedded test cases on the platform.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a id="head_Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *TestController*) |
| classname | string | mandatory | Class name: *TestController* |
| locator | string | mandatory | Library name: *libThunderTestController.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- [TestController.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/TestController.json) (version 1.0.0) (compliant format)

<a id="head_Methods"></a>
# Methods

The following methods are provided by the TestController plugin:

Built-in methods:

| Method | Description |
| :-------- | :-------- |
| [versions](#method_versions) | Retrieves a list of JSON-RPC interfaces offered by this service |
| [exists](#method_exists) | Checks if a JSON-RPC method or property exists |

TestController interface methods:

| Method | Description |
| :-------- | :-------- |
| [run](#method_run) | Runs a single test or multiple tests |

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
  "method": "TestController.1.versions"
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "name": "JController",
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

This method will return *True* for the following methods/properties: *categories, tests, description, versions, exists, run*.

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
  "method": "TestController.1.exists",
  "params": {
    "method": "categories"
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

<a id="method_run"></a>
## *run [<sup>method</sup>](#head_Methods)*

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

<a id="head_Properties"></a>
# Properties

The following properties are provided by the TestController plugin:

TestController interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [categories](#property_categories) | read-only | List of test categories |
| [tests](#property_tests) | read-only | List of tests for a category |
| [description](#property_description) | read-only | Description of a test |

<a id="property_categories"></a>
## *categories [<sup>property</sup>](#head_Properties)*

Provides access to the list of test categories.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | List of test categories |
| (property)[#] | string | mandatory | Test category name |

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

<a id="property_tests"></a>
## *tests [<sup>property</sup>](#head_Properties)*

Provides access to the list of tests for a category.

> This property is **read-only**.

> The *category* parameter shall be passed as the index to the property, i.e. ``tests@<category>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| category | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | List of tests for a category |
| (property)[#] | string | mandatory | Test name |

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

<a id="property_description"></a>
## *description [<sup>property</sup>](#head_Properties)*

Provides access to the description of a test.

> This property is **read-only**.

> The *test* parameter shall be passed as the index to the property, i.e. ``description@<test>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| test | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Description of a test |
| (property).description | string | mandatory | Test description |

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

