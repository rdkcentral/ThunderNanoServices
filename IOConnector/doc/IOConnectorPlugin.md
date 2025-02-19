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
- [Interfaces](#head.Interfaces)
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
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20Thunder.docx)</a> | Thunder API Reference |

<a name="head.Description"></a>
# Description

The IO Connector plugin provides access to GPIO pins.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *IOConnector*) |
| classname | string | mandatory | Class name: *IOConnector* |
| locator | string | mandatory | Library name: *libWPEIOConnector.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| pins | array | mandatory | List of GPIO pins available on the system |
| pins[#] | object | mandatory | Pin properties |
| pins[#].id | integer | mandatory | Pin ID |
| pins[#].mode | string | mandatory | Pin mode (must be one of the following: *Active, Both, High, Inactive, Low, Output*) |
| pins[#]?.activelow | boolean | optional | Denotes if pin is active in low state (default: *false*) |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- [IOConnector.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/IOConnector.json) (version 1.0.0) (uncompliant-extended format)

<a name="head.Properties"></a>
# Properties

The following properties are provided by the IOConnector plugin:

IOConnector interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [pin](#property.pin) | read/write | GPIO pin value |

<a name="property.pin"></a>
## *pin [<sup>property</sup>](#head.Properties)*

Provides access to the GPIO pin value.

Also see: [activity](#event.activity)

> The *pin id* parameter shall be passed as the index to the property, e.g. ``IOConnector.1.pin@<pin-id>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| pin-id | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | integer | mandatory | Value of the pin |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | integer | mandatory | Value of the pin |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown pin ID given |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "IOConnector.1.pin@189"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": 1
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "IOConnector.1.pin@189",
  "params": 1
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

The following events are provided by the IOConnector plugin:

IOConnector interface events:

| Notification | Description |
| :-------- | :-------- |
| [activity](#notification.activity) | Notifies about GPIO pin activity |

<a name="notification.activity"></a>
## *activity [<sup>notification</sup>](#head.Notifications)*

Notifies about GPIO pin activity.

### Description

Register to this event to be notified about pin value changes

### Parameters

> The *pin ID* parameter shall be passed within the client ID during registration, e.g. *189.myid*

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.value | integer | mandatory | Value of the pin |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "IOConnector.1.register",
  "params": {
    "event": "activity",
    "id": "189.myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "189.myid.activity",
  "params": {
    "value": 1
  }
}
```

> The *pin ID* parameter is passed within the designator, e.g. *189.myid.activity*.

