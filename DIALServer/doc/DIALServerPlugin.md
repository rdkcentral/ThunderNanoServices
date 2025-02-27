<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.DIAL_Server_Plugin"></a>
# DIAL Server Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::white_circle:**

DIALServer plugin for Thunder framework.

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

This document describes purpose and functionality of the DIALServer plugin. It includes detailed specification about its configuration, properties provided and notifications sent.

<a name="head.Case_Sensitivity"></a>
## Case Sensitivity

All identifiers of the interfaces described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a name="head.Acronyms,_Abbreviations_and_Terms"></a>
## Acronyms, Abbreviations and Terms

The table below provides and overview of acronyms used in this document and their definitions.

| Acronym | Description |
| :-------- | :-------- |
| <a name="acronym.API">API</a> | Application Programming Interface |
| <a name="acronym.DIAL">DIAL</a> | Discovery and Launch |
| <a name="acronym.HTTP">HTTP</a> | Hypertext Transfer Protocol |
| <a name="acronym.JSON">JSON</a> | JavaScript Object Notation; a data interchange format |
| <a name="acronym.JSON-RPC">JSON-RPC</a> | A remote procedure call protocol encoded in JSON |
| <a name="acronym.REST">REST</a> | Representational State Transfer |
| <a name="acronym.SSDP">SSDP</a> | Simple Service Discovery Protocol |

The table below provides and overview of terms and abbreviations used in this document and their definitions.

| Term | Description |
| :-------- | :-------- |
| <a name="term.Controller">Controller</a> | An internal plugin that allows activating and deactivating of services/plugins configured for use in the framework. |
| <a name="term.callsign">callsign</a> | The name given to an instance of a plugin. One plugin can be instantiated multiple times, but each instance the instance name, callsign, must be unique. |

<a name="head.References"></a>
## References

