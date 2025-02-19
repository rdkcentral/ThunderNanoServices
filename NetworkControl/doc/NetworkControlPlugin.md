<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Network_Control_Plugin"></a>
# Network Control Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

NetworkControl plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Interfaces](#head.Interfaces)
- [Methods](#head.Methods)
- [Properties](#head.Properties)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the NetworkControl plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

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

<a name="head.Description"></a>
# Description

The Network Control plugin provides functionality for network interface management.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *NetworkControl*) |
| classname | string | mandatory | Class name: *NetworkControl* |
| locator | string | mandatory | Library name: *libThunderNetworkControl.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| configuration | object | optional | *...* |
| configuration?.dnsfile | string | optional | Path to DNS resolve file (default: /etc/resolv.conf) |
| configuration?.response | integer | optional | Maximum response time out of the DHCP server |
| configuration?.retries | integer | optional | Maximum number of retries to the DHCP server |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- INetworkControl ([INetworkControl.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/INetworkControl.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

<a name="head.Methods"></a>
# Methods

The following methods are provided by the NetworkControl plugin:

NetworkControl interface methods:

| Method | Description |
| :-------- | :-------- |
| [flush](#method.flush) | Flush and reload requested interface |

<a name="method.flush"></a>
## *flush [<sup>method</sup>](#head.Methods)*

Flush and reload requested interface.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.interface | string | mandatory | Name of the interface to be flushed |

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
  "method": "NetworkControl.1.flush",
  "params": {
    "interface": "..."
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

The following properties are provided by the NetworkControl plugin:

NetworkControl interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [interfaces](#property.interfaces) | read-only | Currently available interfaces |
| [status](#property.status) | read-only | Status of requested interface |
| [network](#property.network) | read/write | Network info of requested interface |
| [dns](#property.dns) | read/write | DNS list |
| [up](#property.up) | read/write | Provides given requested interface is up or not |

<a name="property.interfaces"></a>
## *interfaces [<sup>property</sup>](#head.Properties)*

Provides access to the currently available interfaces.

> This property is **read-only**.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | Currently available interfaces |
| result[#] | string | mandatory | *...* |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "NetworkControl.1.interfaces"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    "..."
  ]
}
```

<a name="property.status"></a>
## *status [<sup>property</sup>](#head.Properties)*

Provides access to the status of requested interface.

> This property is **read-only**.

> The *interface* parameter shall be passed as the index to the property, e.g. ``NetworkControl.1.status@<interface>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| interface | string | mandatory | *...* |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | string | mandatory | Status of requested interface (must be one of the following: *Available, Unavailable*) |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "NetworkControl.1.status@xyz"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "Unavailable"
}
```

<a name="property.network"></a>
## *network [<sup>property</sup>](#head.Properties)*

Provides access to the network info of requested interface.

> The *interface* parameter shall be passed as the index to the property, e.g. ``NetworkControl.1.network@<interface>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| interface | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Network info of requested interface |
| (property).value | array | mandatory | *...* |
| (property).value[#] | object | mandatory | *...* |
| (property).value[#].address | string | mandatory | IP Address |
| (property).value[#].defaultgateway | string | mandatory | Default Gateway |
| (property).value[#].mask | integer | mandatory | Network mask |
| (property).value[#].mode | string | mandatory | Mode of interface activation Dynamic or Static (must be one of the following: *Dynamic, Static*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | Info about requested interface |
| result[#] | object | mandatory | *...* |
| result[#].address | string | mandatory | IP Address |
| result[#].defaultgateway | string | mandatory | Default Gateway |
| result[#].mask | integer | mandatory | Network mask |
| result[#].mode | string | mandatory | Mode of interface activation Dynamic or Static (must be one of the following: *Dynamic, Static*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Failed to set/retrieve network |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "NetworkControl.1.network@xyz"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "address": "...",
      "defaultgateway": "...",
      "mask": 0,
      "mode": "Static"
    }
  ]
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "NetworkControl.1.network@xyz",
  "params": {
    "value": [
      {
        "address": "...",
        "defaultgateway": "...",
        "mask": 0,
        "mode": "Static"
      }
    ]
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

<a name="property.dns"></a>
## *dns [<sup>property</sup>](#head.Properties)*

Provides access to the DNS list.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | DNS list |
| (property).value | array | mandatory | *...* |
| (property).value[#] | string | mandatory | *...* |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | List of DNS |
| result[#] | string | mandatory | *...* |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Failed to set/retrieve DNS |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "NetworkControl.1.dns"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    "..."
  ]
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "NetworkControl.1.dns",
  "params": {
    "value": [
      "..."
    ]
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

<a name="property.up"></a>
## *up [<sup>property</sup>](#head.Properties)*

Provides access to the provides given requested interface is up or not.

> The *interface* parameter shall be passed as the index to the property, e.g. ``NetworkControl.1.up@<interface>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| interface | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Provides given requested interface is up or not |
| (property).value | boolean | mandatory | *...* |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | boolean | mandatory | Up/Down requested interface |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | Failed to set/retrieve UP |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "NetworkControl.1.up@xyz"
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
  "method": "NetworkControl.1.up@xyz",
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

The following events are provided by the NetworkControl plugin:

NetworkControl interface events:

| Notification | Description |
| :-------- | :-------- |
| [update](#notification.update) | Signal interface update |

<a name="notification.update"></a>
## *update [<sup>notification</sup>](#head.Notifications)*

Signal interface update.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.interfacename | string | mandatory | Name of the interface where an update occured |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "NetworkControl.1.register",
  "params": {
    "event": "update",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.update",
  "params": {
    "interfacename": "..."
  }
}
```

