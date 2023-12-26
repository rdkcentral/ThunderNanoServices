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
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | Thunder API Reference |

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *Yang*) |
| classname | string | Class name: *Yang* |
| locator | string | Library name: *libWPEFrameworkYang.so* |
| startmode | string | Determines if the plugin shall be started automatically along with the framework |
| configuration | object |  |
| configuration?.yangcallsign | string | <sup>*(optional)*</sup> Callsign of the Yin service (typically *Yin*) |
| configuration.etymology | string | Describes the meaning of yang |
| configuration?.color | string | <sup>*(optional)*</sup> Default color of yang |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- Exchange::IYang ([IYang.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IYang.h)) (version 1.0.0) (compliant format)

<a name="head.Properties"></a>
# Properties

The following properties are provided by the Yang plugin:

Yang interface properties:

| Property | Description |
| :-------- | :-------- |
| [color](#property.color) | Color of yang |
| [etymology](#property.etymology) <sup>RO</sup> | Meaning of yang |
| [balance](#property.balance) | Percentage of yang in the "universal balance" of the system |


<a name="property.color"></a>
## *color [<sup>property</sup>](#head.Properties)*

Provides access to the color of yang.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Color of yang |
| (property).value | string |  (must be one of the following: *Gray*, *DarkGray*, *Black*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | string |  (must be one of the following: *Gray*, *DarkGray*, *Black*) |

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
    "result": "Gray"
}
```

#### Set Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "Yang.1.color",
    "params": {
        "value": "Gray"
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

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | string |  |

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

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Percentage of yang in the "universal balance" of the system |
| (property).value | integer |  |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | integer |  |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_BAD_REQUEST``` | Given percentage value is invalid |
|  | ```ERROR_UNAVAILABLE``` | Can't set yang because yin is not available |

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

Notifications are autonomous events, triggered by the internals of the implementation, and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the Yang plugin:

Yang interface events:

| Event | Description |
| :-------- | :-------- |
| [balancechanged](#event.balancechanged) | Notifies of yang percentage change |


<a name="event.balancechanged"></a>
## *balancechanged [<sup>event</sup>](#head.Notifications)*

Notifies of yang percentage change.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.percentage | integer | New percentage of yang in the "universal balance" |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.balancechanged",
    "params": {
        "percentage": 50
    }
}
```

