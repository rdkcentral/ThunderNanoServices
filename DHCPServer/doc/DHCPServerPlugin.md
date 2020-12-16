<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.DHCP_Server_Plugin"></a>
# DHCP Server Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

DHCPServer plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)
- [Properties](#head.Properties)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the DHCPServer plugin. It includes detailed specification about its configuration, methods and properties provided.

<a name="head.Case_Sensitivity"></a>
## Case Sensitivity

All identifiers of the interfaces described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a name="head.Acronyms,_Abbreviations_and_Terms"></a>
## Acronyms, Abbreviations and Terms

The table below provides and overview of acronyms used in this document and their definitions.

| Acronym | Description |
| :-------- | :-------- |
| <a name="acronym.API">API</a> | Application Programming Interface |
| <a name="acronym.DHCP">DHCP</a> | Dynamic Host Configuration Protocol |
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
| <a name="ref.DHCP">[DHCP](https://tools.ietf.org/html/rfc2131)</a> | DHCP protocol specification (RFC2131) |
| <a name="ref.HTTP">[HTTP](http://www.w3.org/Protocols)</a> | HTTP specification |
| <a name="ref.JSON-RPC">[JSON-RPC](https://www.jsonrpc.org/specification)</a> | JSON-RPC 2.0 specification |
| <a name="ref.JSON">[JSON](http://www.json.org/)</a> | JSON specification |
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | Thunder API Reference |

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *DHCPServer*) |
| classname | string | Class name: *DHCPServer* |
| locator | string | Library name: *libWPEFrameworkDHCPServer.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |
| configuration | object | Server configuration |
| configuration.name | string | Name of the server |
| configuration.servers | array | List of configured DHCP servers |
| configuration.servers[#] | object | Configuration of a server |
| configuration.servers[#].interface | string | Name of the network interface to bind to |
| configuration.servers[#].poolstart | number | IP pool start number |
| configuration.servers[#].poolsize | number | IP pool size (in IP numbers) |
| configuration.servers[#]?.router | number | <sup>*(optional)*</sup> IP of router |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the DHCPServer plugin:

DHCPServer interface methods:

| Method | Description |
| :-------- | :-------- |
| [activate](#method.activate) | Activates a DHCP server |
| [deactivate](#method.deactivate) | Deactivates a DHCP server |


<a name="method.activate"></a>
## *activate <sup>method</sup>*

Activates a DHCP server.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.interface | string | Network interface name |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 1 | ```ERROR_GENERAL``` | Failed to activate server |
| 22 | ```ERROR_UNKNOWN_KEY``` | Invalid interface name given |
| 5 | ```ERROR_ILLEGAL_STATE``` | Server is already activated |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "DHCPServer.1.activate",
    "params": {
        "interface": "eth0"
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

<a name="method.deactivate"></a>
## *deactivate <sup>method</sup>*

Deactivates a DHCP server.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.interface | string | Network interface name |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 1 | ```ERROR_GENERAL``` | Failed to deactivate server |
| 22 | ```ERROR_UNKNOWN_KEY``` | Invalid interface name given |
| 5 | ```ERROR_ILLEGAL_STATE``` | Server is not activated |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "DHCPServer.1.deactivate",
    "params": {
        "interface": "eth0"
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

The following properties are provided by the DHCPServer plugin:

DHCPServer interface properties:

| Property | Description |
| :-------- | :-------- |
| [status](#property.status) <sup>RO</sup> | Server status |


<a name="property.status"></a>
## *status <sup>property</sup>*

Provides access to the server status.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | List of configured servers |
| (property)[#] | object |  |
| (property)[#].interface | string | Network interface name |
| (property)[#].active | boolean | Denotes if server is currently active |
| (property)[#]?.begin | string | <sup>*(optional)*</sup> IP address pool start |
| (property)[#]?.end | string | <sup>*(optional)*</sup> IP address pool end |
| (property)[#]?.router | string | <sup>*(optional)*</sup> Router IP address |
| (property)[#]?.leases | array | <sup>*(optional)*</sup> List of IP address leases |
| (property)[#]?.leases[#] | object | <sup>*(optional)*</sup> Lease description |
| (property)[#]?.leases[#].name | string | Client identifier (or client hardware address if identifier is absent) |
| (property)[#]?.leases[#].ip | string | Client IP address |
| (property)[#]?.leases[#]?.expires | string | <sup>*(optional)*</sup> Client IP expiration time (in ISO8601 format, empty: never expires) |

> The *server* shall be passed as the index to the property, e.g. *DHCPServer.1.status@eth0*. If omitted, status of all configured servers is returned.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Invalid server name given |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "DHCPServer.1.status@eth0"
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
            "active": true,
            "begin": "192.168.0.10",
            "end": "192.168.0.100",
            "router": "192.168.0.1",
            "leases": [
                {
                    "name": "00e04c326c56",
                    "ip": "192.168.0.10",
                    "expires": "2019-05-07T07:20:26Z"
                }
            ]
        }
    ]
}
```

