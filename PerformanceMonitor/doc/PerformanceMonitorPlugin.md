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
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | Thunder API Reference |

<a name="head.Description"></a>
# Description

Retrieve the performance measurement against given package size.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *PerformanceMonitor*) |
| classname | string | Class name: *PerformanceMonitor* |
| locator | string | Library name: *libWPEFrameworkPerformanceMonitor.so* |
| startmode | string | Determines if the plugin shall be started automatically along with the framework |

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

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

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

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params?.data | string | <sup>*(optional)*</sup> Any string data upto the size specified in the length |
| params?.length | number | <sup>*(optional)*</sup>  |
| params?.duration | number | <sup>*(optional)*</sup>  |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | number | Size of data received by the jsonrpc interface |

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

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | number | Size of data to be provided by the jsonrpc interface |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result?.data | string | <sup>*(optional)*</sup> Any string data upto the size specified in the length |
| result?.length | number | <sup>*(optional)*</sup>  |
| result?.duration | number | <sup>*(optional)*</sup>  |

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

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params?.data | string | <sup>*(optional)*</sup> Any string data upto the size specified in the length |
| params?.length | number | <sup>*(optional)*</sup>  |
| params?.duration | number | <sup>*(optional)*</sup>  |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result?.data | string | <sup>*(optional)*</sup> Any string data upto the size specified in the length |
| result?.length | number | <sup>*(optional)*</sup>  |
| result?.duration | number | <sup>*(optional)*</sup>  |

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

| Property | Description |
| :-------- | :-------- |
| [measurement](#property.measurement) <sup>RO</sup> | Retrieve the performance measurement against given package size |

<a name="property.measurement"></a>
## *measurement [<sup>property</sup>](#head.Properties)*

Provides access to the retrieve the performance measurement against given package size. Measurements will be provided in milliseconds.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Retrieve the performance measurement against given package size. Measurements will be provided in milliseconds |
| (property).serialization | object | Time taken to complete serialization |
| (property).serialization?.minimum | number | <sup>*(optional)*</sup>  |
| (property).serialization?.maximum | number | <sup>*(optional)*</sup>  |
| (property).serialization?.average | number | <sup>*(optional)*</sup>  |
| (property).serialization?.count | number | <sup>*(optional)*</sup>  |
| (property).deserialization | object | Time taken to complete deserialization |
| (property).deserialization?.minimum | number | <sup>*(optional)*</sup>  |
| (property).deserialization?.maximum | number | <sup>*(optional)*</sup>  |
| (property).deserialization?.average | number | <sup>*(optional)*</sup>  |
| (property).deserialization?.count | number | <sup>*(optional)*</sup>  |
| (property).execution | object | Time taken to complete execution |
| (property).execution?.minimum | number | <sup>*(optional)*</sup>  |
| (property).execution?.maximum | number | <sup>*(optional)*</sup>  |
| (property).execution?.average | number | <sup>*(optional)*</sup>  |
| (property).execution?.count | number | <sup>*(optional)*</sup>  |
| (property).threadpool | object | Time taken to complete threadpool wait |
| (property).threadpool?.minimum | number | <sup>*(optional)*</sup>  |
| (property).threadpool?.maximum | number | <sup>*(optional)*</sup>  |
| (property).threadpool?.average | number | <sup>*(optional)*</sup>  |
| (property).threadpool?.count | number | <sup>*(optional)*</sup>  |
| (property).communication | object | Time taken to complete communication |
| (property).communication?.minimum | number | <sup>*(optional)*</sup>  |
| (property).communication?.maximum | number | <sup>*(optional)*</sup>  |
| (property).communication?.average | number | <sup>*(optional)*</sup>  |
| (property).communication?.count | number | <sup>*(optional)*</sup>  |
| (property).total | object | Time taken to complete whole jsonrpc process |
| (property).total?.minimum | number | <sup>*(optional)*</sup>  |
| (property).total?.maximum | number | <sup>*(optional)*</sup>  |
| (property).total?.average | number | <sup>*(optional)*</sup>  |
| (property).total?.count | number | <sup>*(optional)*</sup>  |

> The *package size* argument shall be passed as the index to the property, e.g. *PerformanceMonitor.1.measurement@1000*. Size of package whose statistics info has to be retrieved.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

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

