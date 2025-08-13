<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_Performance_Monitor_Plugin"></a>
# Performance Monitor Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

PerformanceMonitor plugin for Thunder framework.

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

This document describes purpose and functionality of the PerformanceMonitor plugin. It includes detailed specification about its configuration, methods and properties provided.

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

Retrieve the performance measurement against given package size.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a id="head_Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *PerformanceMonitor*) |
| classname | string | mandatory | Class name: *PerformanceMonitor* |
| locator | string | mandatory | Library name: *libThunderPerformanceMonitor.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- IPerformance ([IPerformance.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IPerformance.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

- IPerformanceStatistics ([IPerformance.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IPerformance.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

<a id="head_Methods"></a>
# Methods

The following methods are provided by the PerformanceMonitor plugin:

Performance interface methods:

| Method | Description |
| :-------- | :-------- |
| [send](#method_send) | Test the process of sending the data |
| [receive](#method_receive) | Test the process of receiving the data |
| [exchange](#method_exchange) | Test the process of both sending and receiving the data |

PerformanceStatistics interface methods:

| Method | Description |
| :-------- | :-------- |
| [reset](#method_reset) / [clear](#method_reset) | Clear all performance data collected so far |

<a id="method_send"></a>
## *send [<sup>method</sup>](#head_Methods)*

Test the process of sending the data.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.sendsize | integer | mandatory | Length of the sent data |
| params.buffer | string (base64) | mandatory | Any data to be sent |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "PerformanceMonitor.1.send",
  "params": {
    "sendsize": 15,
    "buffer": "HelloWorld"
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

<a id="method_receive"></a>
## *receive [<sup>method</sup>](#head_Methods)*

Test the process of receiving the data.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.buffersize | integer | mandatory | Size of data to be provided |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | *...* |
| result.buffersize | integer | mandatory | Size of data to be provided |
| result.buffer | string (base64) | mandatory | Data with a specified length |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "PerformanceMonitor.1.receive",
  "params": {
    "buffersize": 10
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "buffersize": 10,
    "buffer": "fhrjtus4p1"
  }
}
```

<a id="method_exchange"></a>
## *exchange [<sup>method</sup>](#head_Methods)*

Test the process of both sending and receiving the data.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.buffersize | integer | mandatory | Length of the data to be both sent as well as received |
| params.buffer | string (base64) | mandatory | Data to be sent and then also received |
| params.maxbuffersize | integer | mandatory | Maximum size of the buffer that can be received |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | *...* |
| result.buffersize | integer | mandatory | Length of the data to be both sent as well as received |
| result.buffer | string (base64) | mandatory | Data to be sent and then also received |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "PerformanceMonitor.1.exchange",
  "params": {
    "buffersize": 20,
    "buffer": "kjrpq018rt",
    "maxbuffersize": 100
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "buffersize": 20,
    "buffer": "kjrpq018rt"
  }
}
```

<a id="method_reset"></a>
## *reset [<sup>method</sup>](#head_Methods)*

Clear all performance data collected so far.

> ``clear`` is an alternative name for this method. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Parameters

This method takes no parameters.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "PerformanceMonitor.1.reset"
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

The following properties are provided by the PerformanceMonitor plugin:

PerformanceStatistics interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [measurement](#property_measurement) | read-only | Retrieve the performance measurement against a given package size |

<a id="property_measurement"></a>
## *measurement [<sup>property</sup>](#head_Properties)*

Provides access to the retrieve the performance measurement against a given package size. Measurements will be provided in milliseconds.

> This property is **read-only**.

> The *index* parameter shall be passed as the index to the property, i.e. ``measurement@<index>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| index | integer | mandatory | Size of package which statistics info is about to be retrieved |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Various performance measurements against a given package size |
| (property).serialization | object | mandatory | Time taken to complete serialization |
| (property).serialization.minimum | integer | mandatory | Minimum value of measurements |
| (property).serialization.maximum | integer | mandatory | Maximum value of measurements |
| (property).serialization.average | integer | mandatory | Average value of measurements |
| (property).serialization.count | integer | mandatory | How many times measurement has been collected |
| (property).deserialization | object | mandatory | Time taken to complete deserialization |
| (property).deserialization.minimum | integer | mandatory | Minimum value of measurements |
| (property).deserialization.maximum | integer | mandatory | Maximum value of measurements |
| (property).deserialization.average | integer | mandatory | Average value of measurements |
| (property).deserialization.count | integer | mandatory | How many times measurement has been collected |
| (property).execution | object | mandatory | Time taken to complete execution |
| (property).execution.minimum | integer | mandatory | Minimum value of measurements |
| (property).execution.maximum | integer | mandatory | Maximum value of measurements |
| (property).execution.average | integer | mandatory | Average value of measurements |
| (property).execution.count | integer | mandatory | How many times measurement has been collected |
| (property).threadpool | object | mandatory | Time taken to complete threadpool wait |
| (property).threadpool.minimum | integer | mandatory | Minimum value of measurements |
| (property).threadpool.maximum | integer | mandatory | Maximum value of measurements |
| (property).threadpool.average | integer | mandatory | Average value of measurements |
| (property).threadpool.count | integer | mandatory | How many times measurement has been collected |
| (property).communication | object | mandatory | Time taken to complete communication |
| (property).communication.minimum | integer | mandatory | Minimum value of measurements |
| (property).communication.maximum | integer | mandatory | Maximum value of measurements |
| (property).communication.average | integer | mandatory | Average value of measurements |
| (property).communication.count | integer | mandatory | How many times measurement has been collected |
| (property).total | object | mandatory | Time taken to complete whole JSON-RPC process |
| (property).total.minimum | integer | mandatory | Minimum value of measurements |
| (property).total.maximum | integer | mandatory | Maximum value of measurements |
| (property).total.average | integer | mandatory | Average value of measurements |
| (property).total.count | integer | mandatory | How many times measurement has been collected |

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
  "result": {
    "serialization": {
      "minimum": 1,
      "maximum": 4,
      "average": 2,
      "count": 5
    },
    "deserialization": {
      "minimum": 1,
      "maximum": 4,
      "average": 2,
      "count": 5
    },
    "execution": {
      "minimum": 1,
      "maximum": 4,
      "average": 2,
      "count": 5
    },
    "threadpool": {
      "minimum": 1,
      "maximum": 4,
      "average": 2,
      "count": 5
    },
    "communication": {
      "minimum": 1,
      "maximum": 4,
      "average": 2,
      "count": 5
    },
    "total": {
      "minimum": 1,
      "maximum": 4,
      "average": 2,
      "count": 5
    }
  }
}
```

