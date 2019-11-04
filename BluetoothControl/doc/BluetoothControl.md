<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Bluetooth_Control_Plugin"></a>
# Bluetooth Control Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

BluetoothControl plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)
- [Properties](#head.Properties)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the BluetoothControl plugin. It includes detailed specification of its configuration, methods and properties provided, as well as notifications sent.

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

The Bluetooth Controll plugin allows enables Bluetooth administration functionality.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *BluetoothControl*) |
| classname | string | Class name: *BluetoothControl* |
| locator | string | Library name: *libWPEFrameworkBluetoothControl.so* |
| autostart | boolean | Determines if the plugin is to be started automatically along with the framework |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the BluetoothControl plugin:

BluetoothControl interface methods:

| Method | Description |
| :-------- | :-------- |
| [scan](#method.scan) | Starts scanning for Bluetooth devices |
| [connect](#method.connect) | Connects to a Bluetooth device |
| [disconnect](#method.disconnect) | Disconnects from a Bluetooth device |
| [pair](#method.pair) | Pairs a Bluetooth device |
| [unpair](#method.unpair) | Unpairs a Bluetooth device |

<a name="method.scan"></a>
## *scan <sup>method</sup>*

Starts scanning for Bluetooth devices.

Also see: [scancomplete](#event.scancomplete)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.type | string | Bluetooth device type (must be one of the following: *Classic*, *LowEnergy*) |
| params?.timeout | number | <sup>*(optional)*</sup> Duration of the scan (in seconds); default: 10s |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 1 | ```ERROR_GENERAL``` | Failed to scan |
| 12 | ```ERROR_INPROGRESS``` | Scan already in progress |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "BluetoothControl.1.scan", 
    "params": {
        "type": "LowEnergy", 
        "timeout": 10
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
<a name="method.connect"></a>
## *connect <sup>method</sup>*

Connects to a Bluetooth device.

Also see: [devicestatechange](#event.devicestatechange)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |
| 9 | ```ERROR_ALREADY_CONNECTED``` | Device already connected |
| 1 | ```ERROR_GENERAL``` | Failed to connect the device |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "BluetoothControl.1.connect", 
    "params": {
        "address": "81:6F:B0:91:9B:FE"
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
<a name="method.disconnect"></a>
## *disconnect <sup>method</sup>*

Disconnects from a Bluetooth device.

Also see: [devicestatechange](#event.devicestatechange)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |
| 36 | ```ERROR_ALREADY_RELEASED``` | Device not connected |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "BluetoothControl.1.disconnect", 
    "params": {
        "address": "81:6F:B0:91:9B:FE"
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
<a name="method.pair"></a>
## *pair <sup>method</sup>*

Pairs a Bluetooth device.

Also see: [devicestatechange](#event.devicestatechange)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |
| 9 | ```ERROR_ALREADY_CONNECTED``` | Device already paired |
| 1 | ```ERROR_GENERAL``` | Failed to pair the device |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "BluetoothControl.1.pair", 
    "params": {
        "address": "81:6F:B0:91:9B:FE"
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
<a name="method.unpair"></a>
## *unpair <sup>method</sup>*

Unpairs a Bluetooth device.

Also see: [devicestatechange](#event.devicestatechange)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |
| 36 | ```ERROR_ALREADY_RELEASED``` | Device not paired |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "BluetoothControl.1.unpair", 
    "params": {
        "address": "81:6F:B0:91:9B:FE"
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
<a name="head.Properties"></a>
# Properties

The following properties are provided by the BluetoothControl plugin:

BluetoothControl interface properties:

| Property | Description |
| :-------- | :-------- |
| [adapters](#property.adapters) <sup>RO</sup> | List of local Bluetooth adapters |
| [adapter](#property.adapter) <sup>RO</sup> | Local Bluetooth adapter information |
| [devices](#property.devices) <sup>RO</sup> | List of known remote Bluetooth devices |
| [device](#property.device) <sup>RO</sup> | Remote Bluetooth device information |

<a name="property.adapters"></a>
## *adapters <sup>property</sup>*

Provides access to the list of local Bluetooth adapters.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | List of local Bluetooth adapters |
| (property)[#] | number | Adapter ID |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "BluetoothControl.1.adapters"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        0
    ]
}
```
<a name="property.adapter"></a>
## *adapter <sup>property</sup>*

Provides access to the local Bluetooth adapter information.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Local Bluetooth adapter information |
| (property).interface | string | Ndapter interface name |
| (property).address | string | Bluetooth address |
| (property).version | number | Device version |
| (property)?.manufacturer | number | <sup>*(optional)*</sup> Device manfuacturer Company Identifer |
| (property)?.name | string | <sup>*(optional)*</sup> Device name |
| (property)?.shortname | string | <sup>*(optional)*</sup> Device short name |

> The *adapter id* shall be passed as the index to the property, e.g. *BluetoothControl.1.adapter@0*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "BluetoothControl.1.adapter@0"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": {
        "interface": "hci0", 
        "address": "81:6F:B0:91:9B:FE", 
        "version": 8, 
        "manufacturer": 0, 
        "name": "Thunder", 
        "shortname": "Thunder"
    }
}
```
<a name="property.devices"></a>
## *devices <sup>property</sup>*

Provides access to the list of known remote Bluetooth devices.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | array | List of known remote Bluetooth devices |
| (property)[#] | string | Bluetooth address |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "BluetoothControl.1.devices"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": [
        "81:6F:B0:91:9B:FE"
    ]
}
```
<a name="property.device"></a>
## *device <sup>property</sup>*

Provides access to the remote Bluetooth device information.

> This property is **read-only**.

### Value

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| (property) | object | Remote Bluetooth device information |
| (property).name | string | Name of the device |
| (property).type | string | Bluetooth device type (must be one of the following: *Classic*, *LowEnergy*) |
| (property).connected | boolean | Denotes if the device is currently connected to host |
| (property).paired | boolean | Denotes if the device is currently paired with host |

> The *device address* shall be passed as the index to the property, e.g. *BluetoothControl.1.device@81:6F:B0:91:9B:FE*.

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |

### Example

#### Get Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "BluetoothControl.1.device@81:6F:B0:91:9B:FE"
}
```
#### Get Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": {
        "name": "Acme Bluetooth Device", 
        "type": "LowEnergy", 
        "connected": true, 
        "paired": true
    }
}
```
<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the plugin, and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the BluetoothControl plugin:

BluetoothControl interface events:

| Event | Description |
| :-------- | :-------- |
| [scancomplete](#event.scancomplete) | Notifies about scan completion |
| [devicestatechange](#event.devicestatechange) | Notifies about device state change |

<a name="event.scancomplete"></a>
## *scancomplete <sup>event</sup>*

Notifies about scan completion.

### Description

Register to this event to be notified about device scan completion

### Parameters

This event carries no parameters.

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "client.events.1.scancomplete"
}
```
<a name="event.devicestatechange"></a>
## *devicestatechange <sup>event</sup>*

Notifies about device state change.

### Description

Register to this event to be notified about device state changes

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params.state | string | Device state (must be one of the following: *Paired*, *Unpaired*, *Connected*, *Disconnected*) |
| params?.disconnectreason | string | <sup>*(optional)*</sup> Disconnection reason in case of *Disconnected* event (must be one of the following: *ConnectionTimeout*, *AuthenticationFailure*, *RemoteLowOnResources*, *RemotePoweredOff*, *TerminatedByRemote*, *TerminatedByHost*) |

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "client.events.1.devicestatechange", 
    "params": {
        "address": "81:6F:B0:91:9B:FE", 
        "state": "Disconnected", 
        "disconnectreason": "ConnectionTimeout"
    }
}
```
