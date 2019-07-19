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

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the NetworkControl plugin. It includes detailed specification of its configuration and methods provided.

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
| [network](#method.network) | Retrieves the actual network information for targeted network interface, if network interface is not given, all network interfaces are returned |
| [reload](#method.reload) | Reload static and non-static network interface adapter |
| [request](#method.request) | Reload non-static network interface adapter |
| [assign](#method.assign) | Reload static network interface adapter |
| [up](#method.up) | Set up network interface adapter |
| [down](#method.down) | Set down network interface adapter |
| [flush](#method.flush) | Flush network interface adapter |

<a name="method.network"></a>
## *network <sup>method</sup>*

Retrieves the actual network information for targeted network interface, if network interface is not given, all network interfaces are returned.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.device | string | Network interface |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array |  |
| result[#] | object |  |
| result[#]?.interface | string | <sup>*(optional)*</sup> Interface name |
| result[#]?.mode | string | <sup>*(optional)*</sup> Mode type (must be one of the following: *Manual*, *Static*, *Dynamic*) |
| result[#]?.address | string | <sup>*(optional)*</sup> IP address |
| result[#]?.mask | number | <sup>*(optional)*</sup> Network inteface mask |
| result[#]?.gateway | string | <sup>*(optional)*</sup> Gateway address |
| result[#]?.broadcast | string | <sup>*(optional)*</sup> Broadcast IP |

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
    "method": "NetworkControl.1.network", 
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
<a name="method.reload"></a>
## *reload <sup>method</sup>*

Reload static and non-static network interface adapter.

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

Reload non-static network interface adapter.

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

Reload static network interface adapter.

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
<a name="method.up"></a>
## *up <sup>method</sup>*

Set up network interface adapter.

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
    "method": "NetworkControl.1.up", 
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
<a name="method.down"></a>
## *down <sup>method</sup>*

Set down network interface adapter.

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
    "method": "NetworkControl.1.down", 
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

Flush network interface adapter.

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
