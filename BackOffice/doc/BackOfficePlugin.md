<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.BackOffice_Plugin"></a>
# BackOffice Plugin

**Version: 1.0**

**Status: :white_circle::white_circle::white_circle:**

BackOffice plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the BackOffice plugin. It includes detailed specification about its configuration.

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
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20Thunder.docx)</a> | Thunder API Reference |

<a name="head.Description"></a>
# Description

The BackOffice plugin responsible for monitoring lifecycle of observables and passing this info to the server.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *BackOffice*) |
| classname | string | Class name: *BackOffice* |
| locator | string | Library name: *libThunderBackOffice.so* |
| startmode | string | Determines if the plugin shall be started automatically along with the framework |
| configuration | object |  |
| configuration.server_address | string | Back office server address |
| configuration.server_port | number | Back office server port |
| configuration.customer | string | Customer name |
| configuration.platform | string | Platform name |
| configuration.country | string | Country code |
| configuration?.type | string | <sup>*(optional)*</sup> Type |
| configuration?.session | number | <sup>*(optional)*</sup> session number |
| configuration.callsign_mapping | string | Mapping on how to map callsigns to server accepted names |
| configuration.state_mapping | string | Mapping on how to map state to server accepted states |

