<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.DHCP_Server_Plugin"></a>
# DHCP Server Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

DHCPServer plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Configuration](#head.Configuration)
- [Interfaces](#head.Interfaces)
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
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20Thunder.docx)</a> | Thunder API Reference |

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *DHCPServer*) |
| classname | string | mandatory | Class name: *DHCPServer* |
| locator | string | mandatory | Library name: *libThunderDHCPServer.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| configuration | object | mandatory | Server configuration |
| configuration.name | string | mandatory | Name of the server |
| configuration.servers | array | mandatory | List of configured DHCP servers |
| configuration.servers[#] | object | mandatory | Configuration of a server |
| configuration.servers[#].interface | string | mandatory | Name of the network interface to bind to |
| configuration.servers[#].poolstart | integer | mandatory | IP pool start number |
| configuration.servers[#].poolsize | integer | mandatory | IP pool size (in IP numbers) |
| configuration.servers[#]?.router | integer | optional | IP of router |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- [DHCPServer.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/DHCPServer.json) (version 1.0.0) (compliant format)

<a name="head.Methods"></a>
# Methods

The following methods are provided by the DHCPServer plugin:

DHCPServer interface methods:

| Method | Description |
| :-------- | :-------- |
| [activate](#method.activate) | Activates a DHCP server |
| [deactivate](#method.deactivate) | Deactivates a DHCP server |

<a name="method.activate"></a>
## *activate [<sup>method</sup>](#head.Methods)*

Activates a DHCP server.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.interface | string | mandatory | Network interface name |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_GENERAL``` | Failed to activate server |
| ```ERROR_UNKNOWN_KEY``` | Invalid interface name given |
| ```ERROR_ILLEGAL_STATE``` | Server is already activated |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
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
  "id": 42,
  "result": null
}
```

<a name="method.deactivate"></a>
## *deactivate [<sup>method</sup>](#head.Methods)*

Deactivates a DHCP server.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.interface | string | mandatory | Network interface name |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null (default: *None*) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_GENERAL``` | Failed to deactivate server |
| ```ERROR_UNKNOWN_KEY``` | Invalid interface name given |
| ```ERROR_ILLEGAL_STATE``` | Server is not activated |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
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
  "id": 42,
  "result": null
}
```

<a name="head.Properties"></a>
# Properties

The following properties are provided by the DHCPServer plugin:

DHCPServer interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [status](#property.status) | read-only | Server status |

<a name="property.status"></a>
## *status [<sup>property</sup>](#head.Properties)*

Provides access to the server status.

> This property is **read-only**.

> The *server* parameter shall be passed as the index to the property, e.g. ``DHCPServer.1.status@<server>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| server | string | mandatory | If omitted, status of all configured servers is returned |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | List of configured servers |
| result[#] | object | mandatory | *...* |
| result[#].interface | string | mandatory | Network interface name |
| result[#].active | boolean | mandatory | Denotes if server is currently active |
| result[#]?.begin | string | optional | IP address pool start |
| result[#]?.end | string | optional | IP address pool end |
| result[#]?.router | string | optional | Router IP address |
| result[#]?.leases | array | optional | List of IP address leases |
| result[#]?.leases[#] | object | optional | Lease description |
| result[#]?.leases[#].name | string | mandatory | Client identifier (or client hardware address if identifier is absent) |
| result[#]?.leases[#].ip | string | mandatory | Client IP address |
| result[#]?.leases[#]?.expires | string | optional | Client IP expiration time (in ISO8601 format, empty: never expires) |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Invalid server name given |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "DHCPServer.1.status@eth0"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
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

