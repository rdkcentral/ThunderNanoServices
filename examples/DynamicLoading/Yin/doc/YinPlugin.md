<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Yin_Plugin"></a>
# Yin Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

Yin plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Configuration](#head.Configuration)
- [Interfaces](#head.Interfaces)
- [Methods](#head.Methods)
- [Properties](#head.Properties)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the Yin plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

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

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *Yin*) |
| classname | string | mandatory | Class name: *Yin* |
| locator | string | mandatory | Library name: *libThunderYin.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| configuration | object | mandatory | *...* |
| configuration?.yangcallsign | string | optional | Callsign of the Yang service (typically *Yang*) |
| configuration.etymology | string | mandatory | Describes the meaning of yin |
| configuration?.datafile | string | optional | Name of the data file |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- IYin ([IYin.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IYin.h)) (version 1.0.0) (compliant format)

<a name="head.Methods"></a>
# Methods

The following methods are provided by the Yin plugin:

Yin interface methods:

| Method | Description |
| :-------- | :-------- |
| [symbol](#method.symbol) | Draws a tai chi symbol on the console (art loaded from file in the data directory) |

<a name="method.symbol"></a>
## *symbol [<sup>method</sup>](#head.Methods)*

Draws a tai chi symbol on the console (art loaded from file in the data directory).

### Parameters

This method takes no parameters.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_NOT_SUPPORTED``` | Drawing of the symbol is not supported with current balance |
| ```ERROR_UNAVAILABLE``` | Data file is not available or unspecified |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Yin.1.symbol"
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

The following properties are provided by the Yin plugin:

Yin interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [etymology](#property.etymology) | read-only | The meaning of yin |
| [balance](#property.balance) | read/write | Percentage of yin in the "universal balance" of the system |

<a name="property.etymology"></a>
## *etymology [<sup>property</sup>](#head.Properties)*

Provides access to the the meaning of yin.

> This property is **read-only**.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory |  |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Yin.1.etymology"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "the bright side"
}
```

<a name="property.balance"></a>
## *balance [<sup>property</sup>](#head.Properties)*

Provides access to the percentage of yin in the "universal balance" of the system.

### Description

The Yang service is additionally required to change the yin/yang balance.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Percentage of yin in the "universal balance" of the system |
| (property).value | integer | mandatory |  |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | integer | mandatory | Percentage of yin in the "universal balance" of the system |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_BAD_REQUEST``` | Given percentage value is invalid |
| ```ERROR_UNAVAILABLE``` | Can't set yin because yang is not available |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Yin.1.balance"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": 0
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Yin.1.balance",
  "params": {
    "value": 50
  }
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

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the Yin plugin:

Yin interface events:

| Notification | Description |
| :-------- | :-------- |
| [balanceChanged](#notification.balanceChanged) | Notifies of yin percentage change |

<a name="notification.balanceChanged"></a>
## *balanceChanged [<sup>notification</sup>](#head.Notifications)*

Notifies of yin percentage change.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.percentage | integer | mandatory | New percentage of yin in the "universal balance" |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Yin.1.register",
  "params": {
    "event": "balanceChanged",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.balanceChanged",
  "params": {
    "percentage": 50
  }
}
```

