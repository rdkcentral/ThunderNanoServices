<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Power_Plugin"></a>
# Power Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

Power plugin for Thunder framework.

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

This document describes purpose and functionality of the Power plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

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
| callsign | string | mandatory | Plugin instance name (default: *Power*) |
| classname | string | mandatory | Class name: *Power* |
| locator | string | mandatory | Library name: *libThunderPower.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| configuration | object | optional | *...* |
| configuration?.powerkey | integer | optional | Key associated as powerkey |
| configuration?.offmode | string | optional | Type of offmode |
| configuration?.control | boolean | optional | Enable control clients |
| configuration?.gpiopin | integer | optional | GGIO pin (Broadcom) |
| configuration?.gpiotype | sting | optional | GPIO type (Broadcom) |
| configuration?.statechange | integer | optional | Statechange (Broadcom) |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- IPower ([IPower.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IPower.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

<a name="head.Methods"></a>
# Methods

The following methods are provided by the Power plugin:

Power interface methods:

| Method | Description |
| :-------- | :-------- |
| [setstate](#method.setstate) | Set the power state |

<a name="method.setstate"></a>
## *setstate [<sup>method</sup>](#head.Methods)*

Set the power state.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.state | string | mandatory | The power state to set (must be one of the following: *ActiveStandby, Hibernate, On, PassiveStandby, PowerOff, SuspendToRam*) |
| params.waittime | integer | mandatory | The time to wait for the power state to be set in seconds |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_GENERAL``` | General failure |
| ```ERROR_DUPLICATE_KEY``` | Trying to set the same power mode |
| ```ERROR_ILLEGAL_STATE``` | Power state is not supported |
| ```ERROR_BAD_REQUEST``` | Invalid Power state or Bad JSON param data format |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Power.1.setstate",
  "params": {
    "state": "Hibernate",
    "waittime": 10
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

<a name="head.Properties"></a>
# Properties

The following properties are provided by the Power plugin:

Power interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [getstate](#property.getstate) | read-only | Get the current power state |

<a name="property.getstate"></a>
## *getstate [<sup>property</sup>](#head.Properties)*

Provides access to the get the current power state.

> This property is **read-only**.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | The current power state (must be one of the following: *ActiveStandby, Hibernate, On, PassiveStandby, PowerOff, SuspendToRam*) |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Power.1.getstate"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "PassiveStandby"
}
```

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the Power plugin:

Power interface events:

| Notification | Description |
| :-------- | :-------- |
| [statechange](#notification.statechange) | Signals a change in the power state |

<a name="notification.statechange"></a>
## *statechange [<sup>notification</sup>](#head.Notifications)*

Signals a change in the power state.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.origin | string | mandatory | The state the device is transitioning from (must be one of the following: *ActiveStandby, Hibernate, On, PassiveStandby, PowerOff, SuspendToRam*) |
| params.destination | string | mandatory | The state the device is transitioning to (must be one of the following: *ActiveStandby, Hibernate, On, PassiveStandby, PowerOff, SuspendToRam*) |
| params.phase | string | mandatory | The phase of the transition (must be one of the following: *After, Before*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "Power.1.register",
  "params": {
    "event": "statechange",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.statechange",
  "params": {
    "origin": "ActiveStandby",
    "destination": "SuspendToRAM",
    "phase": "After"
  }
}
```

