<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_Input_Switch_Plugin"></a>
# Input Switch Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

InputSwitch plugin for Thunder framework.

### Table of Contents

- [Introduction](#head_Introduction)
- [Description](#head_Description)
- [Configuration](#head_Configuration)
- [Interfaces](#head_Interfaces)
- [Methods](#head_Methods)
- [Properties](#head_Properties)

<a id="head_Introduction"></a>
# Introduction

<a id="head_Scope"></a>
## Scope

This document describes purpose and functionality of the InputSwitch plugin. It includes detailed specification about its configuration, methods and properties provided.

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

The InputSwitch plugin allows switching between different input sources.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a id="head_Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *InputSwitch*) |
| classname | string | mandatory | Class name: *InputSwitch* |
| locator | string | mandatory | Library name: *libWPEInputSwitch.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- IInputSwitch ([IInputSwitch.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IInputSwitch.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

<a id="head_Methods"></a>
# Methods

The following methods are provided by the InputSwitch plugin:

InputSwitch interface methods:

| Method | Description |
| :-------- | :-------- |
| [select](#method_select) | Enable the given channel, disabling all other not immune channels |
| [channel](#method_channel) | Enable or Disable the throughput through the given channel |

<a id="method_select"></a>
## *select [<sup>method</sup>](#head_Methods)*

Enable the given channel, disabling all other not immune channels.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.name | string | mandatory | Callsign that is the owner of this channel |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Failed to find a channel with the given name |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "InputSwitch.1.select",
  "params": {
    "name": "WebKitBrowser"
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

<a id="method_channel"></a>
## *channel [<sup>method</sup>](#head_Methods)*

Enable or Disable the throughput through the given channel.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.name | string | mandatory | Callsign that is the owner of this channel |
| params.enabled | boolean | mandatory | Enable or disable the throughput of data through this channel |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Failed to find a channel with the given name |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "InputSwitch.1.channel",
  "params": {
    "name": "WebKitBrowser",
    "enabled": true
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

The following properties are provided by the InputSwitch plugin:

InputSwitch interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [status](#property_status) | read-only | Check the status of the requested channel |

<a id="property_status"></a>
## *status [<sup>property</sup>](#head_Properties)*

Provides access to the check the status of the requested channel.

> This property is **read-only**.

> The *name* parameter shall be passed as the index to the property, i.e. ``status@<name>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| name | string | mandatory | Server name, if omitted, status of all configured channels is returned |

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | array | mandatory | Check the status of the requested channel *(if only one element is present then the array will be omitted)* |
| (property)[#] | object | mandatory | *...* |
| (property)[#].name | string | mandatory | Callsign associated with this channel |
| (property)[#].enabled | boolean | mandatory | Is the channel enabled to receive info |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Could not find the designated channel |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "InputSwitch.1.status@WebKitBrowser"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "name": "WebKitBrowser",
      "enabled": true
    }
  ]
}
```

