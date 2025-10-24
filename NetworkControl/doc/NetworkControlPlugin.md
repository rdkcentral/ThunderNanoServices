<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_Network_Control_Plugin"></a>
# Network Control Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

NetworkControl plugin for Thunder framework.

### Table of Contents

- [Introduction](#head_Introduction)
- [Description](#head_Description)
- [Configuration](#head_Configuration)
- [Interfaces](#head_Interfaces)
- [Methods](#head_Methods)
- [Properties](#head_Properties)
- [Notifications](#head_Notifications)

<a id="head_Introduction"></a>
# Introduction

<a id="head_Scope"></a>
## Scope

This document describes purpose and functionality of the NetworkControl plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

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

The Network Control plugin provides functionality for network interface management.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a id="head_Configuration"></a>
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

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- INetworkControl ([INetworkControl.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/INetworkControl.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

<a id="head_Methods"></a>
# Methods

The following methods are provided by the NetworkControl plugin:

Built-in methods:

| Method | Description |
| :-------- | :-------- |
| [versions](#method_versions) | Retrieves a list of JSON-RPC interfaces offered by this service |
| [exists](#method_exists) | Checks if a JSON-RPC method or property exists |
| [register](#method_register) | Registers for an asynchronous JSON-RPC notification |
| [unregister](#method_unregister) | Unregisters from an asynchronous JSON-RPC notification |

NetworkControl interface methods:

| Method | Description |
| :-------- | :-------- |
| [flush](#method_flush) | Flush and reload requested interface |

<a id="method_versions"></a>
## *versions [<sup>method</sup>](#head_Methods)*

Retrieves a list of JSON-RPC interfaces offered by this service.

### Parameters

This method takes no parameters.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | A list ofsinterfaces with their version numbers<br>*Array length must be at most 255 elements.* |
| result[#] | object | mandatory | *...* |
| result[#].name | string | mandatory | Name of the interface |
| result[#].major | integer | mandatory | Major part of version number |
| result[#].minor | integer | mandatory | Minor part of version number |
| result[#].patch | integer | mandatory | Patch part of version version number |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "NetworkControl.1.versions"
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "name": "JController",
      "major": 1,
      "minor": 0,
      "patch": 0
    }
  ]
}
```

<a id="method_exists"></a>
## *exists [<sup>method</sup>](#head_Methods)*

Checks if a JSON-RPC method or property exists.

### Description

This method will return *True* for the following methods/properties: *interfaces, status, network, dns, up, versions, exists, register, unregister, flush*.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.method | string | mandatory | Name of the method or property to look up |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | boolean | mandatory | Denotes if the method exists or not |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "NetworkControl.1.exists",
  "params": {
    "method": "interfaces"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": false
}
```

<a id="method_register"></a>
## *register [<sup>method</sup>](#head_Methods)*

Registers for an asynchronous JSON-RPC notification.

### Description

This method supports the following event names: *[update](#notification_update)*.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.event | string | mandatory | Name of the notification to register for |
| params.id | string | mandatory | Client identifier |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_FAILED_REGISTERED``` | Failed to register for the notification (e.g. already registered) |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "NetworkControl.1.register",
  "params": {
    "event": "update",
    "id": "myapp"
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

<a id="method_unregister"></a>
## *unregister [<sup>method</sup>](#head_Methods)*

Unregisters from an asynchronous JSON-RPC notification.

### Description

This method supports the following event names: *[update](#notification_update)*.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.event | string | mandatory | Name of the notification to register for |
| params.id | string | mandatory | Client identifier |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_FAILED_UNREGISTERED``` | Failed to unregister from the notification (e.g. not yet registered) |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "NetworkControl.1.unregister",
  "params": {
    "event": "update",
    "id": "myapp"
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

<a id="method_flush"></a>
## *flush [<sup>method</sup>](#head_Methods)*

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

<a id="head_Properties"></a>
# Properties

The following properties are provided by the NetworkControl plugin:

NetworkControl interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [interfaces](#property_interfaces) | read-only | Currently available interfaces |
| [status](#property_status) | read-only | Status of requested interface |
| [network](#property_network) | read/write | Network info of requested interface |
| [dns](#property_dns) | read/write | DNS list |
| [up](#property_up) | read/write | Provides given requested interface is up or not |

<a id="property_interfaces"></a>
## *interfaces [<sup>property</sup>](#head_Properties)*

Provides access to the currently available interfaces.

> This property is **read-only**.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | Currently available interfaces |
| (property)[#] | string | mandatory | *...* |

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

<a id="property_status"></a>
## *status [<sup>property</sup>](#head_Properties)*

Provides access to the status of requested interface.

> This property is **read-only**.

> The *interface* parameter shall be passed as the index to the property, i.e. ``status@<interface>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| interface | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Status of requested interface (must be one of the following: *Available, Unavailable*) |

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
  "result": "Available"
}
```

<a id="property_network"></a>
## *network [<sup>property</sup>](#head_Properties)*

Provides access to the network info of requested interface.

> The *interface* parameter shall be passed as the index to the property, i.e. ``network@<interface>``.

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

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | Info about requested interface |
| (property)[#] | object | mandatory | *...* |
| (property)[#].address | string | mandatory | IP Address |
| (property)[#].defaultgateway | string | mandatory | Default Gateway |
| (property)[#].mask | integer | mandatory | Network mask |
| (property)[#].mode | string | mandatory | Mode of interface activation Dynamic or Static (must be one of the following: *Dynamic, Static*) |

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
      "mode": "Dynamic"
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
        "mode": "Dynamic"
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

<a id="property_dns"></a>
## *dns [<sup>property</sup>](#head_Properties)*

Provides access to the DNS list.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | DNS list |
| (property).value | array | mandatory | *...* |
| (property).value[#] | string | mandatory | *...* |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | List of DNS |
| (property)[#] | string | mandatory | *...* |

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

<a id="property_up"></a>
## *up [<sup>property</sup>](#head_Properties)*

Provides access to the provides given requested interface is up or not.

> The *interface* parameter shall be passed as the index to the property, i.e. ``up@<interface>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| interface | string | mandatory | *...* |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Provides given requested interface is up or not |
| (property).value | boolean | mandatory | *...* |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | boolean | mandatory | Up/Down requested interface |

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

<a id="head_Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the NetworkControl plugin:

NetworkControl interface events:

| Notification | Description |
| :-------- | :-------- |
| [update](#notification_update) | Signal interface update |

<a id="notification_update"></a>
## *update [<sup>notification</sup>](#head_Notifications)*

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

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.update``.

