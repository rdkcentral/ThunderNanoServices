<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.SsoWeather_Plugin"></a>
# SsoWeather Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::white_circle:**

SsoWeather plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Interfaces](#head.Interfaces)
- [Properties](#head.Properties)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the SsoWeather plugin. It includes detailed specification about its configuration, properties provided and notifications sent.

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

The SsoWeather plugin that allows you to lookup basic weather info.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *SsoWeather*) |
| classname | string | Class name: *SsoWeather* |
| locator | string | Library name: *libWPESsoWeather.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- ISsoWeather ([ISsoWeather.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/ISsoWeather.h)) (version 1.0.0) (compliant format)

<a name="head.Properties"></a>
# Properties

The following properties are provided by the SsoWeather plugin:

SsoWeather interface properties:

| Property | Description |
| :-------- | :-------- |
| [temperature](#property.temperature) | Check current temperature |
| [israining](#property.israining) | Check wether is already raining |

<a name="property.temperature"></a>
## *temperature [<sup>property</sup>](#head.Properties)*

Provides access to the check current temperature.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Check current temperature |
| (property).value | integer | value in Celsius degrees |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | integer |  |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_GENERAL``` | Failed to retrieve current temperature |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "SsoWeather.1.temperature"
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
  "method": "SsoWeather.1.temperature",
  "params": {
    "value": 0
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

<a name="property.israining"></a>
## *israining [<sup>property</sup>](#head.Properties)*

Provides access to the check wether is already raining.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Check wether is already raining |
| (property).value | boolean | state (true: raining, false: sunny) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | boolean |  |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_GENERAL``` | Failed to retrieve current temperature |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "SsoWeather.1.israining"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": false
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "SsoWeather.1.israining",
  "params": {
    "value": false
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

The following events are provided by the SsoWeather plugin:

SsoWeather interface events:

| Event | Description |
| :-------- | :-------- |
| [temperature](#event.temperature) | Signals temperature change |
| [israining](#event.israining) | Signals raining state change |

<a name="event.temperature"></a>
## *temperature [<sup>event</sup>](#head.Notifications)*

Signals temperature change.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.temperature | integer | New temperature in degrees Celsius |

### Example

```json
{
  "jsonrpc": "2.0",
  "method": "client.events.1.temperature",
  "params": {
    "temperature": 0
  }
}
```

<a name="event.israining"></a>
## *israining [<sup>event</sup>](#head.Notifications)*

Signals raining state change.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.raining | boolean | New raining state (true: raining, false: sunny) |

### Example

```json
{
  "jsonrpc": "2.0",
  "method": "client.events.1.israining",
  "params": {
    "raining": false
  }
}
```

