<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Power_Plugin"></a>
# Power Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

Power plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)
- [Properties](#head.Properties)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the Power plugin. It includes detailed specification about its configuration, methods and properties provided.

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

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *Power*) |
| classname | string | Class name: *Power* |
| locator | string | Library name: *libWPEFrameworkPower.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |
| configuration | object | <sup>*(optional)*</sup>  |
| configuration?.powerkey | number | <sup>*(optional)*</sup> Key associated as powerkey |
| configuration?.offmode | string | <sup>*(optional)*</sup> Type of offmode |
| configuration?.control | boolean | <sup>*(optional)*</sup> Enable control clients |
| configuration?.gpiopin | number | <sup>*(optional)*</sup> GGIO pin (Broadcom) |
| configuration?.gpiotype | sting | <sup>*(optional)*</sup> GPIO type (Broadcom) |
| configuration?.statechange | number | <sup>*(optional)*</sup> Statechange (Broadcom) |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the Power plugin:

Power interface methods:

| Method | Description |
| :-------- | :-------- |
| [set](#method.set) | Sets power state |


<a name="method.set"></a>
## *set <sup>method</sup>*

Sets power state.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.powerstate | string | Power state (must be one of the following: *on*, *activestandby*, *passivestandby*, *suspendtoram*, *hibernate*, *poweroff*) |
| params.timeout | number | Time to wait for power state change |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 1 | ```ERROR_GENERAL``` | General failure |
| 29 | ```ERROR_DUPLICATE_KEY``` | Trying to set the same power mode |
| 5 | ```ERROR_ILLEGAL_STATE``` | Power state is not supported |
| 30 | ```ERROR_BAD_REQUEST``` | Invalid Power state or Bad JSON param data format |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Power.1.set",
    "params": {
        "powerstate": "on",
        "timeout": 0
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

The following properties are provided by the Power plugin:

Power interface properties:

| Property | Description |
| :-------- | :-------- |
| [state](#property.state) <sup>RO</sup> | Power state |


<a name="property.state"></a>
## *state <sup>property</sup>*

Provides access to the power state.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | string | Power state (must be one of the following: *on*, *activestandby*, *passivestandby*, *suspendtoram*, *hibernate*, *poweroff*) |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "Power.1.state"
}
```

#### Get Response

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "result": "on"
}
```

