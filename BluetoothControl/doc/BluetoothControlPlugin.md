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

This document describes purpose and functionality of the BluetoothControl plugin. It includes detailed specification about its configuration, methods and properties provided, as well as notifications sent.

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

The Bluetooth Control plugin allows Bluetooth device administration.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *BluetoothControl*) |
| classname | string | Class name: *BluetoothControl* |
| locator | string | Library name: *libWPEFrameworkBluetoothControl.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |
| configuration | object | <sup>*(optional)*</sup>  |
| configuration?.interface | number | <sup>*(optional)*</sup> ID of interface |
| configuration?.name | String | <sup>*(optional)*</sup> Name of interface |
| configuration?.class | number | <sup>*(optional)*</sup> Number of Class |
| configuration?.autopasskeyconfirm | boolean | <sup>*(optional)*</sup> Enable autopass confirm |
| configuration?.persistmac | boolean | <sup>*(optional)*</sup> Enable persistent MAC |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the BluetoothControl plugin:

BluetoothControl interface methods:

| Method | Description |
| :-------- | :-------- |
| [scan](#method.scan) | Starts scanning for Bluetooth devices |
| [connect](#method.connect) | Connects to a Bluetooth device |
| [disconnect](#method.disconnect) | Disconnects from a connected Bluetooth device |
| [pair](#method.pair) | Pairs a Bluetooth device |
| [unpair](#method.unpair) | Unpairs a paired Bluetooth device |
| [abortpairing](#method.abortpairing) | Aborts the pairing process |
| [pincode](#method.pincode) | Specifies a PIN code for authentication during a legacy pairing process |
| [passkey](#method.passkey) | Specifies a passkey for authentication during a pairing process |
| [confirmpasskey](#method.confirmpasskey) | Confirms a passkey for authentication during a pairing process |


<a name="method.scan"></a>
## *scan <sup>method</sup>*

Starts scanning for Bluetooth devices.

Also see: [scancomplete](#event.scancomplete)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.type | string | Bluetooth device type (must be one of the following: *Classic*, *LowEnergy*) |
| params?.timeout | number | <sup>*(optional)*</sup> Duration of the scan (in seconds); default: 10 seconds |

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
        "timeout": 60
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

Disconnects from a connected Bluetooth device.

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

### Description

The client may expect PIN or passkey requests during the pairing process. The process can be cancelled any time by calling the *abortpairing* method

Also see: [devicestatechange](#event.devicestatechange), [pincoderequest](#event.pincoderequest), [passkeyrequest](#event.passkeyrequest), [passkeyconfirmrequest](#event.passkeyconfirmrequest)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params?.timeout | integer | <sup>*(optional)*</sup> Maximum time allowed for the pairing process to complete (in seconds); default: 20 seconds |

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
        "address": "81:6F:B0:91:9B:FE",
        "timeout": 60
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

Unpairs a paired Bluetooth device.

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

<a name="method.abortpairing"></a>
## *abortpairing <sup>method</sup>*

Aborts the pairing process.

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
| 5 | ```ERROR_ILLEGAL_STATE``` | Device not currently pairing |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "BluetoothControl.1.abortpairing",
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

<a name="method.pincode"></a>
## *pincode <sup>method</sup>*

Specifies a PIN code for authentication during a legacy pairing process.

### Description

This method is to be called upon receiving a *pincoderequest* event during a legacy pairing process. If the specified PIN code is incorrect the pairing process will be aborted.

Also see: [pincoderequest](#event.pincoderequest)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params.secret | string | A PIN code string, typically consisting of (but not limited to) four decimal digits |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |
| 5 | ```ERROR_ILLEGAL_STATE``` | Device not currently pairing or PIN code has not been requested |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "BluetoothControl.1.pincode",
    "params": {
        "address": "81:6F:B0:91:9B:FE",
        "secret": "0000"
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

<a name="method.passkey"></a>
## *passkey <sup>method</sup>*

Specifies a passkey for authentication during a pairing process.

### Description

This method is to be called upon receiving a *passkeyrequest* event during a pairing process. If the specified passkey is incorrect or empty the pairing process will be aborted.

Also see: [passkeyrequest](#event.passkeyrequest)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params.secret | integer | A six-digit decimal number passkey |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |
| 5 | ```ERROR_ILLEGAL_STATE``` | Device not currently pairing or a passkey has not been requested |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "BluetoothControl.1.passkey",
    "params": {
        "address": "81:6F:B0:91:9B:FE",
        "secret": 123456
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

<a name="method.confirmpasskey"></a>
## *confirmpasskey <sup>method</sup>*

Confirms a passkey for authentication during a pairing process.

### Description

This method is to be called upon receiving a *passkeyconfirmationrequest* event during a pairing process. If the confirmation is negative or the pairing process will be aborted.

Also see: [passkeyconfirmrequest](#event.passkeyconfirmrequest)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params.iscorrect | boolean | Specifies if the passkey sent in *passkeyconfirmrequest* event is correct (true) or incorrect (false) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |
| 5 | ```ERROR_ILLEGAL_STATE``` | Device is currently not pairing or passkey confirmation has not been requested |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 1234567890,
    "method": "BluetoothControl.1.confirmpasskey",
    "params": {
        "address": "81:6F:B0:91:9B:FE",
        "iscorrect": true
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
| (property)?.manufacturer | number | <sup>*(optional)*</sup> Device manufacturer Company Identifer |
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
| (property)?.class | integer | <sup>*(optional)*</sup> Class of device (3 octets) |
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
        "class": 2360324,
        "connected": true,
        "paired": true
    }
}
```

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the implementation, and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the BluetoothControl plugin:

BluetoothControl interface events:

| Event | Description |
| :-------- | :-------- |
| [scancomplete](#event.scancomplete) | Notifies about scan completion |
| [devicestatechange](#event.devicestatechange) | Notifies about device state change |
| [pincoderequest](#event.pincoderequest) | Notifies about a PIN code request |
| [passkeyrequest](#event.passkeyrequest) | Notifies about a passkey request |
| [passkeyconfirmrequest](#event.passkeyconfirmrequest) | Notifies about a passkey confirmation request |


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
| params.state | string | Device state (must be one of the following: *Pairing*, *Paired*, *Unpaired*, *Connected*, *Disconnected*) |
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

<a name="event.pincoderequest"></a>
## *pincoderequest <sup>event</sup>*

Notifies about a PIN code request.

### Description

Register to this event to be notified about PIN code requests during a legacy pairing process. Upon receiving this event the client is required to respond with a *pincode* call in order to complete the pairing procedure. The PIN code value would typically be collected by prompting the end-user. If the client fails to respond before the pairing timeout elapses the pairing procedure will be aborted.<br><br>Note that this event will never be send for a Bluetooth LowEnergy device.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.pincoderequest",
    "params": {
        "address": "81:6F:B0:91:9B:FE"
    }
}
```

<a name="event.passkeyrequest"></a>
## *passkeyrequest <sup>event</sup>*

Notifies about a passkey request.

### Description

Register to this event to be notified about passkey requests that may be required during a pairing process. Upon receiving this event the client is required to respond with a *passkey* call in order to complete the pairing procedure. The passkey value would typically be collected by prompting the end-user. If the client fails to respond before the pairing timeout elapses the pairing procedure will be aborted.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.passkeyrequest",
    "params": {
        "address": "81:6F:B0:91:9B:FE"
    }
}
```

<a name="event.passkeyconfirmrequest"></a>
## *passkeyconfirmrequest <sup>event</sup>*

Notifies about a passkey confirmation request.

### Description

Register to this event to be notified about passkey confirmation requests that may required during a pairing process. Upon receiving this event the client is required to respond with a *passkeyconfirm* call in order to complete the pairing procedure. The passkey confirmation would typically be collected by prompting the end-user. If the client fails to respond before the pairing timeout elapses the pairing procedure will be aborted.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params.secret | integer | A six-digit decimal number passkey sent by the remote device for confirmation |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.passkeyconfirmrequest",
    "params": {
        "address": "81:6F:B0:91:9B:FE",
        "secret": 123456
    }
}
```

