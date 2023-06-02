<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.The_Alexa_Voice_Service_Headless_Client"></a>
# The Alexa Voice Service Headless Client

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

AVS plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the AVS plugin. It includes detailed specification of its configuration, methods provided and notifications sent.

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

The Alexa Voice Service Headless Client serves as a personal assistant

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *AVS*) |
| classname | string | Class name: *AVS* |
| locator | string | Library name: *libWPEFrameworkAVS.so* |
| autostart | boolean | Determines if the plugin is to be started automatically along with the framework |
| configuration | object | <sup>*(optional)*</sup>  |
| configuration.alexaclientconfig | string | The path to the AlexaClientSDKConfig.json (e.g /usr/share/WPEFramework/AVS/AlexaClientSDKConfig.json) |
| configuration?.smartscreenconfig | string | <sup>*(optional)*</sup> The path to the SmartScreenSDKConfig.json (e.g /usr/share/WPEFramework/AVS/SmartScreenSDKConfig.json). This config will be used only when SmartScreen functionality is enabled |
| configuration?.kwdmodelspath | string | <sup>*(optional)*</sup> Path to the Keyword Detection models (e.g /usr/share/WPEFramework/AVS/models). The path mus contain the localeToModels.json file |
| configuration?.loglevel | string | <sup>*(optional)*</sup> Capitalized log level of the AVS components. Possible values: NONE, CRITICAL, ERROR, WARN, INFO. Debug log levels start from DEBUG0 up to DEBUG0 |
| configuration.audiosource | string | The callsign of the plugin that provides the voice audio input or PORTAUDIO, when the portaudio library should be used. (e.g BluetoothRemoteControll, PORTAUDIO) |
| configuration?.enablesmartscreen | boolean | <sup>*(optional)*</sup> Enable the SmartScreen support in the runtime. The SmartScreen functionality must be compiled in |
| configuration?.enablekwd | boolean | <sup>*(optional)*</sup> Enable the Keyword Detection engine in the runtime. The KWD functionality must be compiled in |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the AVS plugin:

AVSController interface methods:

| Method | Description |
| :-------- | :-------- |
| [Mute](#method.Mute) | Mutes both AVS_SPEAKER_VOLUME and AVS_ALERTS_VOLUME |
| [Record](#method.Record) | Starts or stops the voice recording, skipping keyword detection |

<a name="method.mute"></a>
## *mute <sup>method</sup>*

Mutes both AVS_SPEAKER_VOLUME and AVS_ALERTS_VOLUME.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.mute | boolean | Mute or umute |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_GENERAL``` | when there is a fatal error or authorisation is not possible |
|  | ```ERROR_UNAVAILABLE``` | when the AVSController is unavailable |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "AVS.1.mute",
    "params": {
        "mute": true
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
<a name="method.record"></a>
## *record <sup>method</sup>*

Starts or stops the voice recording, skipping keyword detection.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.start | boolean | Start or stop voice recording |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
|  | ```ERROR_GENERAL``` | when there is a fatal error or authorisation is not possible |
|  | ```ERROR_UNAVAILABLE``` | when the AVSController is unavailable |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "AVS.1.record",
    "params": {
        "start": true
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
<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the implementation, and broadcasted via JSON-RPC to all registered observers.Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the AVS plugin:

AVSController interface events:

| Event | Description |
| :-------- | :-------- |
| [DialogueStateChange](#event.DialogueStateChange) | notifies about dialogue state changes |

<a name="event.dialoguestatechange"></a>
## *dialoguestatechange <sup>event</sup>*

notifies about dialogue state changes.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.state | string | The new state (must be one of the following: *IDLE*, *LISTENING*, *EXPECTING*, *THINKING*, *SPEAKING*) |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.dialoguestatechange",
    "params": {
        "state": "SPEAKING"
    }
}
```
