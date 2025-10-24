<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_Script_Engine_Plugin"></a>
# Script Engine Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

ScriptEngine plugin for Thunder framework.

### Table of Contents

- [Introduction](#head_Introduction)
- [Configuration](#head_Configuration)
- [Interfaces](#head_Interfaces)
- [Properties](#head_Properties)
- [Notifications](#head_Notifications)

<a id="head_Introduction"></a>
# Introduction

<a id="head_Scope"></a>
## Scope

This document describes purpose and functionality of the ScriptEngine plugin. It includes detailed specification about its configuration, properties provided and notifications sent.

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

<a id="head_Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *ScriptEngine*) |
| classname | string | mandatory | Class name: *ScriptEngine* |
| locator | string | mandatory | Library name: *libScriptEngine.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- IScriptEngine ([IScriptEngine.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IScriptEngine.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

<a id="head_Properties"></a>
# Properties

The following properties are provided by the ScriptEngine plugin:

ScriptEngine interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [url](#property_url) | read/write | Script to be loaded into the engine and to be executed |

<a id="property_url"></a>
## *url [<sup>property</sup>](#head_Properties)*

Provides access to the script to be loaded into the engine and to be executed.

### Value

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | object | mandatory | Script to be loaded into the engine and to be executed |
| (property).value | string | mandatory | *...* |

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| (property) | string | mandatory | Loaded URL |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "ScriptEngine.1.url"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": "https://example.com"
}
```

#### Set Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "ScriptEngine.1.url",
  "params": {
    "value": "..."
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

The following events are provided by the ScriptEngine plugin:

ScriptEngine interface events:

| Notification | Description |
| :-------- | :-------- |
| [urlchanged](#notification_urlchanged) | Signal changes on the subscribed namespace |

<a id="notification_urlchanged"></a>
## *urlchanged [<sup>notification</sup>](#head_Notifications)*

Signal changes on the subscribed namespace..

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.url | string | mandatory | Loaded URL |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "ScriptEngine.1.register",
  "params": {
    "event": "urlchanged",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.urlchanged",
  "params": {
    "url": "https://example.com"
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.urlchanged``.

