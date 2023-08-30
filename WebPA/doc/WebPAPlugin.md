<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.WebPA_Plugin"></a>
# WebPA Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

WebPA plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the WebPA plugin. It includes detailed specification about its configuration.

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

<a name="head.Description"></a>
# Description

The WebPA plugin provides web browsing functionality based on the WebPA engine.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *WebPA*) |
| classname | string | Class name: *WebPA* |
| locator | string | Library name: *libWPEFrameworkWebPA.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |
| configuration | object | <sup>*(optional)*</sup>  |
| configuration?.interface | string | <sup>*(optional)*</sup> Interface |
| configuration?.pingwaittime | string | <sup>*(optional)*</sup> Ping waittime timout |
| configuration?.webpaurl | string | <sup>*(optional)*</sup> WebPA url |
| configuration?.paroduslocalurl | string | <sup>*(optional)*</sup> Parodus local url |
| configuration?.partnerid | string | <sup>*(optional)*</sup> Partner id |
| configuration?.webpabackoffmax | string | <sup>*(optional)*</sup> WebPA backoff max |
| configuration?.sslcertpath | string | <sup>*(optional)*</sup> Path of SSL certification |
| configuration?.forceipv4 | string | <sup>*(optional)*</sup> Force IPv4 |
| configuration?.location | string | <sup>*(optional)*</sup> Location |

