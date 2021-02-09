<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.ResourceMonitor_Plugin"></a>
# ResourceMonitor Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

ResourceMonitor plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the ResourceMonitor plugin. It includes detailed specification about its configuration.

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

The ResourceMonitor plugin provides web browsing functionality based on the ResourceMonitor engine.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *ResourceMonitor*) |
| classname | string | Class name: *ResourceMonitor* |
| locator | string | Library name: *libWPEFrameworkResourceMonitor.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |
| csv_filepath | string |<sup>*(optional)*</sup> Path where .csv file should be saved (default /tmp/resource.csv) |
| csv_separator | string |<sup>*(optional)*</sup> separator between columns of .csv (default ';' )|
| interval | number | <sup>*(optional)*</sup> Duration between measurements (default: 5) |
|name | string | <sup>*(optional)*</sup> Process to monitor (options: WPE - monitors each process begining with "WPE"; WPEFramework-1.0.0; WPEProcess; WPEWebProcess; WPENetworkProcess etc. (default: WPE)

# Methods

The following methods are provided by the ResourceMonitor plugin:

ResourceMonitor interface methods:

| Method | Description |
| :-------- | :-------- |
| [CompileMemoryCsv](#method.compilememorycsv) | Gets last 10 measurements|


<a name="method.compilememorycsv"></a>
## *CompileMemoryCsv <sup>method</sup>*

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result| string | Last 10 measurements with header|
