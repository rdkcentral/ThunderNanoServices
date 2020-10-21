<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.IO_Connector_Plugin"></a>
# IO Connector Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

IOConnector plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Properties](#head.Properties)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the IOConnector plugin. It includes detailed specification about its configuration, properties provided and notifications sent.

<a name="head.Case_Sensitivity"></a>
## Case Sensitivity

All identifiers of the interfaces described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a name="head.Acronyms,_Abbreviations_and_Terms"></a>
## Acronyms, Abbreviations and Terms

The table below provides and overview of acronyms used in this document and their definitions.

| Acronym | Description |
| :-------- | :-------- |
| <a name="acronym.API">API</a> | Application Programming Interface |
| <a name="acronym.GPIO">GPIO</a> | General-Purpose Input/Output |
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

The IO Connector plugin provides access to GPIO pins.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *IOConnector*) |
| classname | string | Class name: *IOConnector* |
| locator | string | Library name: *libWPEIOConnector.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |
| pins | array | List of GPIO pins available on the system |
| pins[#] | object | Pin properties |
| pins[#].id | number | Pin ID |
| pins[#].mode | string | Pin mode (must be one of the following: *Low*, *High*, *Both*, *Active*, *Inactive*, *Output*) |
| pins[#]?.activelow | boolean | <sup>*(optional)*</sup> Denotes if pin is active in low state (default: *false*) |

<a name="head.Properties"></a>
# Properties

The following properties are provided by the IOConnector plugin:

IOConnector interface properties:

| Property | Description |
| :-------- | :-------- |
| [pin](#property.pin) | GPIO pin value |


<a name="property.pin"></a>
## *pin <sup>property</sup>*

Provides access to the GPIO pin value.

Also see: [activity](#event.activity)

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | number | GPIO pin value |

> The *pin id* shall be passed as the index to the property, e.g. *IOConnector.1.pin@189*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown pin ID given |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "IOConnector.1.pin@189"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": 1
}
```

#### Set Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "IOConnector.1.pin@189",
    "params": 1
}
```

#### Set Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": "null"
}
```

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the implementation, and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the IOConnector plugin:

IOConnector interface events:

| Event | Description |
| :-------- | :-------- |
| [activity](#event.activity) | Notifies about GPIO pin activity |


<a name="event.activity"></a>
## *activity <sup>event</sup>*

Notifies about GPIO pin activity.

### Description

Register to this event to be notified about pin value changes

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.value | number | GPIO pin value |

> The *pin ID* shall be passed within the designator, e.g. *189.client.events.1*.

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "189.client.events.1.activity",
    "params": {
        "value": 1
    }
}
```

