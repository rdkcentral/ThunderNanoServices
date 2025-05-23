<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_Secure_Shell_Server"></a>
# Secure Shell Server

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

SecureShellServer plugin for Thunder framework.

### Table of Contents

- [Introduction](#head_Introduction)
- [Description](#head_Description)
- [Configuration](#head_Configuration)
- [Interfaces](#head_Interfaces)
- [Properties](#head_Properties)

<a id="head_Introduction"></a>
# Introduction

<a id="head_Scope"></a>
## Scope

This document describes purpose and functionality of the SecureShellServer plugin. It includes detailed specification about its configuration and properties provided.

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

Secure Shell Server plugins allows to get information about active SSH connections.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a id="head_Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *SecureShellServer*) |
| classname | string | mandatory | Class name: *SecureShellServer* |
| locator | string | mandatory | Library name: *libWPESecureShellServer.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- ISSHSessions ([ISecureShellServer.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/ISecureShellServer.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

<a id="head_Properties"></a>
# Properties

The following properties are provided by the SecureShellServer plugin:

SSHSessions interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [activesessionscount](#property_activesessionscount) / [getactivesessionscount](#property_activesessionscount) | read-only | Get count of currently active SSH client sessions maintained by SecureShell SSH Service |
| [activesessionsinfo](#property_activesessionsinfo) / [getactivesessionsinfo](#property_activesessionsinfo) | read-only | Get details of currently active SSH client sessions maintained by SecureShell SSH Service |
| [closesession](#property_closesession) / [closeclientsession](#property_closesession) | read-only | Close an active SSH client session |

<a id="property_activesessionscount"></a>
## *activesessionscount [<sup>property</sup>](#head_Properties)*

Provides access to the get count of currently active SSH client sessions maintained by SecureShell SSH Service.

> This property is **read-only**.

> ``getactivesessionscount`` is an alternative name for this property. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | integer | mandatory | Number of client sessions count |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "SecureShellServer.1.activesessionscount"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": 0
}
```

<a id="property_activesessionsinfo"></a>
## *activesessionsinfo [<sup>property</sup>](#head_Properties)*

Provides access to the get details of currently active SSH client sessions maintained by SecureShell SSH Service.

> This property is **read-only**.

> ``getactivesessionsinfo`` is an alternative name for this property. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | Get details of currently active SSH client sessions maintained by SecureShell SSH Service |
| (property)[#] | object | mandatory | *...* |
| (property)[#].pid | integer | mandatory | SSH client process ID |
| (property)[#].ipaddress | string | mandatory | SSH client connected from this IP address |
| (property)[#].timestamp | string | mandatory | SSH client connected at this timestamp |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_GENERAL``` | No active SSH client sessions |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "SecureShellServer.1.activesessionsinfo"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "pid": 268,
      "ipaddress": "192.168.33.57",
      "timestamp": "Sun Jun 30 21:49:08 2019"
    }
  ]
}
```

<a id="property_closesession"></a>
## *closesession [<sup>property</sup>](#head_Properties)*

Provides access to the close an active SSH client session.

> This property is **read-only**.

> ``closeclientsession`` is an alternative name for this property. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | integer | mandatory | SSH client process id |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "SecureShellServer.1.closesession"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": 0
}
```

