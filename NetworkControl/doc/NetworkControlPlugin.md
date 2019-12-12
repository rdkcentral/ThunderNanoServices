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
- [Methods](#head.Methods)
- [Properties](#head.Properties)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the NetworkControl plugin. It includes detailed specification of its configuration, methods and properties provided, as well as notifications sent.

<a name="head.Case_Sensitivity"></a>
## Case Sensitivity

All identifiers on the interface described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

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
| autostart | boolean | Determines if the plugin is to be started automatically along with the framework |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the NetworkControl plugin:

NetworkControl interface methods:

| Method | Description |
| :-------- | :-------- |
| [reload](#method.reload) | Reloads a static and non-static network interface adapter |
| [request](#method.request) | Reloads a non-static network interface adapter |
| [assign](#method.assign) | Reloads a static network interface adapter |
| [flush](#method.flush) | Flushes a network interface adapter |

<a name="method.reload"></a>
## *reload <sup>method</sup>*

Reloads a static and non-static network interface adapter.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.device | string | Network interface |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unavailable network interface |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "NetworkControl.1.reload", 
    "params": {
        "device": "eth0"
    }
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": null
}
```
<a name="method.request"></a>
## *request <sup>method</sup>*

Reloads a non-static network interface adapter.

Also see: [connectionchange](#event.connectionchange)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.device | string | Network interface |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unavaliable network interface |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "NetworkControl.1.request", 
    "params": {
        "device": "eth0"
    }
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": null
}
```
<a name="method.assign"></a>
## *assign <sup>method</sup>*

Reloads a static network interface adapter.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.device | string | Network interface |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unavaliable network interface |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "NetworkControl.1.assign", 
    "params": {
        "device": "eth0"
    }
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": null
}
```
<a name="method.flush"></a>
## *flush <sup>method</sup>*

Flushes a network interface adapter.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.device | string | Network interface |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unavaliable network interface |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "NetworkControl.1.flush", 
    "params": {
        "device": "eth0"
    }
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": null
}
```
<a name="head.Properties"></a>
# Properties

The following properties are provided by the NetworkControl plugin:

NetworkControl interface properties:

| Property | Description |
| :-------- | :-------- |
| [network](#property.network) <sup>RO</sup> | Current network information |
| [up](#property.up) | Interface up status |

<a name="property.network"></a>
## *network <sup>property</sup>*

Provides access to the current network information.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | Current network information |
| (property)[#] | object |  |
| (property)[#]?.interface | string | <sup>*(optional)*</sup> Interface name |
| (property)[#]?.mode | string | <sup>*(optional)*</sup> Mode (must be one of the following: *Manual*, *Static*, *Dynamic*) |
| (property)[#]?.address | string | <sup>*(optional)*</sup> IP address |
| (property)[#]?.mask | number | <sup>*(optional)*</sup> Network interface mask |
| (property)[#]?.gateway | string | <sup>*(optional)*</sup> Gateway address |
| (property)[#]?.broadcast | string | <sup>*(optional)*</sup> Broadcast IP |

> The *interface* shall be passed as the index to the property, e.g. *NetworkControl.1.network@eth0*. If network interface is not given, all network interfaces are returned.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unavailable network interface |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "NetworkControl.1.network@eth0"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        {
            "interface": "eth0", 
            "mode": "dynamic", 
            "address": "192.168.1.158", 
            "mask": 24, 
            "gateway": "192.168.1.1", 
            "broadcast": "192.168.1.255"
        }
    ]
}
```
<a name="property.up"></a>
## *up <sup>property</sup>*

Provides access to the interface up status.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | boolean | Interface up status |

> The *interface* shall be passed as the index to the property, e.g. *NetworkControl.1.up@eth0*.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | Unavailable network interface |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "NetworkControl.1.up@eth0"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": false
}
```
#### Set Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "NetworkControl.1.up@eth0", 
    "params": false
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

Notifications are autonomous events, triggered by the internals of the plugin, and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the NetworkControl plugin:

NetworkControl interface events:

| Event | Description |
| :-------- | :-------- |
| [connectionchange](#event.connectionchange) | Notifies about connection status (created, updated, removed, connected and connectionfailed) |

<a name="event.connectionchange"></a>
## *connectionchange <sup>event</sup>*

Notifies about connection status (created, updated, removed, connected and connectionfailed).

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.name | string | Name of network interface |
| params.address | string | IP Address of network interface, if it is connected |
| params.status | string | Status of the interface, update, connected or not (must be one of the following: *created*, *updated*, *removed*, *connected*, *connectionfailed*) |

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "client.events.1.connectionchange", 
    "params": {
        "name": "eth0", 
        "address": "192.168.1.10", 
        "status": "connected"
    }
}
```
