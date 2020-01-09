<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Volume_Control_Plugin"></a>
# Volume Control Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::white_circle:**

VolumeControl plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Properties](#head.Properties)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the VolumeControl plugin. It includes detailed specification of its configuration, properties provided and notifications sent.

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

The Volume Control plugin allows to manage system's audio volume

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *VolumeControl*) |
| classname | string | Class name: *VolumeControl* |
| locator | string | Library name: *libWPEVolumeControl.so* |
| autostart | boolean | Determines if the plugin is to be started automatically along with the framework |

<a name="head.Properties"></a>
# Properties

The following properties are provided by the VolumeControl plugin:

VolumeControl interface properties:

| Property | Description |
| :-------- | :-------- |
| [volume](#property.volume) | Volume in percents |
| [muted](#property.muted) | Drives whether audop should be muted |

<a name="property.volume"></a>
## *volume <sup>property</sup>*

Provides access to the volume in percents.

Also see: [volumechange](#event.volumechange)

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | number | Volume in percents |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "VolumeControl.1.volume"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": 50
}
```
#### Set Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "VolumeControl.1.volume", 
    "params": 50
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
<a name="property.muted"></a>
## *muted <sup>property</sup>*

Provides access to the drives whether audop should be muted.

Also see: [mutedchange](#event.mutedchange)

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | boolean | Drives whether audop should be muted |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "VolumeControl.1.muted"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": true
}
```
#### Set Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "VolumeControl.1.muted", 
    "params": true
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

The following events are provided by the VolumeControl plugin:

VolumeControl interface events:

| Event | Description |
| :-------- | :-------- |
| [volumechange](#event.volumechange) | Signals change of the volume |
| [mutedchange](#event.mutedchange) | Signals change of the muted property |

<a name="event.volumechange"></a>
## *volumechange <sup>event</sup>*

Signals change of the volume.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.volume | number | Current volume value |

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "client.events.1.volumechange", 
    "params": {
        "volume": 100
    }
}
```
<a name="event.mutedchange"></a>
## *mutedchange <sup>event</sup>*

Signals change of the muted property.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.muted | boolean | Muted state |

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "client.events.1.mutedchange", 
    "params": {
        "muted": true
    }
}
```
