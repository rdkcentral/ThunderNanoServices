<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Device_Info_Plugin"></a>
# Device Info Plugin

**Version: 1.0**

DeviceInfo functionality for WPEFramework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the DeviceInfo plugin. It includes detailed specification of its configuration and methods provided.

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
| <a name="ref.WPEF">[WPEF](https://github.com/WebPlatformForEmbedded/WPEFramework/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | WPEFramework API Reference |

<a name="head.Description"></a>
# Description

The DeviceInfo plugin allows retrieving of various device-related information.

The plugin is designed to be loaded and executed within the WPEFramework. For more information on WPEFramework refer to [[WPEF](#ref.WPEF)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *DeviceInfo*) |
| classname | string | Class name: *DeviceInfo* |
| locator | string | Library name: *libWPEFrameworkDeviceInfo.so* |
| autostart | boolean | Determines if the plugin is to be started automatically along with the framework |

<a name="head.Methods"></a>
# Methods

The following API is provided by the plugin via JSON-RPC:

- [addresses](#method.addresses)
- [system](#method.system)
- [sockets](#method.sockets)

This API follows the JSON-RPC 2.0 specification. Refer to [[JSON-RPC](#ref.JSON-RPC)] for more information.


<a name="method.addresses"></a>
## *addresses*

Retrieves network interface addresses.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array |  |
| result[#] | object |  |
| result[#].name | string | Interface name |
| result[#].mac | string | Interface MAC address |
| result[#]?.ips | array | <sup>*(optional)*</sup>  |
| result[#]?.ips[#] | string | <sup>*(optional)*</sup> Interface IP address |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "DeviceInfo.1.addresses"
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        {
            "name": "lo", 
            "mac": "00:00:00:00:00", 
            "ips": [
                "127.0.0.1"
            ]
        }
    ]
}
```
<a name="method.system"></a>
## *system*

Retrieves system general information.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.version | string | Software version (in form "version#hashtag") |
| result.uptime | number | System uptime (in seconds) |
| result.totalram | number | Total installed system RAM memory (in bytes) |
| result.freeram | number | Free system RAM memory (in bytes) |
| result.devicename | string | Host name |
| result.cpuload | number | Current CPU load (percentage) |
| result.togalgpuram | number | Total GPU DRAM memory (in bytes) |
| result.freegpuram | number | Free GPU DRAM memory (in bytes) |
| result.serialnumber | string | Device serial number |
| result.deviceid | string | Device ID |
| result.time | string | Current system date and time |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "DeviceInfo.1.system"
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": {
        "version": "1.0#14452f612c3747645d54974255d11b8f3b4faa54", 
        "uptime": 120, 
        "totalram": 655757312, 
        "freeram": 563015680, 
        "devicename": "buildroot", 
        "cpuload": 2, 
        "togalgpuram": 381681664, 
        "freegpuram": 358612992, 
        "serialnumber": "WPEuCfrLF45", 
        "deviceid": "WPEuCfrLF45", 
        "time": "Mon, 11 Mar 2019 14:38:18"
    }
}
```
<a name="method.sockets"></a>
## *sockets*

Retrieves socket port information.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.runs | number | Number of runs |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "DeviceInfo.1.sockets"
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": {
        "runs": 1
    }
}
```
