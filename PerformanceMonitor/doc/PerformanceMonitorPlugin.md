<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Performance_Monitor_Plugin"></a>
# Performance Monitor Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

PerformanceMonitor plugin for Thunder framework.

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

This document describes purpose and functionality of the PerformanceMonitor plugin. It includes detailed specification about its configuration, methods and properties provided.

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

Retrieve the performance measurement against given package size.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *PerformanceMonitor*) |
| classname | string | mandatory | Class name: *PerformanceMonitor* |
| locator | string | mandatory | Library name: *libThunderPerformanceMonitor.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- [PerformanceMonitor.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/PerformanceMonitor.json) (version 1.0.0) (uncompliant-collapsed format)

<a name="head.Methods"></a>
# Methods

The following methods are provided by the PerformanceMonitor plugin:

PerformanceMonitor interface methods:

| Method | Description |
| :-------- | :-------- |
| [clear](#method.clear) | Clear all performance data collected |
| [send](#method.send) | Interface to test send data |
| [receive](#method.receive) | Interface to test receive data |
| [exchange](#method.exchange) | Interface to test exchange data |

<a name="method.clear"></a>
## *clear [<sup>method</sup>](#head.Methods)*

Clear all performance data collected.

### Parameters

This method takes no parameters.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "PerformanceMonitor.1.clear"
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

<a name="method.send"></a>
## *send [<sup>method</sup>](#head.Methods)*

Interface to test send data.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params?.data | string | optional | Any string data upto the size specified in the length |
| params?.length | integer | optional | Size of the data |
| params?.duration | integer | optional | Duration of the measurements |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | integer | mandatory | Size of data received by the jsonrpc interface |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "PerformanceMonitor.1.send",
  "params": {
    "data": "abababababab",
    "length": 0,
    "duration": 0
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

<a name="method.receive"></a>
## *receive [<sup>method</sup>](#head.Methods)*

Interface to test receive data.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | integer | mandatory | Size of data to be provided by the jsonrpc interface |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | *...* |
| result?.data | string | optional | Any string data upto the size specified in the length |
| result?.length | integer | optional | Size of the data |
| result?.duration | integer | optional | Duration of the measurements |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "PerformanceMonitor.1.receive",
  "params": 0
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "data": "abababababab",
    "length": 0,
    "duration": 0
  }
}
```

<a name="method.exchange"></a>
## *exchange [<sup>method</sup>](#head.Methods)*

Interface to test exchange data.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params?.data | string | optional | Any string data upto the size specified in the length |
| params?.length | integer | optional | Size of the data |
| params?.duration | integer | optional | Duration of the measurements |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | *...* |
| result?.data | string | optional | Any string data upto the size specified in the length |
| result?.length | integer | optional | Size of the data |
| result?.duration | integer | optional | Duration of the measurements |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "PerformanceMonitor.1.exchange",
  "params": {
    "data": "abababababab",
    "length": 0,
    "duration": 0
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "data": "abababababab",
    "length": 0,
    "duration": 0
  }
}
```

<a name="head.Properties"></a>
# Properties

The following properties are provided by the PerformanceMonitor plugin:

PerformanceMonitor interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [measurement](#property.measurement) | read-only | Retrieve the performance measurement against given package size |

<a name="property.measurement"></a>
## *measurement [<sup>property</sup>](#head.Properties)*

Provides access to the retrieve the performance measurement against given package size. Measurements will be provided in milliseconds.

> This property is **read-only**.

> The *package size* parameter shall be passed as the index to the property, e.g. ``PerformanceMonitor.1.measurement@<package-size>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| package-size | string | mandatory | Size of package whose statistics info has to be retrieved |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Retrieve the performance measurement against given package size. Measurements will be provided in milliseconds |
| (property).serialization | object | mandatory | Time taken to complete serialization |
| (property).serialization?.minimum | integer | optional | Minimum value of measurements |
| (property).serialization?.maximum | integer | optional | Maximum value of measurements |
| (property).serialization?.average | integer | optional | Average value of measurements |
| (property).serialization?.count | integer | optional | How many times measurement has been collected |
| (property).deserialization | object | mandatory | Time taken to complete deserialization |
| (property).deserialization?.minimum | integer | optional | Minimum value of measurements |
| (property).deserialization?.maximum | integer | optional | Maximum value of measurements |
| (property).deserialization?.average | integer | optional | Average value of measurements |
| (property).deserialization?.count | integer | optional | How many times measurement has been collected |
| (property).execution | object | mandatory | Time taken to complete execution |
| (property).execution?.minimum | integer | optional | Minimum value of measurements |
| (property).execution?.maximum | integer | optional | Maximum value of measurements |
| (property).execution?.average | integer | optional | Average value of measurements |
| (property).execution?.count | integer | optional | How many times measurement has been collected |
| (property).threadpool | object | mandatory | Time taken to complete threadpool wait |
| (property).threadpool?.minimum | integer | optional | Minimum value of measurements |
| (property).threadpool?.maximum | integer | optional | Maximum value of measurements |
| (property).threadpool?.average | integer | optional | Average value of measurements |
| (property).threadpool?.count | integer | optional | How many times measurement has been collected |
| (property).communication | object | mandatory | Time taken to complete communication |
| (property).communication?.minimum | integer | optional | Minimum value of measurements |
| (property).communication?.maximum | integer | optional | Maximum value of measurements |
| (property).communication?.average | integer | optional | Average value of measurements |
| (property).communication?.count | integer | optional | How many times measurement has been collected |
| (property).total | object | mandatory | Time taken to complete whole jsonrpc process |
| (property).total?.minimum | integer | optional | Minimum value of measurements |
| (property).total?.maximum | integer | optional | Maximum value of measurements |
| (property).total?.average | integer | optional | Average value of measurements |
| (property).total?.count | integer | optional | How many times measurement has been collected |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "PerformanceMonitor.1.measurement@1000"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

