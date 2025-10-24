<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_AVS_Plugin"></a>
# AVS Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

AVS plugin for Thunder framework.

### Table of Contents

- [Introduction](#head_Introduction)
- [Description](#head_Description)
- [Configuration](#head_Configuration)
- [Interfaces](#head_Interfaces)
- [Methods](#head_Methods)
- [Notifications](#head_Notifications)

<a id="head_Introduction"></a>
# Introduction

<a id="head_Scope"></a>
## Scope

This document describes purpose and functionality of the AVS plugin. It includes detailed specification about its configuration, methods provided and notifications sent.

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

The Alexa Voice Service Headless Client serves as a personal assistant.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a id="head_Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *AVS*) |
| classname | string | mandatory | Class name: *AVS* |
| locator | string | mandatory | Library name: *libThunderAVS.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| configuration | object | optional | *...* |
| configuration.alexaclientconfig | string | mandatory | The path to the AlexaClientSDKConfig.json (e.g /usr/share/Thunder/AVS/AlexaClientSDKConfig.json) |
| configuration?.smartscreenconfig | string | optional | The path to the SmartScreenSDKConfig.json (e.g /usr/share/Thunder/AVS/SmartScreenSDKConfig.json). This config will be used only when SmartScreen functionality is enabled |
| configuration?.kwdmodelspath | string | optional | Path to the Keyword Detection models (e.g /usr/share/Thunder/AVS/models). The path mus contain the localeToModels.json file |
| configuration?.loglevel | string | optional | Capitalized log level of the AVS components. Possible values: NONE, CRITICAL, ERROR, WARN, INFO. Debug log levels start from DEBUG0 up to DEBUG0 |
| configuration.audiosource | string | mandatory | The callsign of the plugin that provides the voice audio input or PORTAUDIO, when the portaudio library should be used. (e.g BluetoothRemoteControll, PORTAUDIO) |
| configuration?.enablesmartscreen | boolean | optional | Enable the SmartScreen support in the runtime. The SmartScreen functionality must be compiled in |
| configuration?.enablekwd | boolean | optional | Enable the Keyword Detection engine in the runtime. The KWD functionality must be compiled in |

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- IAVSController ([IAVSClient.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IAVSClient.h)) (version 1.0.0) (uncompliant-collapsed format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

<a id="head_Methods"></a>
# Methods

The following methods are provided by the AVS plugin:

AVSController interface methods:

| Method | Description |
| :-------- | :-------- |
| [mute](#method_mute) | Mutes the audio output of AVS |
| [record](#method_record) | Starts or stops the voice recording, skipping keyword detection |

<a id="method_mute"></a>
## *mute [<sup>method</sup>](#head_Methods)*

Mutes the audio output of AVS.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | boolean | mandatory | *...* |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_GENERAL``` | when there is a fatal error or authorisation is not possible |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "AVS.1.mute",
  "params": false
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

<a id="method_record"></a>
## *record [<sup>method</sup>](#head_Methods)*

Starts or stops the voice recording, skipping keyword detection.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | boolean | mandatory | *...* |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_GENERAL``` | when there is a fatal error or authorisation is not possible |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "AVS.1.record",
  "params": false
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

<a id="head_Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the AVS plugin:

AVSController interface events:

| Notification | Description |
| :-------- | :-------- |
| [dialoguestatechange](#notification_dialoguestatechange) | Notifies about dialogue state changes |

<a id="notification_dialoguestatechange"></a>
## *dialoguestatechange [<sup>notification</sup>](#head_Notifications)*

Notifies about dialogue state changes.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | string | mandatory | The new state (must be one of the following: *Expecting, Idle, Listening, Speaking, Thinking*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "AVS.1.register",
  "params": {
    "event": "dialoguestatechange",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.dialoguestatechange",
  "params": "SPEAKING"
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.dialoguestatechange``.

