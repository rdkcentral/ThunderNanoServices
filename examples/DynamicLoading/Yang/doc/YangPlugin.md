<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Yang_Plugin"></a>
# Yang Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

Yang plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Configuration](#head.Configuration)
- [Interfaces](#head.Interfaces)
- [Properties](#head.Properties)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the Yang plugin. It includes detailed specification about its configuration, properties provided and notifications sent.

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
| callsign | string | mandatory | Plugin instance name (default: *Yang*) |
| classname | string | mandatory | Class name: *Yang* |
| locator | string | mandatory | Library name: *libThunderYang.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| configuration | object | mandatory | *...* |
| configuration?.yangcallsign | string | optional | Callsign of the Yin service (typically *Yin*) |
| configuration.etymology | string | mandatory | Describes the meaning of yang |
| configuration?.color | string | optional | Default color of yang |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- IYang ([IYang.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IYang.h)) (version 1.0.0) (compliant format)

<a name="head.Properties"></a>
# Properties

The following properties are provided by the Yang plugin:

Yang interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [color](#property.color) | read/write | Color of yang |
| [etymology](#property.etymology) | read-only | Meaning of yang |
| [balance](#property.balance) | read/write | Percentage of yang in the "universal balance" of the system |

<a name="property.color"></a>
## *color [<sup>property</sup>](#head.Properties)*

Provides access to the color of yang.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Color of yang |
| (property).value | string | mandatory | *...* (must be one of the following: *BLACK, DARK_GRAY, GRAY*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | Color of yang (must be one of the following: *BLACK, DARK_GRAY, GRAY*) |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Yang.1.color"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "GRAY"
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Yang.1.color",
  "params": {
    "value": "GRAY"
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

<a name="property.etymology"></a>
## *etymology [<sup>property</sup>](#head.Properties)*

Provides access to the meaning of yang.

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
  "method": "Yang.1.etymology"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "the dark side"
}
```

<a name="property.balance"></a>
## *balance [<sup>property</sup>](#head.Properties)*

Provides access to the percentage of yang in the "universal balance" of the system.

### Description

The Yin service is additionally required to change the yin/yang balance.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Percentage of yang in the "universal balance" of the system |
| (property).value | integer | mandatory |  |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | integer | mandatory | Percentage of yang in the "universal balance" of the system |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_BAD_REQUEST``` | Given percentage value is invalid |
| ```ERROR_UNAVAILABLE``` | Can't set yang because yin is not available |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Yang.1.balance"
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
  "method": "Yang.1.balance",
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

The following events are provided by the Yang plugin:

Yang interface events:

| Notification | Description |
| :-------- | :-------- |
| [balanceChanged](#notification.balanceChanged) | Notifies of yang percentage change |

<a name="notification.balanceChanged"></a>
## *balanceChanged [<sup>notification</sup>](#head.Notifications)*

Notifies of yang percentage change.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.percentage | integer | mandatory | New percentage of yang in the "universal balance" |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Yang.1.register",
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

