<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Bluetooth_SDP_Server_Plugin"></a>
# Bluetooth SDP Server Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

BluetoothSDPServer plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the BluetoothSDPServer plugin. It includes detailed specification about its configuration.

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
| <a name="acronym.SDP">SDP</a> | Service Discovery Protocol |

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
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20Thunder.docx)</a> | Thunder API Reference |

<a name="head.Description"></a>
# Description

The Bluetooth SDP Server runs a service discovery server for Bluetooth devices. The SDP server may be required for some audio source devices and is mandatory for discovery by nearby audio sink devices. .

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *BluetoothSDPServer*) |
| classname | string | mandatory | Class name: *BluetoothSDPServer* |
| locator | string | mandatory | Library name: *libThunderBluetoothSDPServer.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| configuration | object | mandatory | *...* |
| configuration.controller | string | mandatory | Callsign of the Bluetooth controller service (typically *BluetoothControl*) |
| configuration?.provider | string | optional | Name of the service provider (e.g. device manufacturer) |
| configuration?.services | array | optional | *...* |
| configuration?.services[#] | object | optional | Service settings |
| configuration?.services[#]?.class | string | optional | Class of the Bluetooth service (must be one of the following: *a2dp-audio-sink, a2dp-audio-source*) |
| configuration?.services[#].callsign | string | mandatory | Callsign of the service providing the functionality |
| configuration?.services[#]?.name | string | optional | Name of the service advertised over SDP |
| configuration?.services[#]?.description | string | optional | Description of the service advertised over SDP |
| configuration?.services[#]?.public | boolean | optional | Indicates if the service is listed in SDP PublicBrowseRoot group |
| configuration?.services[#]?.parameters | object | optional | Service-specific configuration parameters |