| Ref ID | Description |
| :-------- | :-------- |
| <a name="ref.DIAL">[DIAL](http://http://www.dial-multiscreen.org)</a> | DIAL protocol specification |
| <a name="ref.HTTP">[HTTP](http://www.w3.org/Protocols)</a> | HTTP specification |
| <a name="ref.JSON-RPC">[JSON-RPC](https://www.jsonrpc.org/specification)</a> | JSON-RPC 2.0 specification |
| <a name="ref.JSON">[JSON](http://www.json.org/)</a> | JSON specification |
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20Thunder.docx)</a> | Thunder API Reference |

<a name="head.Description"></a>
# Description

The DIAL Server plugin implements the server side of the DIAL protocol, allowing second-screen devices discovering and launching applications on a first-screen device, utilizing SSDP protocol and RESTful API. For more invormation about the DIAL protocol please refer to [[DIAL](#ref.DIAL)].

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *DIALServer*) |
| classname | string | mandatory | Class name: *DIALServer* |
| locator | string | mandatory | Library name: *libThunderDIALServer.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| configuration | object | mandatory | Server configuration |
| configuration.name | string | mandatory | User-friendly name of the device |
| configuration.model | string | mandatory | Name of the device model |
| configuration.description | string | mandatory | Short description of the device |
| configuration?.modelnumber | string | optional | Device model number |
| configuration?.modelurl | string | optional | URL to device model website |
| configuration.manufacturer | string | mandatory | Name of the device manufacturer |
| configuration?.manufacturerurl | string | optional | URL to manufacturer website |
| configuration?.serialnumber | string | optional | Device serial number |
| configuration?.upc | string | optional | Device UPC barcode number (Universal Product Code) |
| configuration?.interface | string | optional | Server interface IP and port (default: SSDP multicast address and port) |
| configuration?.webserver | string | optional | Callsign of a service implementing the web server functionality (default: *WebServer*) |
| configuration?.switchboard | string | optional | Callsign of a service implementing the switchboard functionality (default: *SwitchBoard*). If defined and the service is available then start/stop requests will be relayed to the *SwitchBoard* rather than handled by the *Controller* directly. This is used only in non-passive mode |
| configuration.apps | array | mandatory | List of supported applications |
| configuration.apps[#] | object | mandatory | (an application definition) |
| configuration.apps[#].name | string | mandatory | Name of the application |
| configuration.apps[#]?.handler | string | optional | Name of the application handler. If not defined then *name* will be used instead |
| configuration.apps[#]?.callsign | string | optional | Callsign of the service implementing the application. If defined and the service is available then the *Controller* will be used to unconditionally start/stop the application by activating/deactivating its service directly (active mode), or by the *SwitchBoard* if selected and available (switchboard mode). If not defined then these operations will be handed over to JavaScript, by sending a notification and using *handler* (or *name*) property to identify the application (passive mode) |
| configuration.apps[#]?.url | string | optional | A URL related to the application |
| configuration.apps[#]?.allowstop | boolean | optional | Denotes if the application can be stopped *(true)* or not *(false, default)* |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- [DIALServer.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/DIALServer.json) (version 1.0.0) (uncompliant-extended format)

<a name="head.Properties"></a>
# Properties

The following properties are provided by the DIALServer plugin:

DIALServer interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [state](#property.state) | read/write | Current application running state |

<a name="property.state"></a>
## *state [<sup>property</sup>](#head.Properties)*

Provides access to the current application running state.

### Description

This property can be used to update the running status of an un-managed application (i.e. running in *passive mode*). For DIALServer-managed applications this property shall be considered *read-only*.

> The *application name* parameter shall be passed as the index to the property, e.g. ``DIALServer.1.state@<application-name>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| application-name | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Current application running state (must be one of the following: *Hidden, Started, Stopped*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | Current application running state (must be one of the following: *Hidden, Started, Stopped*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Specified application does not exist |
| ```ERROR_ILLEGAL_STATE``` | Specified application is running in *active mode* |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "DIALServer.1.state@YouTube"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "Stopped"
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "DIALServer.1.state@YouTube",
  "params": "Stopped"
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

The following events are provided by the DIALServer plugin:

DIALServer interface events:

| Notification | Description |
| :-------- | :-------- |
| [start](#notification.start) | Signals that application launch (or show if previously hidden) was requested over DIAL |
| [stop](#notification.stop) | Signals that application stop was requested over DIAL |
| [hide](#notification.hide) | Signals that application hide was requested over DIAL |
| [show](#notification.show) <sup>deprecated</sup> | Signals that application show was requested over DIAL |
| [change](#notification.change) <sup>deprecated</sup> | Signals that application URL change was requested over DIAL |

<a name="notification.start"></a>
## *start [<sup>notification</sup>](#head.Notifications)*

Signals that application launch (or show if previously hidden) was requested over DIAL.

### Description

This event is sent out only for un-managed applications (i.e. in *passive mode*).

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.application | string | mandatory | Application name |
| params?.parameters | string | optional | Additional application-specific parameters |
| params?.payload | string | optional | Additional application-specific payload |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "DIALServer.1.register",
  "params": {
    "event": "start",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.start",
  "params": {
    "application": "YouTube",
    "parameters": "watch?v=zpp045FBbQY",
    "payload": "..."
  }
}
```

<a name="notification.stop"></a>
## *stop [<sup>notification</sup>](#head.Notifications)*

Signals that application stop was requested over DIAL.

### Description

This event is sent out only for un-managed applications (i.e. in *passive mode*).

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.application | string | mandatory | Application name |
| params?.parameters | string | optional | Additional application-specific parameters |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "DIALServer.1.register",
  "params": {
    "event": "stop",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.stop",
  "params": {
    "application": "YouTube",
    "parameters": "watch?v=zpp045FBbQY"
  }
}
```

<a name="notification.hide"></a>
## *hide [<sup>notification</sup>](#head.Notifications)*

Signals that application hide was requested over DIAL.

### Description

This event is sent out only for un-managed applications (i.e. in *passive mode*).

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.application | string | mandatory | Application name |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "DIALServer.1.register",
  "params": {
    "event": "hide",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.hide",
  "params": {
    "application": "YouTube"
  }
}
```

<a name="notification.show"></a>
## *show [<sup>notification</sup>](#head.Notifications)*

Signals that application show was requested over DIAL.

> ``show`` is an alternative name for this notification. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Description

This event is sent out only for un-managed applications (i.e. in *passive mode*).

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.application | string | mandatory | Application name |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "DIALServer.1.register",
  "params": {
    "event": "show",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.show",
  "params": {
    "application": "YouTube"
  }
}
```

<a name="notification.change"></a>
## *change [<sup>notification</sup>](#head.Notifications)*

Signals that application URL change was requested over DIAL.

> ``change`` is an alternative name for this notification. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Description

This event is sent out only for un-managed applications (i.e. in *passive mode*).

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.application | string | mandatory | Application name |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "DIALServer.1.register",
  "params": {
    "event": "change",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.change",
  "params": {
    "application": "YouTube"
  }
}
```

