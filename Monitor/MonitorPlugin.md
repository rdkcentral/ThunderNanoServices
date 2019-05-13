<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Monitor_Plugin"></a>
# Monitor Plugin

**Version: 1.0**

Monitor functionality for WPEFramework.

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

This document describes purpose and functionality of the Monitor plugin. It includes detailed specification of its configuration, methods provided and notifications sent.

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

The Monitor plugin provides a watchdog-like functionality for framework processes.

The plugin is designed to be loaded and executed within the WPEFramework. For more information on WPEFramework refer to [[WPEF](#ref.WPEF)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *Monitor*) |
| classname | string | Class name: *Monitor* |
| locator | string | Library name: *libWPEFrameworkMonitor.so* |
| autostart | boolean | Determines if the plugin is to be started automatically along with the framework |

<a name="head.Methods"></a>
# Methods

The following API is provided by the plugin via JSON-RPC:

- [status](#method.status)
- [resetstats](#method.resetstats)
- [restartlimits](#method.restartlimits)

This API follows the JSON-RPC 2.0 specification. Refer to [[JSON-RPC](#ref.JSON-RPC)] for more information.


<a name="method.status"></a>
## *status*

Returns the memory and process statistics either for a single plugin or all plugins watched by the Monitor

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string | The callsing of a plugin to get measurements snapshot of, if set empty then all observed objects will be returned |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array |  |
| result[#] | object |  |
| result[#].measurements | object | Measurements for the plugin |
| result[#].measurements.resident | object | Resident memory measurement |
| result[#].measurements.resident.min | number | Minimal value measured |
| result[#].measurements.resident.max | number | Maximal value measured |
| result[#].measurements.resident.average | number | Average of all measurements |
| result[#].measurements.resident.last | number | Last measured value |
| result[#].measurements.allocated | object | Allocated memory measurement |
| result[#].measurements.allocated.min | number | Minimal value measured |
| result[#].measurements.allocated.max | number | Maximal value measured |
| result[#].measurements.allocated.average | number | Average of all measurements |
| result[#].measurements.allocated.last | number | Last measured value |
| result[#].measurements.shared | object | Shared memory measurement |
| result[#].measurements.shared.min | number | Minimal value measured |
| result[#].measurements.shared.max | number | Maximal value measured |
| result[#].measurements.shared.average | number | Average of all measurements |
| result[#].measurements.shared.last | number | Last measured value |
| result[#].measurements.process | object | Processes measurement |
| result[#].measurements.process.min | number | Minimal value measured |
| result[#].measurements.process.max | number | Maximal value measured |
| result[#].measurements.process.average | number | Average of all measurements |
| result[#].measurements.process.last | number | Last measured value |
| result[#].measurements.operational | boolean | Whether the plugin is up and running |
| result[#].measurements.count | number | Number of measurements |
| result[#].observable | string | A callsign of the watched plugin |
| result[#].memoryrestartsettings | object | Restart limits for memory consumption related failures applying to the plugin |
| result[#].memoryrestartsettings.limit | number | Maximum number or restarts to be attempted |
| result[#].memoryrestartsettings.windowseconds | number | Time period within which failures must happen for the limit to be considered crossed |
| result[#].operationalrestartsettings | object | Restart limits for stability failures applying to the plugin |
| result[#].operationalrestartsettings.limit | number | Maximum number or restarts to be attempted |
| result[#].operationalrestartsettings.windowseconds | number | Time period within which failures must happen for the limit to be considered crossed |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Monitor.1.status", 
    "params": {
        "callsign": ""
    }
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        {
            "measurements": {
                "resident": {
                    "min": 0, 
                    "max": 100, 
                    "average": 50, 
                    "last": 100
                }, 
                "allocated": {
                    "min": 0, 
                    "max": 100, 
                    "average": 50, 
                    "last": 100
                }, 
                "shared": {
                    "min": 0, 
                    "max": 100, 
                    "average": 50, 
                    "last": 100
                }, 
                "process": {
                    "min": 0, 
                    "max": 100, 
                    "average": 50, 
                    "last": 100
                }, 
                "operational": true, 
                "count": 100
            }, 
            "observable": "callsign", 
            "memoryrestartsettings": {
                "limit": 3, 
                "windowseconds": 60
            }, 
            "operationalrestartsettings": {
                "limit": 3, 
                "windowseconds": 60
            }
        }
    ]
}
```
<a name="method.resetstats"></a>
## *resetstats*

Resets memory and process statistics for a single plugin watched by the Monitor

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string | The callsign of a plugin to reset statistics of |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object | Measurements for the plugin before reset |
| result.measurements | object | Measurements for the plugin |
| result.measurements.resident | object | Resident memory measurement |
| result.measurements.resident.min | number | Minimal value measured |
| result.measurements.resident.max | number | Maximal value measured |
| result.measurements.resident.average | number | Average of all measurements |
| result.measurements.resident.last | number | Last measured value |
| result.measurements.allocated | object | Allocated memory measurement |
| result.measurements.allocated.min | number | Minimal value measured |
| result.measurements.allocated.max | number | Maximal value measured |
| result.measurements.allocated.average | number | Average of all measurements |
| result.measurements.allocated.last | number | Last measured value |
| result.measurements.shared | object | Shared memory measurement |
| result.measurements.shared.min | number | Minimal value measured |
| result.measurements.shared.max | number | Maximal value measured |
| result.measurements.shared.average | number | Average of all measurements |
| result.measurements.shared.last | number | Last measured value |
| result.measurements.process | object | Processes measurement |
| result.measurements.process.min | number | Minimal value measured |
| result.measurements.process.max | number | Maximal value measured |
| result.measurements.process.average | number | Average of all measurements |
| result.measurements.process.last | number | Last measured value |
| result.measurements.operational | boolean | Whether the plugin is up and running |
| result.measurements.count | number | Number of measurements |
| result.observable | string | A callsign of the watched plugin |
| result.memoryrestartsettings | object | Restart limits for memory consumption related failures applying to the plugin |
| result.memoryrestartsettings.limit | number | Maximum number or restarts to be attempted |
| result.memoryrestartsettings.windowseconds | number | Time period within which failures must happen for the limit to be considered crossed |
| result.operationalrestartsettings | object | Restart limits for stability failures applying to the plugin |
| result.operationalrestartsettings.limit | number | Maximum number or restarts to be attempted |
| result.operationalrestartsettings.windowseconds | number | Time period within which failures must happen for the limit to be considered crossed |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Monitor.1.resetstats", 
    "params": {
        "callsign": "WebServer"
    }
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": {
        "measurements": {
            "resident": {
                "min": 0, 
                "max": 100, 
                "average": 50, 
                "last": 100
            }, 
            "allocated": {
                "min": 0, 
                "max": 100, 
                "average": 50, 
                "last": 100
            }, 
            "shared": {
                "min": 0, 
                "max": 100, 
                "average": 50, 
                "last": 100
            }, 
            "process": {
                "min": 0, 
                "max": 100, 
                "average": 50, 
                "last": 100
            }, 
            "operational": true, 
            "count": 100
        }, 
        "observable": "callsign", 
        "memoryrestartsettings": {
            "limit": 3, 
            "windowseconds": 60
        }, 
        "operationalrestartsettings": {
            "limit": 3, 
            "windowseconds": 60
        }
    }
}
```
<a name="method.restartlimits"></a>
## *restartlimits*

Sets new restart limits for a plugin

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string | The callsign of a plugin to reset measurements snapshot for |
| params.operationalrestartsettings | object | Restart setting for memory consumption type of failures |
| params.operationalrestartsettings.limit | number | Maximum number or restarts to be attempted |
| params.operationalrestartsettings.windowseconds | number | Time period within which failures must happen for the limit to be considered crossed |
| params.memoryrestartsettings | object | Restart setting for stability type of failures |
| params.memoryrestartsettings.limit | number | Maximum number or restarts to be attempted |
| params.memoryrestartsettings.windowseconds | number | Time period within which failures must happen for the limit to be considered crossed |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "Monitor.1.restartlimits", 
    "params": {
        "callsign": "WebServer", 
        "operationalrestartsettings": {
            "limit": 3, 
            "windowseconds": 60
        }, 
        "memoryrestartsettings": {
            "limit": 3, 
            "windowseconds": 60
        }
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

Notifications are autonomous events, triggered by the internals of the plugin, and broadcasted via JSON-RPC to all registered observers. Refer to [[WPEF](#ref.WPEF)] for information on how to register for a notification.

The following notifications are provided by the plugin:

- [action](#event.action)

<a name="event.action"></a>
## *action*

Signals action taken by the monitor

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string | Callsign of the plugin the monitor acted upon |
| params.action | string | The action executed by the monitor on a plugin. One of: "Activate", "Deactivate", "StoppedRestarting" |
| params.reason | string | A message describing the reason the action was taken of |

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "client.events.1.action", 
    "params": {
        "callsign": "WebServer", 
        "action": "Deactivate", 
        "reason": "EXCEEDED_MEMORY"
    }
}
```
