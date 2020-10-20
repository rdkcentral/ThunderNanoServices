<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.DIAL_Server_Plugin"></a>
# DIAL Server Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::white_circle:**

DIALServer plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the DIALServer plugin. It includes detailed specification about its configuration and notifications sent.

<a name="head.Case_Sensitivity"></a>
## Case Sensitivity

All identifiers of the interfaces described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a name="head.Acronyms,_Abbreviations_and_Terms"></a>
## Acronyms, Abbreviations and Terms

The table below provides and overview of acronyms used in this document and their definitions.

| Acronym | Description |
| :-------- | :-------- |
| <a name="acronym.API">API</a> | Application Programming Interface |
| <a name="acronym.DIAL">DIAL</a> | Discovery and Launch |
| <a name="acronym.HTTP">HTTP</a> | Hypertext Transfer Protocol |
| <a name="acronym.JSON">JSON</a> | JavaScript Object Notation; a data interchange format |
| <a name="acronym.JSON-RPC">JSON-RPC</a> | A remote procedure call protocol encoded in JSON |
| <a name="acronym.REST">REST</a> | Representational State Transfer |
| <a name="acronym.SSDP">SSDP</a> | Simple Service Discovery Protocol |

The table below provides and overview of terms and abbreviations used in this document and their definitions.

| Term | Description |
| :-------- | :-------- |
| <a name="term.Controller">Controller</a> | An internal plugin that allows activating and deactivating of services/plugins configured for use in the framework. |
| <a name="term.callsign">callsign</a> | The name given to an instance of a plugin. One plugin can be instantiated multiple times, but each instance the instance name, callsign, must be unique. |

<a name="head.References"></a>
## References

| Ref ID | Description |
| :-------- | :-------- |
| <a name="ref.DIAL">[DIAL](http://http://www.dial-multiscreen.org)</a> | DIAL protocol specification |
| <a name="ref.HTTP">[HTTP](http://www.w3.org/Protocols)</a> | HTTP specification |
| <a name="ref.JSON-RPC">[JSON-RPC](https://www.jsonrpc.org/specification)</a> | JSON-RPC 2.0 specification |
| <a name="ref.JSON">[JSON](http://www.json.org/)</a> | JSON specification |
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | Thunder API Reference |

<a name="head.Description"></a>
# Description

The DIAL Server plugin implements the server side of the DIAL protocol, allowing second-screen devices discovering and launching applications on a first-screen device, utilizing SSDP protocol and RESTful API. For more invormation about the DIAL protocol please refer to [[DIAL](#ref.DIAL)].

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *DIALServer*) |
| classname | string | Class name: *DIALServer* |
| locator | string | Library name: *libWPEFrameworkDIALServer.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |
| configuration | object | Server configuration |
| configuration.name | string | User-friendly name of the device |
| configuration.model | string | Name of the device model |
| configuration.description | string | Short description of the device |
| configuration?.modelnumber | string | <sup>*(optional)*</sup> Device model number |
| configuration?.modelurl | string | <sup>*(optional)*</sup> URL to device model website |
| configuration.manufacturer | string | Name of the device manufacturer |
| configuration?.manufacturerurl | string | <sup>*(optional)*</sup> URL to manufacturer website |
| configuration?.serialnumber | string | <sup>*(optional)*</sup> Device serial number |
| configuration?.upc | string | <sup>*(optional)*</sup> Device UPC barcode number (Universal Product Code) |
| configuration?.interface | string | <sup>*(optional)*</sup> Server interface IP and port (default: SSDP multicast address and port) |
| configuration?.webserver | string | <sup>*(optional)*</sup> Callsign of a service implementing the web server functionality (default: *WebServer*) |
| configuration?.switchboard | string | <sup>*(optional)*</sup> Callsign of a service implementing the switchboard functionality (default: *SwitchBoard*). If defined and the service is available then start/stop requests will be relayed to the *SwitchBoard* rather than handled by the *Controller* directly. This is used only in non-passive mode |
| configuration.apps | array | List of supported applications |
| configuration.apps[#] | object | (an application definition) |
| configuration.apps[#].name | string | Name of the application |
| configuration.apps[#]?.handler | string | <sup>*(optional)*</sup> Name of the application handler. If not defined then *name* will be used instead |
| configuration.apps[#]?.callsign | string | <sup>*(optional)*</sup> Callsign of the service implementing the application. If defined and the service is available then the *Controller* will be used to unconditionally start/stop the application by activating/deactivating its service directly (active mode), or by the *SwitchBoard* if selected and available (switchboard mode). If not defined then these operations will be handed over to JavaScript, by sending a notification and using *handler* (or *name*) property to identify the application (passive mode) |
| configuration.apps[#]?.url | string | <sup>*(optional)*</sup> A URL related to the application |
| configuration.apps[#]?.allowstop | boolean | <sup>*(optional)*</sup> Denotes if the application can be stopped *(true)* or not *(false, default)* |

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the implementation, and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the DIALServer plugin:

DIALServer interface events:

| Event | Description |
| :-------- | :-------- |
| [start](#event.start) | Signals that application start was requested over DIAL *(passive mode only)* |
| [stop](#event.stop) | Signals that application stop was requested over DIAL *(passive mode only)* |
| [hide](#event.hide) | Signals that application hide was requested over DIAL *(passive mode only)* |


<a name="event.start"></a>
## *start <sup>event</sup>*

Signals that application start was requested over DIAL *(passive mode only)*.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.application | string | Application name |
| params?.parameters | string | <sup>*(optional)*</sup> Additional application-specific parameters |
| params?.payload | string | <sup>*(optional)*</sup> Additional application-specific payload |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.start",
    "params": {
        "application": "YouTube",
        "parameters": "watch?v=zpp045FBbQY",
        "payload": ""
    }
}
```

<a name="event.stop"></a>
## *stop <sup>event</sup>*

Signals that application stop was requested over DIAL *(passive mode only)*.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.application | string | Application name |
| params?.parameters | string | <sup>*(optional)*</sup> Additional application-specific parameters |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.stop",
    "params": {
        "application": "YouTube",
        "parameters": "watch?v=zpp045FBbQY"
    }
}
```

<a name="event.hide"></a>
## *hide <sup>event</sup>*

Signals that application hide was requested over DIAL *(passive mode only)*.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.application | string | Application name |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.hide",
    "params": {
        "application": "YouTube"
    }
}
```

