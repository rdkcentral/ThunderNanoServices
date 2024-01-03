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
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | Thunder API Reference |

<a name="head.Description"></a>
# Description

The Network Control plugin provides functionality for network interface management.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *NetworkControl*) |
| classname | string | Class name: *NetworkControl* |
| locator | string | Library name: *libWPEFrameworkNetworkControl.so* |
| startmode | string | Determines if the plugin shall be started automatically along with the framework |
| configuration | object | <sup>*(optional)*</sup>  |
| configuration?.dnsfile | string | <sup>*(optional)*</sup> Path to DNS resolve file (default: /etc/resolv.conf) |
| configuration?.response | number | <sup>*(optional)*</sup> Maximum response time out of the DHCP server |
| configuration?.retries | number | <sup>*(optional)*</sup> Maximum number of retries to the DHCP server |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- INetworkControl ([INetworkControl.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/INetworkControl.h)) (version 1.0.0) (compliant format)

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

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.interface | string |  |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

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

| Property | Description |
| :-------- | :-------- |
| [interfaces](#property.interfaces) <sup>RO</sup> | Currently available interfaces |
| [status](#property.status) <sup>RO</sup> | Status of requested interface |
| [network](#property.network) | Network info of requested interface |
| [dns](#property.dns) | DNS list |
| [up](#property.up) | Provides given requested interface is up or not |

<a name="property.interfaces"></a>
## *interfaces [<sup>property</sup>](#head.Properties)*

Provides access to the currently available interfaces.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | Currently available interfaces |
| result[#] | string |  |

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

### Value

> The *interface* argument shall be passed as the index to the property, e.g. *NetworkControl.1.status@xyz*.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | string | Status of requested interface (must be one of the following: *Unavailable*, *Available*) |

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

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Network info of requested interface |
| (property).value | array |  |
| (property).value[#] | object |  |
| (property).value[#].address | string |  |
| (property).value[#].defaultgateway | string |  |
| (property).value[#].mask | integer |  |
| (property).value[#].mode | string |  (must be one of the following: *Static*, *Dynamic*) |

> The *interface* argument shall be passed as the index to the property, e.g. *NetworkControl.1.network@xyz*.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array |  |
| result[#] | object |  |
| result[#].address | string |  |
| result[#].defaultgateway | string |  |
| result[#].mask | integer |  |
| result[#].mode | string |  (must be one of the following: *Static*, *Dynamic*) |

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

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | DNS list |
| (property).value | array |  |
| (property).value[#] | string |  |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array |  |
| result[#] | string |  |

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

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Provides given requested interface is up or not |
| (property).value | boolean |  |

> The *interface* argument shall be passed as the index to the property, e.g. *NetworkControl.1.up@xyz*.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | boolean |  |

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

| Event | Description |
| :-------- | :-------- |
| [update](#event.update) |  |

<a name="event.update"></a>
## *update [<sup>event</sup>](#head.Notifications)*

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.interfacename | string |  |

### Example

```json
{
  "jsonrpc": "2.0",
  "method": "client.events.1.update",
  "params": {
    "interfacename": "..."
  }
}
```

